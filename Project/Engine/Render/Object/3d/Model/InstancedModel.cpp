#include "InstancedModel.h"
#include "Core/View/SRVManager.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace {

constexpr size_t kInitialInstanceCapacity = 1;

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
}

void InstancedModel::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) {
	ownedSharedData_ = std::make_unique<ModelSharedData>();
	MadoEngine::ModelResource::Initialize(*ownedSharedData_, device, modelPath);
	Initialize(device, commandList, *ownedSharedData_);
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

	instanceSrvIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
	EnsureInstanceResource(kInitialInstanceCapacity);
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
	EnsureInstanceResource(instances_.size());

	Matrix4x4 cameraMatrix = Matrix::MakeAffine({ 1.0f, 1.0f, 1.0f }, camera_.GetRotation(), camera_.GetPosition());
	Matrix4x4 viewMatrix = Matrix::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix::MakePerspectiveFov(camera_.GetFovY(), camera_.GetAspectRatio(), camera_.GetNearClip(), camera_.GetFarClip());

	drawInstanceCount_ = 0;
	for (const InstanceDesc& instance : instances_) {
		if (!instance.isVisible) {
			continue;
		}

		Matrix4x4 worldMatrix = Matrix::MakeAffine(instance.transform.scale, instance.transform.rotate, instance.transform.translate);
		Matrix4x4 worldViewProjectionMatrix = Matrix::Multiply(worldMatrix, Matrix::Multiply(viewMatrix, projectionMatrix));
		Matrix4x4 worldInverseTransposeMatrix = Matrix::Transpose(Matrix::Inverse(worldMatrix));

		InstanceForGPU& gpuData = instanceGpuData_[drawInstanceCount_];
		gpuData.WVP = worldViewProjectionMatrix;
		gpuData.World = worldMatrix;
		gpuData.WorldInverseTranspose = worldInverseTransposeMatrix;
		gpuData.color = instance.color;
		++drawInstanceCount_;
	}

	if (drawInstanceCount_ == 0) {
		return;
	}

	assert(psoRegistry_);
	commandList_->SetGraphicsRootSignature(MadoEngine::RootSignatureManager::GetInstance().Get(psoDesc_.rootSigKey));
	commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));
	commandList_->IASetVertexBuffers(0, 1, &sharedData_->vertexBufferView);
	commandList_->IASetIndexBuffer(&sharedData_->indexBufferView);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(1, MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(instanceSrvIndex_));
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(4, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	commandList_->SetGraphicsRootConstantBufferView(5, lightGpuDataResource_->GetGPUVirtualAddress());

	if (!sharedData_->modelData.subMeshes.empty()) {
		for (const auto& subMesh : sharedData_->modelData.subMeshes) {
			uint32_t texIndex = textureIndex_;
			if (subMesh.materialIndex < textureIndices_.size()) {
				texIndex = textureIndices_[subMesh.materialIndex];
			}

			commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(texIndex));
			commandList_->DrawIndexedInstanced(subMesh.indexCount, static_cast<UINT>(drawInstanceCount_), subMesh.indexStart, 0, 0);
		}
	} else {
		commandList_->DrawIndexedInstanced(static_cast<UINT>(sharedData_->modelData.indeces.size()), static_cast<UINT>(drawInstanceCount_), 0, 0, 0);
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

void InstancedModel::UpdateLightGpuData() {
	if (!lightGpuData_) {
		return;
	}

	*lightGpuData_ = LightManager::GetInstance().GetCachedGpuData(sceneType_, receiveLightMask_);
}

bool InstancedModel::IsValidHandle(uint32_t handle) const {
	return handle < instances_.size();
}
