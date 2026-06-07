#include "Model.h"
#include "Core/View/SRVManager.h"

Model::Model(std::string ObjectName) {
	objectName_ = ObjectName;
}

void Model::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) {
	device_ = device;
	commandList_ = commandList;

	// 拡張子を取得
	std::string extension = "";
	size_t dotPos = modelPath.rfind('.');
	if (dotPos != std::string::npos) {
		extension = modelPath.substr(dotPos);
		// 小文字に変換
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	}

	// 拡張子によって処理を分岐
	if (extension == ".obj") {

		// .obj形式の処理
		modelData_ = LoadObject3dFile(modelPath);
	}

	if (extension == ".gltf" || extension == ".glb") {

		// .gltf/.glb形式の処理
		modelData_ = LoadObject3dFile(modelPath);
		animationData_ = LoadAnimationFile(modelPath);

	}

	if (!modelData_.skinClusterData.empty()) {
		useAnimationTimer_ = true;

		skeletonData_ = CreateSkeleton(modelData_.rootNode);

		skinClusterIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate(); // スキンクラスター用のSRVインデックスを確保

		skinClusterData_ = CreateSkinCluster(
			device_,
			skeletonData_,
			modelData_,
			skinClusterIndex_ // 動的に割り当てられたインデックスを使用
		);
	}

	// マルチマテリアル対応：すべてのマテリアルのテクスチャを読み込む
	textureNames_.resize(modelData_.materialPaths.size());
	textureIndices_.resize(modelData_.materialPaths.size());
	for (size_t i = 0; i < modelData_.materialPaths.size(); ++i) {
		textureNames_[i] = modelData_.materialPaths[i];
		textureIndices_[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(textureNames_[i]);
	}

	// 頂点リソースをつくる（共有）
	indexResource_ = CreateBufferResource(device_.Get(), sizeof(uint32_t) * modelData_.indeces.size());
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress(); // ここのエラーは.mltファイルのTexturePathが間違えてる可能性が高い
	indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * modelData_.indeces.size());
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexData_ = CreateMappedBuffer<uint32_t>(device_.Get(), indexResource_, modelData_.indeces.size());

	// 頂點リソースをつくる（共有）
	vertexResource_ = CreateBufferResource(device_.Get(), sizeof(ModelVertexData) * modelData_.vertices.size());
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(sizeof(ModelVertexData) * modelData_.vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(ModelVertexData);
	vertexData_ = CreateMappedBuffer<ModelVertexData>(device_.Get(), vertexResource_, modelData_.vertices.size());

	materialData_ = CreateMappedBuffer<ModelMaterial>(device_.Get(), materialResource_);
	transformationData_ = CreateMappedBuffer<ModelTransformationMatrix>(device_.Get(), transformationResource_);
	cameraData_ = CreateMappedBuffer<CameraForGPU>(device_.Get(), cameraResource_);
	directionalLightData_ = CreateMappedBuffer<DirectionalLight>(device_.Get(), directionalLightResource_);
	pointLightData_ = CreateMappedBuffer<PointLight>(device_.Get(), pointLightResource_);
	spotLightData_ = CreateMappedBuffer<SpotLight>(device_.Get(), spotLightResource_);
}

void Model::Update() {
	
	if (useAnimationTimer_) {
		animationTime_ += 1.0f / 60.0f;
		animationTime_ = fmod(animationTime_, animationData_.duration);
	}

	ApplyAnimation(skeletonData_, animationData_, animationTime_);
	UpdateAnimation(skeletonData_);
	UpdateCluster(skinClusterData_, skeletonData_);

	cameraData_->worldPosition = camera_.GetPosition(); // カメラの位置を渡す

	// 行列の内容を更新
	worldMatrix_ = Matrix::MakeAffine(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 cameraMatrix = Matrix::MakeAffine({1.0f,1.0f,1.0f}, camera_.GetRotation(), camera_.GetPosition());
	Matrix4x4 viewMatrix = Matrix::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix::MakePerspectiveFov(camera_.GetFovY(), static_cast<float>(1280) / static_cast<float>(720), camera_.GetNearClip(), camera_.GetFarClip());
	Matrix4x4 worldViewProjectionMatrix = Matrix::Multiply(worldMatrix_, Matrix::Multiply(viewMatrix, projectionMatrix));

	// 追加処理：法線変換行列を計算
	Matrix4x4 worldInverseMatrix = Matrix::Inverse(worldMatrix_);

	switch (type_)
	{
	case ModelType::Animated:
	{
		NodeAnimation& rootNodeAnimation = animationData_.nodeAnimations[modelData_.rootNode.name];
		Vector3 translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime_);
		Quaternion rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime_);
		Vector3 scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime_);
		Matrix4x4 localMatrix = Matrix::MakeAffineAnimation(scale, rotate, translate);

		modelData_.rootNode.localMatrix = localMatrix;

		transformationData_->WVP = Matrix::Multiply(modelData_.rootNode.localMatrix, worldViewProjectionMatrix);
		transformationData_->World = Matrix::Multiply(modelData_.rootNode.localMatrix, worldMatrix_);

		break;
	}
	case ModelType::Skinning:
	{
		transformationData_->WVP = worldViewProjectionMatrix;
		transformationData_->World = worldMatrix_;

		break;
	}
	}

	transformationData_->WorldInverseTranspose = worldInverseMatrix;

	// UV変換行列の計算
	Matrix4x4 scale = Matrix::MakeIdentity();
	scale.m[0][0] = uvTransform_.scale.x;
	scale.m[1][1] = uvTransform_.scale.y;

	Matrix4x4 rot = Matrix::MakeIdentity();
	rot.m[0][0] = cos(uvTransform_.rotate);
	rot.m[0][1] = -sin(uvTransform_.rotate);
	rot.m[1][0] = sin(uvTransform_.rotate);
	rot.m[1][1] = cos(uvTransform_.rotate);

	Matrix4x4 trans = Matrix::MakeIdentity();
	trans.m[3][0] = uvTransform_.translate.x;
	trans.m[3][1] = uvTransform_.translate.y;

	// 最終変換行列
	materialData_->uvTransformMatrix = scale * rot * trans;

}

void Model::Draw(Camera& useCamera) {
	
	camera_ = useCamera;

    //commandList_->SetGraphicsRootSignature();
	//commandList_->SetPipelineState();

	// 通常モデルは1つのVBVのみ
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);

	commandList_->IASetIndexBuffer(&indexBufferView_);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 共通パラメータの設定
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

	// 環境マップ（CubeMap）の設定
	if (useEnvironmentMap_) {
		commandList_->SetGraphicsRootDescriptorTable(7, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(environmentMapIndex_));
	} else {
		// 環境マップが未設定の場合はデフォルトテクスチャ（t0と同じ）をバインド
		commandList_->SetGraphicsRootDescriptorTable(7, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	}

	// サブメッシュがある場合はマルチマテリアル描画
	if (!modelData_.subMeshes.empty()) {
		for (const auto& subMesh : modelData_.subMeshes) {
			// サブメッシュのマテリアルインデックスに対応するテクスチャを設定
			uint32_t texIndex = textureIndex_; // デフォルト
			if (subMesh.materialIndex < textureIndices_.size()) {
				texIndex = textureIndices_[subMesh.materialIndex];
			}

			// テクスチャを設定
			commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(texIndex));

			// サブメッシュを描画
			commandList_->DrawIndexedInstanced(
				subMesh.indexCount, 1, subMesh.indexStart, 0, 0);
		}
	} else {
		// 従来の単一マテリアル描画（後方互換性）
		commandList_->DrawIndexedInstanced(
			static_cast<UINT>(modelData_.indeces.size()), 1, 0, 0, 0);
	}
}