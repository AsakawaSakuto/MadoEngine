#include "CylinderEffectRenderer3d.h"
#include "Core/TextureManager/TextureManager.h"
#include "Core/View/SRVManager.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>

namespace {

	constexpr std::size_t kInitialCylinderInstanceCapacity = 64;
	constexpr UINT kCylinderRootInstances = 0;
	constexpr UINT kCylinderRootPerView = 1;
	constexpr UINT kCylinderRootTexture = 2;

} // namespace

namespace MadoEngine::Effect {

	CylinderEffectRenderer3d::~CylinderEffectRenderer3d() {
		Finalize();
	}

	void CylinderEffectRenderer3d::Initialize(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		MadoEngine::Render::PSORegistry* psoRegistry) {
		assert(device);
		assert(commandList);
		assert(psoRegistry);

		Finalize();
		device_ = device;
		commandList_ = commandList;
		psoRegistry_ = psoRegistry;
		mappedPerView_ = CreateMappedBuffer<PerViewForGPU>(device_, perViewResource_, 1, false);
		instanceSrvIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
		EnsureInstanceCapacity(kInitialCylinderInstanceCapacity);
		isInitialized_ = true;
		Logger::Output("CylinderEffectRenderer3dを初期化しました。", Logger::Level::Engine);
	}

	void CylinderEffectRenderer3d::Finalize() {
		if (mappedInstances_ && instanceResource_) {
			instanceResource_->Unmap(0, nullptr);
		}
		if (mappedPerView_ && perViewResource_) {
			perViewResource_->Unmap(0, nullptr);
		}
		mappedInstances_ = nullptr;
		mappedPerView_ = nullptr;

		if (instanceSrvIndex_ != (std::numeric_limits<uint32_t>::max)()) {
			MadoEngine::Core::SRVManager::GetInstance().Free(instanceSrvIndex_);
			instanceSrvIndex_ = (std::numeric_limits<uint32_t>::max)();
		}

		instanceResource_.Reset();
		perViewResource_.Reset();
		instances_.clear();
		batches_.clear();
		geometryCache_.clear();
		textureIndexCache_.clear();
		instanceCapacity_ = 0;
		isInstanceDataDirty_ = false;
		device_ = nullptr;
		commandList_ = nullptr;
		psoRegistry_ = nullptr;
		isInitialized_ = false;
	}

	void CylinderEffectRenderer3d::Begin(const Camera& camera) {
		if (!isInitialized_) {
			return;
		}

		instances_.clear();
		batches_.clear();
		isInstanceDataDirty_ = true;
		mappedPerView_->viewProjection = camera.GetViewProjectionMatrix();
	}

	void CylinderEffectRenderer3d::Submit(const CylinderRenderData& data) {
		if (!isInitialized_) {
			return;
		}

		CylinderInstanceForGPU instance;
		instance.world = Matrix::MakeAffine(
			data.transform.scale,
			data.transform.rotate,
			data.transform.translate
		);
		instance.radii = {
			data.bottomRadii.x,
			data.bottomRadii.y,
			data.topRadii.x,
			data.topRadii.y,
		};
		instance.geometry = {
			data.height,
			data.startAngleRadians,
			data.arcAngleRadians,
			data.globalAlpha,
		};
		instance.uvTransform = {
			data.uvScale.x,
			data.uvScale.y,
			data.uvOffset.x,
			data.uvOffset.y,
		};
		instance.effectParameters = {
			data.uvRotationRadians,
			data.bottomFadeRange,
			data.topFadeRange,
			static_cast<float>(data.pivot),
		};

		uint32_t gradientCount = (std::min)(data.gradientCount, kMaximumCylinderGradientStops);
		if (gradientCount == 0) {
			gradientCount = 1;
			instance.gradientColors[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
			instance.gradientPositions[0].x = 0.0f;
		} else {
			for (uint32_t index = 0; index < gradientCount; ++index) {
				instance.gradientColors[index] = data.gradient[index].color;
				instance.gradientPositions[index / 4][index % 4] = data.gradient[index].position;
			}
		}
		instance.metadata[0] = data.radialSegments;
		instance.metadata[1] = data.heightSegments;
		instance.metadata[2] = gradientCount;
		instance.metadata[3] = static_cast<uint32_t>(data.uvDirection);

		const GeometryKey geometryKey = { data.radialSegments, data.heightSegments };
		const uint32_t textureIndex = ResolveTextureIndex(data.textureName);
		const uint32_t firstInstance = static_cast<uint32_t>(instances_.size());
		instances_.push_back(instance);

		if (!batches_.empty()) {
			DrawBatch& lastBatch = batches_.back();
			if (lastBatch.geometryKey == geometryKey &&
				lastBatch.textureIndex == textureIndex &&
				lastBatch.blendMode == data.blendMode &&
				lastBatch.cullMode == data.cullMode &&
				lastBatch.renderLayer == data.renderLayer &&
				lastBatch.firstInstance + lastBatch.instanceCount == firstInstance) {
				++lastBatch.instanceCount;
				isInstanceDataDirty_ = true;
				return;
			}
		}

		DrawBatch batch;
		batch.geometryKey = geometryKey;
		batch.firstInstance = firstInstance;
		batch.instanceCount = 1;
		batch.textureIndex = textureIndex;
		batch.blendMode = data.blendMode;
		batch.cullMode = data.cullMode;
		batch.renderLayer = data.renderLayer;
		batches_.push_back(batch);
		isInstanceDataDirty_ = true;
	}

	void CylinderEffectRenderer3d::Draw(MadoEngine::Render::RenderLayerMask layerMask) {
		if (!isInitialized_ || instances_.empty() || layerMask == 0) {
			return;
		}

		if (isInstanceDataDirty_) {
			EnsureInstanceCapacity(instances_.size());
			std::memcpy(
				mappedInstances_,
				instances_.data(),
				instances_.size() * sizeof(CylinderInstanceForGPU)
			);
			isInstanceDataDirty_ = false;
		}

		ID3D12RootSignature* rootSignature = MadoEngine::RootSignatureManager::GetInstance().Get("CylinderEffect3d.RootSig");
		assert(rootSignature);
		commandList_->SetGraphicsRootSignature(rootSignature);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->SetGraphicsRootDescriptorTable(
			kCylinderRootInstances,
			MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(instanceSrvIndex_)
		);
		commandList_->SetGraphicsRootConstantBufferView(
			kCylinderRootPerView,
			perViewResource_->GetGPUVirtualAddress()
		);

		for (const DrawBatch& batch : batches_) {
			if (!MadoEngine::Render::ContainsRenderLayer(layerMask, batch.renderLayer)) {
				continue;
			}

			SharedGeometry& geometry = ResolveGeometry(batch.geometryKey);
			const MadoEngine::Render::PSODesc psoDesc = CreatePSODesc(batch.blendMode, batch.cullMode);
			commandList_->SetPipelineState(psoRegistry_->Get(psoDesc));
			commandList_->IASetIndexBuffer(&geometry.indexBufferView);
			commandList_->SetGraphicsRootDescriptorTable(
				kCylinderRootTexture,
				MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(batch.textureIndex)
			);
			commandList_->DrawIndexedInstanced(
				geometry.indexCount,
				batch.instanceCount,
				0,
				0,
				batch.firstInstance
			);
		}
	}

	std::size_t CylinderEffectRenderer3d::GeometryKeyHash::operator()(const GeometryKey& key) const {
		const std::size_t radialHash = std::hash<uint32_t>{}(key.radialSegments);
		const std::size_t heightHash = std::hash<uint32_t>{}(key.heightSegments);
		return radialHash ^ (heightHash + 0x9e3779b9 + (radialHash << 6) + (radialHash >> 2));
	}

	void CylinderEffectRenderer3d::EnsureInstanceCapacity(std::size_t requiredCount) {
		if (instanceCapacity_ >= requiredCount) {
			return;
		}

		std::size_t newCapacity = (std::max)(kInitialCylinderInstanceCapacity, instanceCapacity_);
		while (newCapacity < requiredCount) {
			newCapacity *= 2;
		}

		if (mappedInstances_ && instanceResource_) {
			instanceResource_->Unmap(0, nullptr);
		}
		mappedInstances_ = CreateMappedBuffer<CylinderInstanceForGPU>(
			device_,
			instanceResource_,
			newCapacity,
			false
		);
		instanceCapacity_ = newCapacity;
		MadoEngine::Core::SRVManager::GetInstance().CreateStructuredBufferSRV(
			instanceResource_.Get(),
			instanceSrvIndex_,
			static_cast<uint32_t>(instanceCapacity_),
			sizeof(CylinderInstanceForGPU)
		);
	}

	CylinderEffectRenderer3d::SharedGeometry& CylinderEffectRenderer3d::ResolveGeometry(const GeometryKey& key) {
		if (const auto found = geometryCache_.find(key); found != geometryCache_.end()) {
			return found->second;
		}

		const uint32_t columns = key.radialSegments + 1;
		const uint32_t indexCount = key.radialSegments * key.heightSegments * 6;
		SharedGeometry geometry;
		uint32_t* indices = CreateMappedBuffer<uint32_t>(device_, geometry.indexResource, indexCount, false);
		uint32_t writeIndex = 0;
		for (uint32_t heightIndex = 0; heightIndex < key.heightSegments; ++heightIndex) {
			for (uint32_t radialIndex = 0; radialIndex < key.radialSegments; ++radialIndex) {
				const uint32_t bottomLeft = heightIndex * columns + radialIndex;
				const uint32_t bottomRight = bottomLeft + 1;
				const uint32_t topLeft = bottomLeft + columns;
				const uint32_t topRight = topLeft + 1;
				indices[writeIndex++] = topLeft;
				indices[writeIndex++] = topRight;
				indices[writeIndex++] = bottomLeft;
				indices[writeIndex++] = topRight;
				indices[writeIndex++] = bottomRight;
				indices[writeIndex++] = bottomLeft;
			}
		}
		geometry.indexResource->Unmap(0, nullptr);
		geometry.indexBufferView.BufferLocation = geometry.indexResource->GetGPUVirtualAddress();
		geometry.indexBufferView.SizeInBytes = sizeof(uint32_t) * indexCount;
		geometry.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		geometry.indexCount = indexCount;
		return geometryCache_.emplace(key, std::move(geometry)).first->second;
	}

	uint32_t CylinderEffectRenderer3d::ResolveTextureIndex(const std::string& textureName) {
		if (const auto found = textureIndexCache_.find(textureName); found != textureIndexCache_.end()) {
			return found->second;
		}

		uint32_t textureIndex = MadoEngine::TextureManager::GetInstance().GetTextureIndex(textureName);
		if (textureIndex == (std::numeric_limits<uint32_t>::max)()) {
			textureIndex = MadoEngine::TextureManager::GetInstance().GetTextureIndex("white2x2");
		}
		textureIndexCache_[textureName] = textureIndex;
		return textureIndex;
	}

	MadoEngine::Render::PSODesc CylinderEffectRenderer3d::CreatePSODesc(
		MadoEngine::Render::BlendMode blendMode,
		MadoEngine::Render::CullMode cullMode) const {
		MadoEngine::Render::PSODesc desc;
		desc.blendMode = blendMode;
		desc.depthMode = MadoEngine::Render::DepthMode::ReadOnly;
		desc.cullMode = cullMode;
		desc.fillMode = MadoEngine::Render::FillMode::Solid;
		desc.topology = MadoEngine::Render::TopologyType::Triangle;
		desc.inputLayout = MadoEngine::Render::InputLayoutType::None;
		desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.vsKey = "Object3d/PrimitiveEffect/Cylinder.VS";
		desc.psKey = "Object3d/PrimitiveEffect/Cylinder.PS";
		desc.rootSigKey = "CylinderEffect3d.RootSig";
		return desc;
	}

} // namespace MadoEngine::Effect
