#include "ParticleRenderer3d.h"
#include "Core/TextureManager/TextureManager.h"
#include "Core/View/SRVManager.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/2d/Sprite/SpriteData.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

namespace {

	constexpr std::size_t kInitialParticleInstanceCapacity = 256;
	constexpr UINT kParticleRootInstances = 0;
	constexpr UINT kParticleRootPerView = 1;
	constexpr UINT kParticleRootTexture = 2;
	constexpr UINT kParticleRootFirstInstance = 3;
	constexpr UINT kGpuParticleRootInstances = 0;
	constexpr UINT kGpuParticleRootPerView = 1;
	constexpr UINT kGpuParticleRootTexture = 2;
	constexpr UINT kGpuParticleRootBatch = 3;
	constexpr UINT kGpuParticleRootAliveIndices = 4;

	/// @brief BlendModeをParticle Shader用の値へ変換する
	/// @param blendMode 変換するBlendMode
	/// @return Particle Shaderへ渡すBlendMode値
	uint32_t ToParticleShaderBlendMode(MadoEngine::Render::BlendMode blendMode) {
		switch (blendMode) {
			case MadoEngine::Render::BlendMode::Normal:
				return 0;
			case MadoEngine::Render::BlendMode::Add:
				return 1;
			case MadoEngine::Render::BlendMode::Subtract:
				return 2;
			case MadoEngine::Render::BlendMode::Multiply:
				return 3;
			case MadoEngine::Render::BlendMode::None:
				return 4;
		}

		return 0;
	}

} // namespace

namespace MadoEngine::Particle {

	ParticleRenderer3d::~ParticleRenderer3d() {
		Finalize();
	}

	void ParticleRenderer3d::Initialize(
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

		SpriteVertexData* vertices = CreateMappedBuffer<SpriteVertexData>(device_, vertexResource_, 4, false);
		vertices[0] = { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f } };
		vertices[1] = { { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f } };
		vertices[2] = { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f } };
		vertices[3] = { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f } };
		vertexResource_->Unmap(0, nullptr);
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
		vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

		uint32_t* indices = CreateMappedBuffer<uint32_t>(device_, indexResource_, 6, false);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 1;
		indices[4] = 3;
		indices[5] = 2;
		indexResource_->Unmap(0, nullptr);
		indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
		indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
		indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

		for (uint32_t index = 0; index < kFrameResourceCount; ++index) {
			PerViewFrameResource& frameResource = perViewFrameResources_[index];
			frameResource.mappedData = CreateMappedBuffer<PerViewForGPU>(
				device_,
				frameResource.resource,
				1,
				false
			);
			frameResource.instanceSrvIndex =
				MadoEngine::Core::SRVManager::GetInstance().Allocate();
			EnsureInstanceCapacity(index, kInitialParticleInstanceCapacity);
		}

		D3D12_INDIRECT_ARGUMENT_DESC indirectArgument{};
		indirectArgument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc{};
		commandSignatureDesc.ByteStride = sizeof(GpuParticleDrawArguments);
		commandSignatureDesc.NumArgumentDescs = 1;
		commandSignatureDesc.pArgumentDescs = &indirectArgument;
		const HRESULT commandSignatureResult = device_->CreateCommandSignature(
			&commandSignatureDesc,
			nullptr,
			IID_PPV_ARGS(&gpuDrawCommandSignature_)
		);
		assert(SUCCEEDED(commandSignatureResult));
		isInitialized_ = true;

		Logger::Output("ParticleRenderer3dを初期化しました。", Logger::Level::Engine);
	}

	void ParticleRenderer3d::Finalize() {
		for (PerViewFrameResource& frameResource : perViewFrameResources_) {
			if (frameResource.mappedData && frameResource.resource) {
				frameResource.resource->Unmap(0, nullptr);
			}
			if (frameResource.mappedInstances && frameResource.instanceResource) {
				frameResource.instanceResource->Unmap(0, nullptr);
			}
			if (
				frameResource.instanceSrvIndex !=
				(std::numeric_limits<uint32_t>::max)()) {
				MadoEngine::Core::SRVManager::GetInstance().Free(
					frameResource.instanceSrvIndex
				);
			}
			frameResource.mappedData = nullptr;
			frameResource.mappedInstances = nullptr;
			frameResource.resource.Reset();
			frameResource.instanceResource.Reset();
			frameResource.instanceSrvIndex =
				(std::numeric_limits<uint32_t>::max)();
			frameResource.instanceCapacity = 0;
			frameResource.fenceValue = 0;
		}

		vertexResource_.Reset();
		indexResource_.Reset();
		gpuDrawCommandSignature_.Reset();
		instances_.clear();
		batches_.clear();
		gpuBatches_.clear();
		missingTextureNames_.clear();
		currentPerViewFrameIndex_ = 0;
		nextPerViewFrameIndex_ = 0;
		completedFenceValue_ = 0;
		isInstanceDataDirty_ = false;
		hasCurrentPerViewFrame_ = false;
		device_ = nullptr;
		commandList_ = nullptr;
		psoRegistry_ = nullptr;
		isInitialized_ = false;
	}

	void ParticleRenderer3d::Begin(
		const Camera& camera,
		uint64_t submissionFenceValue) {
		if (!isInitialized_) {
			return;
		}

		instances_.clear();
		batches_.clear();
		gpuBatches_.clear();
		isInstanceDataDirty_ = true;
		cameraPosition_ = camera.GetPosition();
		hasCurrentPerViewFrame_ = false;
		for (uint32_t offset = 0; offset < kFrameResourceCount; ++offset) {
			const uint32_t candidate =
				(nextPerViewFrameIndex_ + offset) % kFrameResourceCount;
			if (perViewFrameResources_[candidate].fenceValue > completedFenceValue_) {
				continue;
			}

			currentPerViewFrameIndex_ = candidate;
			nextPerViewFrameIndex_ = (candidate + 1) % kFrameResourceCount;
			perViewFrameResources_[candidate].fenceValue = submissionFenceValue;
			hasCurrentPerViewFrame_ = true;
			break;
		}
		if (!hasCurrentPerViewFrame_) {
			return;
		}

		const Matrix4x4 inverseView = Matrix::Inverse(camera.GetViewMatrix());
		PerViewForGPU* mappedPerView =
			perViewFrameResources_[currentPerViewFrameIndex_].mappedData;
		mappedPerView->viewProjection = camera.GetViewProjectionMatrix();
		mappedPerView->cameraRight = {
			inverseView.m[0][0],
			inverseView.m[0][1],
			inverseView.m[0][2],
			0.0f,
		};
		mappedPerView->cameraUp = {
			inverseView.m[1][0],
			inverseView.m[1][1],
			inverseView.m[1][2],
			0.0f,
		};
		mappedPerView->fog = fogParameters_;
	}

	void ParticleRenderer3d::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		completedFenceValue_ = (std::max)(completedFenceValue_, completedFenceValue);
	}

	void ParticleRenderer3d::Submit(
		std::span<const ParticleState> particles,
		const EmitterConfig& config,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) {
		if (!isInitialized_ || particles.empty()) {
			return;
		}

		const uint32_t textureIndex = ResolveTextureIndex(config.renderer.textureName);
		if (textureIndex == (std::numeric_limits<uint32_t>::max)()) {
			return;
		}

		struct SortableParticleInstance {
			ParticleInstanceForGPU instance;
			float distanceSquared = 0.0f;
		};

		const Matrix4x4 emitterMatrix = Matrix::MakeAffine(
			emitterTransform.scale,
			emitterTransform.rotate,
			emitterTransform.translate
		);
		std::vector<SortableParticleInstance> submitted;
		submitted.reserve(particles.size());

		for (const ParticleState& particle : particles) {
			SortableParticleInstance sortable;
			sortable.instance.position = particle.position;
			sortable.instance.rotation = particle.rotation;
			sortable.instance.scale = particle.scale;
			sortable.instance.color = particle.color;

			if (config.simulationSpace == SimulationSpace::Local) {
				sortable.instance.position = Matrix::Transform(particle.position, emitterMatrix);
				sortable.instance.rotation += emitterTransform.rotate.z;
				sortable.instance.scale.x *= std::abs(emitterTransform.scale.x);
				sortable.instance.scale.y *= std::abs(emitterTransform.scale.y);
			}

			const Vector3 toCamera = sortable.instance.position - cameraPosition_;
			sortable.distanceSquared = toCamera.LengthSq();
			submitted.push_back(sortable);
		}

		if (config.renderer.sortMode == SortMode::BackToFront) {
			std::sort(submitted.begin(), submitted.end(), [](const SortableParticleInstance& lhs, const SortableParticleInstance& rhs) {
				return lhs.distanceSquared > rhs.distanceSquared;
			});
		}

		DrawBatch batch;
		batch.firstInstance = static_cast<uint32_t>(instances_.size());
		batch.instanceCount = static_cast<uint32_t>(submitted.size());
		batch.textureIndex = textureIndex;
		batch.blendMode = config.renderer.blendMode;
		batch.renderLayer = renderLayer;
		for (const SortableParticleInstance& sortable : submitted) {
			instances_.push_back(sortable.instance);
		}
		batches_.push_back(batch);
		isInstanceDataDirty_ = true;
	}

	void ParticleRenderer3d::SubmitGpu(
		const GpuParticleRenderData& renderData,
		const EmitterConfig& config,
		MadoEngine::Render::RenderLayer renderLayer) {
		if (
			!isInitialized_ ||
			renderData.drawInstanceBufferAddress == 0 ||
			renderData.aliveIndexBufferAddress == 0 ||
			!renderData.indirectArgumentBuffer) {
			return;
		}

		const uint32_t textureIndex = ResolveTextureIndex(config.renderer.textureName);
		if (textureIndex == (std::numeric_limits<uint32_t>::max)()) {
			return;
		}

		GpuDrawBatch batch;
		batch.renderData = renderData;
		batch.textureIndex = textureIndex;
		batch.blendMode = config.renderer.blendMode;
		batch.renderLayer = renderLayer;
		gpuBatches_.push_back(batch);
	}

	void ParticleRenderer3d::Draw(MadoEngine::Render::RenderLayerMask layerMask) {
		if (
			!isInitialized_ ||
			!hasCurrentPerViewFrame_ ||
			(instances_.empty() && gpuBatches_.empty()) ||
			layerMask == 0) {
			return;
		}

		if (isInstanceDataDirty_ && !instances_.empty()) {
			EnsureInstanceCapacity(
				currentPerViewFrameIndex_,
				instances_.size()
			);
			PerViewFrameResource& frameResource =
				perViewFrameResources_[currentPerViewFrameIndex_];
			std::memcpy(
				frameResource.mappedInstances,
				instances_.data(),
				instances_.size() * sizeof(ParticleInstanceForGPU)
			);
			isInstanceDataDirty_ = false;
		}

		commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
		commandList_->IASetIndexBuffer(&indexBufferView_);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (!batches_.empty()) {
			ID3D12RootSignature* rootSignature =
				MadoEngine::RootSignatureManager::GetInstance().Get("Particle3d.RootSig");
			assert(rootSignature);
			commandList_->SetGraphicsRootSignature(rootSignature);
			commandList_->SetGraphicsRootDescriptorTable(
				kParticleRootInstances,
				MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(
					perViewFrameResources_[currentPerViewFrameIndex_].instanceSrvIndex
				)
			);
			commandList_->SetGraphicsRootConstantBufferView(
				kParticleRootPerView,
				perViewFrameResources_[currentPerViewFrameIndex_].resource->GetGPUVirtualAddress()
			);

			for (const DrawBatch& batch : batches_) {
				if (
					batch.instanceCount == 0 ||
					!MadoEngine::Render::ContainsRenderLayer(layerMask, batch.renderLayer)) {
					continue;
				}

				const MadoEngine::Render::PSODesc psoDesc = CreatePSODesc(batch.blendMode);
				commandList_->SetPipelineState(psoRegistry_->Get(psoDesc));
				commandList_->SetGraphicsRootDescriptorTable(
					kParticleRootTexture,
					MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(batch.textureIndex)
				);
				const uint32_t perBatchConstants[] = {
					batch.firstInstance,
					ToParticleShaderBlendMode(batch.blendMode),
				};
				commandList_->SetGraphicsRoot32BitConstants(
					kParticleRootFirstInstance,
					_countof(perBatchConstants),
					perBatchConstants,
					0
				);
				commandList_->DrawIndexedInstanced(
					6,
					batch.instanceCount,
					0,
					0,
					0
				);
			}
		}

		if (!gpuBatches_.empty()) {
			ID3D12RootSignature* rootSignature =
				MadoEngine::RootSignatureManager::GetInstance().Get("GpuParticle3d.RootSig");
			assert(rootSignature);
			assert(gpuDrawCommandSignature_);
			commandList_->SetGraphicsRootSignature(rootSignature);
			commandList_->SetGraphicsRootConstantBufferView(
				kGpuParticleRootPerView,
				perViewFrameResources_[currentPerViewFrameIndex_].resource->GetGPUVirtualAddress()
			);

			for (const GpuDrawBatch& batch : gpuBatches_) {
				if (!MadoEngine::Render::ContainsRenderLayer(layerMask, batch.renderLayer)) {
					continue;
				}

				const MadoEngine::Render::PSODesc psoDesc =
					CreateGpuPSODesc(batch.blendMode);
				ID3D12PipelineState* pipelineState = psoRegistry_->Get(psoDesc);
				if (!pipelineState) {
					continue;
				}
				commandList_->SetPipelineState(pipelineState);
				commandList_->SetGraphicsRootShaderResourceView(
					kGpuParticleRootInstances,
					batch.renderData.drawInstanceBufferAddress
				);
				commandList_->SetGraphicsRootDescriptorTable(
					kGpuParticleRootTexture,
					MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(batch.textureIndex)
				);
				const uint32_t perBatchConstants[] = {
					0,
					ToParticleShaderBlendMode(batch.blendMode),
				};
				commandList_->SetGraphicsRoot32BitConstants(
					kGpuParticleRootBatch,
					_countof(perBatchConstants),
					perBatchConstants,
					0
				);
				commandList_->SetGraphicsRootShaderResourceView(
					kGpuParticleRootAliveIndices,
					batch.renderData.aliveIndexBufferAddress
				);
				commandList_->ExecuteIndirect(
					gpuDrawCommandSignature_.Get(),
					1,
					batch.renderData.indirectArgumentBuffer,
					0,
					nullptr,
					0
				);
			}
		}
	}

	void ParticleRenderer3d::SetFogParameters(const ParticleFogParameters& parameters) {
		fogParameters_ = parameters;
	}

	bool ParticleRenderer3d::IsGpuRenderingAvailable() const {
		if (
			!isInitialized_ ||
			!psoRegistry_ ||
			!gpuDrawCommandSignature_ ||
			!MadoEngine::RootSignatureManager::GetInstance().Get("GpuParticle3d.RootSig")) {
			return false;
		}

		const MadoEngine::Render::BlendMode blendModes[] = {
			MadoEngine::Render::BlendMode::Normal,
			MadoEngine::Render::BlendMode::Add,
			MadoEngine::Render::BlendMode::Subtract,
			MadoEngine::Render::BlendMode::Multiply,
			MadoEngine::Render::BlendMode::None,
		};
		for (const MadoEngine::Render::BlendMode blendMode : blendModes) {
			if (!psoRegistry_->Get(CreateGpuPSODesc(blendMode))) {
				return false;
			}
		}
		return true;
	}

	void ParticleRenderer3d::EnsureInstanceCapacity(
		uint32_t frameResourceIndex,
		std::size_t requiredCount) {
		assert(frameResourceIndex < kFrameResourceCount);
		PerViewFrameResource& frameResource =
			perViewFrameResources_[frameResourceIndex];
		if (frameResource.instanceCapacity >= requiredCount) {
			return;
		}

		std::size_t newCapacity = (std::max)(
			kInitialParticleInstanceCapacity,
			frameResource.instanceCapacity
		);
		while (newCapacity < requiredCount) {
			newCapacity *= 2;
		}

		if (frameResource.mappedInstances && frameResource.instanceResource) {
			frameResource.instanceResource->Unmap(0, nullptr);
		}
		frameResource.mappedInstances =
			CreateMappedBuffer<ParticleInstanceForGPU>(
				device_,
				frameResource.instanceResource,
				newCapacity,
				false
			);
		frameResource.instanceCapacity = newCapacity;
		MadoEngine::Core::SRVManager::GetInstance().CreateStructuredBufferSRV(
			frameResource.instanceResource.Get(),
			frameResource.instanceSrvIndex,
			static_cast<uint32_t>(frameResource.instanceCapacity),
			sizeof(ParticleInstanceForGPU)
		);
	}

	uint32_t ParticleRenderer3d::ResolveTextureIndex(const std::string& textureName) {
		uint32_t textureIndex = (std::numeric_limits<uint32_t>::max)();
		if (!MadoEngine::TextureManager::GetInstance().TryGetTextureIndex(textureName, textureIndex)) {
			if (missingTextureNames_.insert(textureName).second) {
				Logger::Output(
					"Particleのテクスチャが見つからないため描画をスキップします: " + textureName,
					Logger::Level::Warning
				);
			}
			return (std::numeric_limits<uint32_t>::max)();
		}

		missingTextureNames_.erase(textureName);
		return textureIndex;
	}

	MadoEngine::Render::PSODesc ParticleRenderer3d::CreatePSODesc(
		MadoEngine::Render::BlendMode blendMode) const {
		MadoEngine::Render::PSODesc desc;
		desc.blendMode = blendMode;
		desc.depthMode = MadoEngine::Render::DepthMode::ReadOnly;
		desc.cullMode = MadoEngine::Render::CullMode::None;
		desc.fillMode = MadoEngine::Render::FillMode::Solid;
		desc.topology = MadoEngine::Render::TopologyType::Triangle;
		desc.inputLayout = MadoEngine::Render::InputLayoutType::Sprite;
		desc.preserveRenderTargetAlpha = true;
		desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
		desc.vsKey = "Object3d/Particle/Particle.VS";
		desc.psKey = "Object3d/Particle/Particle.PS";
		desc.rootSigKey = "Particle3d.RootSig";
		return desc;
	}

	MadoEngine::Render::PSODesc ParticleRenderer3d::CreateGpuPSODesc(
		MadoEngine::Render::BlendMode blendMode) const {
		MadoEngine::Render::PSODesc desc = CreatePSODesc(blendMode);
		desc.vsKey = "Object3d/Particle/GpuParticle.VS";
		desc.rootSigKey = "GpuParticle3d.RootSig";
		return desc;
	}

} // namespace MadoEngine::Particle
