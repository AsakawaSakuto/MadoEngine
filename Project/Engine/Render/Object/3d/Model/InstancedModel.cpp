#include "InstancedModel.h"
#include "Core/View/SRVManager.h"
#include "Render/Shadow/ShadowMap.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace {

constexpr size_t kInitialInstanceCapacity = 1;
constexpr UINT kInstancedRootMaterial = 0;
constexpr UINT kInstancedRootInstance = 1;
constexpr UINT kInstancedRootTexture = 2;
constexpr UINT kInstancedRootCamera = 3;
constexpr UINT kInstancedRootEnvironment = 4;
constexpr UINT kInstancedRootLight = 5;
constexpr UINT kInstancedRootShadow = 6;
constexpr UINT kInstancedRootShadowMap = 7;
constexpr float kInstancedShadowCompareBias = 0.00005f;

/// @brief 指定値以上の2の累乗を取得します。
/// @param value 必要な要素数です。
/// @return 確保する要素数です。
size_t CalculateCapacity(size_t value) {
	size_t capacity = kInitialInstanceCapacity;
	while (capacity < value) {
		capacity *= 2;
	}
	return capacity;
}

/// @brief インスタンスモデル用のシャドウマップ生成PSO設定を作成します。
/// @return インスタンスモデル用のシャドウマップ生成PSO設定です。
MadoEngine::Render::PSODesc CreateInstancedShadowPSODesc() {
	MadoEngine::Render::PSODesc desc = MadoEngine::Render::ShadowMap::CreatePSODesc();
	desc.vsKey = "Object3d/Shadow/InstancedShadowMap.VS";
	desc.rootSigKey = "InstancedModel.RootSig";
	return desc;
}

} // namespace

InstancedModel::InstancedModel(std::string objectName) {
	objectName_ = std::move(objectName);
	color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	isVisible_ = true;
}

InstancedModel::~InstancedModel() {
	if (instanceSrvIndex_ != UINT32_MAX) {
		MadoEngine::Core::SRVManager::GetInstance().Free(instanceSrvIndex_);
		instanceSrvIndex_ = UINT32_MAX;
	}
	if (shadowInstanceSrvIndex_ != UINT32_MAX) {
		MadoEngine::Core::SRVManager::GetInstance().Free(shadowInstanceSrvIndex_);
		shadowInstanceSrvIndex_ = UINT32_MAX;
	}
}

void InstancedModel::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData) {
	assert(device);
	assert(commandList);

	device_ = device;
	commandList_ = commandList;
	sharedData_ = &sharedData;

	InitializeInstanceResources();
}

void InstancedModel::InitializeInstanceResources() {
	assert(sharedData_);

	psoDesc_ = sharedData_->psoDesc;
	psoDesc_.inputLayout = MadoEngine::Render::InputLayoutType::StaticModel;
	psoDesc_.vsKey = "Object3d/Model/InstancedModel.VS";
	psoDesc_.psKey = "Object3d/Model/Model.PS";
	psoDesc_.rootSigKey = "InstancedModel.RootSig";

	textureNames_ = sharedData_->textureNames;
	textureIndices_ = sharedData_->textureIndices;
	if (!textureIndices_.empty()) {
		textureIndex_ = textureIndices_[0];
	}

	materialData_ = CreateMappedBuffer<ModelMaterial>(device_.Get(), materialResource_);
	materialData_->color = color_;
	materialData_->enableLighting = enableLighting_ ? 1 : 0;
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();
	materialData_->useEnvironmentMap = useEnvironmentMap_ ? 1 : 0;

	cameraData_ = CreateMappedBuffer<CameraForGPU>(device_.Get(), cameraResource_);
	lightGpuData_ = CreateMappedBuffer<LightGpuData>(device_.Get(), lightGpuDataResource_);
	UpdateLightGpuData();

	shadowGpuData_ = CreateMappedBuffer<ModelShadowGpuData>(device_.Get(), shadowGpuDataResource_);
	UpdateReceiveShadowGpuData();

	instanceSrvIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
	shadowInstanceSrvIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
	EnsureInstanceResource(kInitialInstanceCapacity);
	EnsureShadowInstanceResource(kInitialInstanceCapacity);
}

/// @brief 現在のインスタンスTransformからワールド行列を作成する
/// @param transform ワールド行列へ変換するTransform
/// @param billboardCamera ビルボードの向きに使用するカメラ。nullptrの場合は通常の回転を使用する
/// @return 作成したワールド行列
Matrix4x4 InstancedModel::MakeWorldMatrix(const Transform3D& transform, const Camera* billboardCamera) const {
	if (!usebillbord_ || !billboardCamera) {
		return Matrix::MakeAffine(transform.scale, transform.rotate, transform.translate);
	}

	Matrix4x4 scaleMatrix = Matrix::MakeScale(transform.scale);
	Matrix4x4 rotateMatrix = Matrix::MakeAffine({ 1.0f, 1.0f, 1.0f }, transform.rotate, { 0.0f, 0.0f, 0.0f });
	Matrix4x4 billboardMatrix = Matrix::Inverse(billboardCamera->GetViewMatrix());
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;
	billboardMatrix.m[3][3] = 1.0f;

	Matrix4x4 translateMatrix = Matrix::MakeTranslate(transform.translate);
	return Matrix::Multiply(Matrix::Multiply(Matrix::Multiply(scaleMatrix, rotateMatrix), billboardMatrix), translateMatrix);
}

/// @brief 通常描画で参照するシャドウマップ情報を設定します。
/// @param shadowMapSrv シャドウマップSRVのGPUディスクリプタハンドルです。
/// @param lightViewProjection ライト視点のビュー射影行列です。
/// @param width シャドウマップの幅です。
/// @param height シャドウマップの高さです。
void InstancedModel::SetShadowMap(
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
	const Matrix4x4& lightViewProjection,
	uint32_t width,
	uint32_t height) {
	shadowMapSrvHandle_ = shadowMapSrv;
	shadowLightViewProjection_ = lightViewProjection;
	shadowMapWidth_ = width;
	shadowMapHeight_ = height;
	UpdateReceiveShadowGpuData();
}

void InstancedModel::Update() {
	if (!materialData_) {
		return;
	}

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
	materialData_->enableLighting = enableLighting_ ? 1 : 0;
	materialData_->uvTransformMatrix = scale * rot * trans;
	materialData_->useEnvironmentMap = useEnvironmentMap_ ? 1 : 0;
}

void InstancedModel::Draw(Camera& useCamera) {
	if (!sharedData_ || !isVisible_ || instances_.empty()) {
		return;
	}

	camera_ = useCamera;
	cameraData_->worldPosition = camera_.GetPosition();
	UpdateLightGpuData();

	Matrix4x4 viewProjectionMatrix = camera_.GetViewProjectionMatrix();
	EnsureInstanceResource(instances_.size());
	drawInstanceCount_ = BuildInstanceGpuData(viewProjectionMatrix, instanceGpuData_, &camera_);

	if (drawInstanceCount_ == 0) {
		return;
	}

	assert(psoRegistry_);
	commandList_->SetGraphicsRootSignature(MadoEngine::RootSignatureManager::GetInstance().Get(psoDesc_.rootSigKey));
	commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));
	commandList_->IASetVertexBuffers(0, 1, &sharedData_->vertexBufferView);
	commandList_->IASetIndexBuffer(&sharedData_->indexBufferView);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList_->SetGraphicsRootConstantBufferView(kInstancedRootMaterial, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(kInstancedRootInstance, MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(instanceSrvIndex_));
	commandList_->SetGraphicsRootDescriptorTable(kInstancedRootTexture, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(kInstancedRootCamera, cameraResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(kInstancedRootEnvironment, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(kInstancedRootLight, lightGpuDataResource_->GetGPUVirtualAddress());

	UpdateReceiveShadowGpuData();
	const D3D12_GPU_DESCRIPTOR_HANDLE fallbackSrv = MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_);
	const D3D12_GPU_DESCRIPTOR_HANDLE shadowSrv = shadowMapSrvHandle_.ptr != 0 ? shadowMapSrvHandle_ : fallbackSrv;
	commandList_->SetGraphicsRootConstantBufferView(kInstancedRootShadow, shadowGpuDataResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(kInstancedRootShadowMap, shadowSrv);

	if (!sharedData_->modelData.subMeshes.empty()) {
		for (const auto& subMesh : sharedData_->modelData.subMeshes) {
			uint32_t texIndex = textureIndex_;
			if (subMesh.materialIndex < textureIndices_.size()) {
				texIndex = textureIndices_[subMesh.materialIndex];
			}

			commandList_->SetGraphicsRootDescriptorTable(kInstancedRootTexture, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(texIndex));
			commandList_->DrawIndexedInstanced(subMesh.indexCount, static_cast<UINT>(drawInstanceCount_), subMesh.indexStart, 0, 0);
		}
	} else {
		commandList_->DrawIndexedInstanced(static_cast<UINT>(sharedData_->modelData.indeces.size()), static_cast<UINT>(drawInstanceCount_), 0, 0, 0);
	}
}

/// @brief シャドウマップへインスタンスモデルの深度を書き込みます。
/// @param lightViewProjection ライト視点のビュー射影行列です。
void InstancedModel::DrawShadow(const Matrix4x4& lightViewProjection) {
	const Camera* billboardCamera = usebillbord_ ? &camera_ : nullptr;
	DrawShadowInternal(lightViewProjection, billboardCamera);
}

/// @brief シャドウマップ生成用にインスタンシングモデルを描画する
/// @param lightViewProjection ライト視点のビュープロジェクション行列
/// @param billboardCamera ビルボードの向きに使用するカメラ
void InstancedModel::DrawShadow(const Matrix4x4& lightViewProjection, const Camera& billboardCamera) {
	camera_ = billboardCamera;
	DrawShadowInternal(lightViewProjection, &billboardCamera);
}

/// @brief シャドウマップ生成用の共通描画処理を行う
/// @param lightViewProjection ライト視点のビュープロジェクション行列
/// @param billboardCamera ビルボードの向きに使用するカメラ。nullptrの場合は通常の回転を使用する
void InstancedModel::DrawShadowInternal(const Matrix4x4& lightViewProjection, const Camera* billboardCamera) {
	if (!sharedData_ || !isVisible_ || !castShadow_ || instances_.empty()) {
		return;
	}

	EnsureShadowInstanceResource(instances_.size());
	shadowDrawInstanceCount_ = BuildInstanceGpuData(lightViewProjection, shadowInstanceGpuData_, billboardCamera);
	if (shadowDrawInstanceCount_ == 0) {
		return;
	}

	assert(psoRegistry_);
	const MadoEngine::Render::PSODesc shadowPsoDesc = CreateInstancedShadowPSODesc();
	commandList_->SetGraphicsRootSignature(MadoEngine::RootSignatureManager::GetInstance().Get(shadowPsoDesc.rootSigKey));
	commandList_->SetPipelineState(psoRegistry_->Get(shadowPsoDesc));
	commandList_->IASetVertexBuffers(0, 1, &sharedData_->vertexBufferView);
	commandList_->IASetIndexBuffer(&sharedData_->indexBufferView);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootDescriptorTable(kInstancedRootInstance, MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(shadowInstanceSrvIndex_));

	if (!sharedData_->modelData.subMeshes.empty()) {
		for (const auto& subMesh : sharedData_->modelData.subMeshes) {
			commandList_->DrawIndexedInstanced(subMesh.indexCount, static_cast<UINT>(shadowDrawInstanceCount_), subMesh.indexStart, 0, 0);
		}
	} else {
		commandList_->DrawIndexedInstanced(static_cast<UINT>(sharedData_->modelData.indeces.size()), static_cast<UINT>(shadowDrawInstanceCount_), 0, 0, 0);
	}
}

uint32_t InstancedModel::AddInstance(const InstanceDesc& desc) {
	instances_.push_back(desc);
	EnsureInstanceResource(instances_.size());
	return static_cast<uint32_t>(instances_.size() - 1);
}

void InstancedModel::ClearInstances() {
	instances_.clear();
	drawInstanceCount_ = 0;
}

void InstancedModel::SetInstanceTransform(uint32_t handle, const Transform3D& transform) {
	if (!IsValidHandle(handle)) {
		return;
	}
	instances_[handle].transform = transform;
}

void InstancedModel::SetInstanceColor(uint32_t handle, const Vector4& color) {
	if (!IsValidHandle(handle)) {
		return;
	}
	instances_[handle].color = color;
}

void InstancedModel::SetInstanceVisible(uint32_t handle, bool isVisible) {
	if (!IsValidHandle(handle)) {
		return;
	}
	instances_[handle].isVisible = isVisible;
}

void InstancedModel::SetSceneType(SceneType sceneType) {
	sceneType_ = sceneType;
	UpdateLightGpuData();
}

void InstancedModel::SetReceiveLightMask(LightLayerMask receiveLightMask) {
	receiveLightMask_ = receiveLightMask;
	UpdateLightGpuData();
}

void InstancedModel::EnsureInstanceResource(size_t requiredCount) {
	if (requiredCount <= instanceCapacity_) {
		return;
	}

	instanceCapacity_ = CalculateCapacity(requiredCount);
	instanceGpuData_ = CreateMappedBuffer<InstanceForGPU>(device_.Get(), instanceResource_, instanceCapacity_);
	MadoEngine::Core::SRVManager::GetInstance().CreateStructuredBufferSRV(
		instanceResource_.Get(),
		instanceSrvIndex_,
		static_cast<uint32_t>(instanceCapacity_),
		sizeof(InstanceForGPU)
	);
}

/// @brief 指定されたビュー射影行列でインスタンスGPUデータを更新します。
/// @param viewProjectionMatrix 変換に使用するビュー射影行列です。

/// @brief シャドウ描画用のインスタンスGPUデータ領域を確保します。
/// @param requiredCount 必要なインスタンス数です。
void InstancedModel::EnsureShadowInstanceResource(size_t requiredCount) {
	if (requiredCount <= shadowInstanceCapacity_) {
		return;
	}

	shadowInstanceCapacity_ = CalculateCapacity(requiredCount);
	shadowInstanceGpuData_ = CreateMappedBuffer<InstanceForGPU>(device_.Get(), shadowInstanceResource_, shadowInstanceCapacity_);
	MadoEngine::Core::SRVManager::GetInstance().CreateStructuredBufferSRV(
		shadowInstanceResource_.Get(),
		shadowInstanceSrvIndex_,
		static_cast<uint32_t>(shadowInstanceCapacity_),
		sizeof(InstanceForGPU)
	);
}

/// @brief 指定されたビュー射影行列でインスタンスGPUデータを更新します。
/// @param viewProjectionMatrix 変換に使用するビュー射影行列です。
/// @param outputData 書き込み先のGPUデータです。
/// @param billboardCamera ビルボードの向きに使用するカメラ。nullptrの場合は通常の回転を使用する
/// @return 描画対象のインスタンス数です。
size_t InstancedModel::BuildInstanceGpuData(const Matrix4x4& viewProjectionMatrix, InstanceForGPU* outputData, const Camera* billboardCamera) {
	if (!outputData) {
		return 0;
	}

	size_t drawCount = 0;
	for (const InstanceDesc& instance : instances_) {
		if (!instance.isVisible) {
			continue;
		}

		Matrix4x4 worldMatrix = MakeWorldMatrix(instance.transform, billboardCamera);
		Matrix4x4 worldViewProjectionMatrix = Matrix::Multiply(worldMatrix, viewProjectionMatrix);
		Matrix4x4 worldInverseTransposeMatrix = Matrix::Transpose(Matrix::Inverse(worldMatrix));

		InstanceForGPU& gpuData = outputData[drawCount];
		gpuData.WVP = worldViewProjectionMatrix;
		gpuData.World = worldMatrix;
		gpuData.WorldInverseTranspose = worldInverseTransposeMatrix;
		gpuData.color = instance.color;
		++drawCount;
	}
	return drawCount;
}

void InstancedModel::UpdateLightGpuData() {
	if (!lightGpuData_) {
		return;
	}

	*lightGpuData_ = LightManager::GetInstance().GetCachedGpuData(sceneType_, receiveLightMask_);
}

/// @brief 影を受けるためのGPUデータを更新します。
void InstancedModel::UpdateReceiveShadowGpuData() {
	if (!shadowGpuData_) {
		return;
	}

	shadowGpuData_->lightViewProjection = shadowLightViewProjection_;
	shadowGpuData_->shadowMapInfo = {
		static_cast<float>(shadowMapWidth_),
		static_cast<float>(shadowMapHeight_),
		kInstancedShadowCompareBias,
		0.0f
	};
	shadowGpuData_->useShadow = (receiveShadow_ && shadowMapSrvHandle_.ptr != 0) ? 1u : 0u;
}

bool InstancedModel::IsValidHandle(uint32_t handle) const {
	return handle < instances_.size();
}
