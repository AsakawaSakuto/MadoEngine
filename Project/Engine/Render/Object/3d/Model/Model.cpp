#include "Model.h"
#include "Core/View/SRVManager.h"
#include "Shader/RootSignatureManager.h"
#include "../Animation/AnimationFunction.h"
#include <cassert>
#include <cmath>

Model::Model(std::string objectName) {
	objectName_ = objectName;
	color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	isVisible_ = true;
}

void Model::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) {
	ownedSharedData_ = std::make_unique<ModelSharedData>();
	MadoEngine::ModelResource::Initialize(*ownedSharedData_, device, modelPath);
	Initialize(device, commandList, *ownedSharedData_);
}

void Model::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData) {
	assert(device);
	assert(commandList);

	device_ = device;
	commandList_ = commandList;
	sharedData_ = &sharedData;

	InitializeInstanceResources();
}

void Model::InitializeInstanceResources() {
	assert(sharedData_);

	type_ = sharedData_->type;
	psoDesc_ = sharedData_->psoDesc;
	rootNode_ = sharedData_->modelData.rootNode;
	textureNames_ = sharedData_->textureNames;
	textureIndices_ = sharedData_->textureIndices;
	if (!textureIndices_.empty()) {
		textureIndex_ = textureIndices_[0];
	}

	useAnimationTimer_ = !sharedData_->animationData.nodeAnimations.empty();

	if (type_ == ModelType::Skinning) {
		useAnimationTimer_ = true;
		skeletonData_ = CreateSkeleton(sharedData_->modelData.rootNode);

		skinClusterIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
		skinClusterData_ = CreateSkinCluster(device_, skeletonData_, sharedData_->modelData, skinClusterIndex_);
	}

	materialData_ = CreateMappedBuffer<ModelMaterial>(device_.Get(), materialResource_);
	materialData_->color = color_;
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();
	materialData_->useEnvironmentMap = useEnvironmentMap_ ? 1 : 0;
	//materialData_->enableLighting = true;

	transformationData_ = CreateMappedBuffer<ModelTransformationMatrix>(device_.Get(), transformationResource_);
	transformationData_->WVP = Matrix::MakeIdentity();
	transformationData_->World = Matrix::MakeIdentity();
	transformationData_->WorldInverseTranspose = Matrix::MakeIdentity();

	cameraData_ = CreateMappedBuffer<CameraForGPU>(device_.Get(), cameraResource_);

	directionalLightData_ = CreateMappedBuffer<DirectionalLight>(device_.Get(), directionalLightResource_);
	directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };

	pointLightData_ = CreateMappedBuffer<PointLight>(device_.Get(), pointLightResource_);

	spotLightData_ = CreateMappedBuffer<SpotLight>(device_.Get(), spotLightResource_);
}

void Model::Update() {
	if (!sharedData_ || !materialData_ || !transformationData_ || !cameraData_) {
		return;
	}

	const Animation& animationData = sharedData_->animationData;

	if (useAnimationTimer_ && animationData.duration > 0.0f) {
		animationTime_ += 1.0f / 60.0f;
		animationTime_ = std::fmod(animationTime_, animationData.duration);
	}

	if (type_ == ModelType::Skinning && !skeletonData_.joints.empty()) {
		ApplyAnimation(skeletonData_, animationData, animationTime_);
		UpdateAnimation(skeletonData_);
		UpdateCluster(skinClusterData_, skeletonData_);
	}

	cameraData_->worldPosition = camera_.GetPosition();

	worldMatrix_ = Matrix::MakeAffine(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 cameraMatrix = Matrix::MakeAffine({ 1.0f, 1.0f, 1.0f }, camera_.GetRotation(), camera_.GetPosition());
	Matrix4x4 viewMatrix = Matrix::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix::MakePerspectiveFov(camera_.GetFovY(), camera_.GetAspectRatio(), camera_.GetNearClip(), camera_.GetFarClip());
	Matrix4x4 worldViewProjectionMatrix = Matrix::Multiply(worldMatrix_, Matrix::Multiply(viewMatrix, projectionMatrix));
	Matrix4x4 worldInverseTransposeMatrix = Matrix::Transpose(Matrix::Inverse(worldMatrix_));

	switch (type_) {
	case ModelType::Animated:
	{
		Matrix4x4 localMatrix = rootNode_.localMatrix;
		auto rootNodeIt = animationData.nodeAnimations.find(rootNode_.name);
		if (rootNodeIt != animationData.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = rootNodeIt->second;
			Vector3 translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime_);
			Quaternion rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime_);
			Vector3 scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime_);
			localMatrix = Matrix::MakeAffineAnimation(scale, rotate, translate);
		}

		rootNode_.localMatrix = localMatrix;
		transformationData_->WVP = Matrix::Multiply(rootNode_.localMatrix, worldViewProjectionMatrix);
		transformationData_->World = Matrix::Multiply(rootNode_.localMatrix, worldMatrix_);
		break;
	}
	case ModelType::Skinning:
	case ModelType::Static:
	default:
		transformationData_->WVP = worldViewProjectionMatrix;
		transformationData_->World = worldMatrix_;
		break;
	}

	transformationData_->WorldInverseTranspose = worldInverseTransposeMatrix;

	Matrix4x4 scale = Matrix::MakeIdentity();
	scale.m[0][0] = uvTransform_.scale.x;
	scale.m[1][1] = uvTransform_.scale.y;

	Matrix4x4 rot = Matrix::MakeIdentity();
	rot.m[0][0] = std::cos(uvTransform_.rotate);
	rot.m[0][1] = -std::sin(uvTransform_.rotate);
	rot.m[1][0] = std::sin(uvTransform_.rotate);
	rot.m[1][1] = std::cos(uvTransform_.rotate);

	Matrix4x4 trans = Matrix::MakeIdentity();
	trans.m[3][0] = uvTransform_.translate.x;
	trans.m[3][1] = uvTransform_.translate.y;

	materialData_->color = color_;
	materialData_->uvTransformMatrix = scale * rot * trans;
	materialData_->useEnvironmentMap = useEnvironmentMap_ ? 1 : 0;
}

void Model::Draw(Camera& useCamera) {
	if (!sharedData_ || !isVisible_) {
		return;
	}

	camera_ = useCamera;

	assert(psoRegistry_);
	commandList_->SetGraphicsRootSignature(
		MadoEngine::RootSignatureManager::GetInstance().Get(psoDesc_.rootSigKey));
	commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));

	if (type_ == ModelType::Skinning) {
		D3D12_VERTEX_BUFFER_VIEW vbvs[] = {
			sharedData_->vertexBufferView,
			skinClusterData_.influenceBufferView
		};
		commandList_->IASetVertexBuffers(0, _countof(vbvs), vbvs);
	} else {
		commandList_->IASetVertexBuffers(0, 1, &sharedData_->vertexBufferView);
	}

	commandList_->IASetIndexBuffer(&sharedData_->indexBufferView);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

	const UINT environmentRootIndex = (type_ == ModelType::Skinning) ? 8u : 7u;
	if (type_ == ModelType::Skinning) {
		commandList_->SetGraphicsRootDescriptorTable(7, skinClusterData_.paletteSrvHandle.second);
	}

	if (useEnvironmentMap_) {
		commandList_->SetGraphicsRootDescriptorTable(environmentRootIndex, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(environmentMapIndex_));
	} else {
		commandList_->SetGraphicsRootDescriptorTable(environmentRootIndex, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	}

	if (!sharedData_->modelData.subMeshes.empty()) {
		for (const auto& subMesh : sharedData_->modelData.subMeshes) {
			uint32_t texIndex = textureIndex_;
			if (subMesh.materialIndex < textureIndices_.size()) {
				texIndex = textureIndices_[subMesh.materialIndex];
			}

			commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(texIndex));
			commandList_->DrawIndexedInstanced(subMesh.indexCount, 1, subMesh.indexStart, 0, 0);
		}
	} else {
		commandList_->DrawIndexedInstanced(static_cast<UINT>(sharedData_->modelData.indeces.size()), 1, 0, 0, 0);
	}
}
