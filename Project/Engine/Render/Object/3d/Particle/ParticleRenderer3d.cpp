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

		mappedPerView_ = CreateMappedBuffer<PerViewForGPU>(device_, perViewResource_, 1, false);
		instanceSrvIndex_ = MadoEngine::Core::SRVManager::GetInstance().Allocate();
		EnsureInstanceCapacity(kInitialParticleInstanceCapacity);
		isInitialized_ = true;

		Logger::Output("ParticleRenderer3dを初期化しました。", Logger::Level::Engine);
	}

	void ParticleRenderer3d::Finalize() {
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

		vertexResource_.Reset();
		indexResource_.Reset();
		instanceResource_.Reset();
		perViewResource_.Reset();
		instances_.clear();
		batches_.clear();
		missingTextureNames_.clear();
		instanceCapacity_ = 0;
		isInstanceDataDirty_ = false;
		device_ = nullptr;
		commandList_ = nullptr;
		psoRegistry_ = nullptr;
		isInitialized_ = false;
	}

	void ParticleRenderer3d::Begin(const Camera& camera) {
		if (!isInitialized_) {
			return;
		}

		instances_.clear();
		batches_.clear();
		isInstanceDataDirty_ = true;
		cameraPosition_ = camera.GetPosition();

		const Matrix4x4 inverseView = Matrix::Inverse(camera.GetViewMatrix());
		mappedPerView_->viewProjection = camera.GetViewProjectionMatrix();
		mappedPerView_->cameraRight = {
			inverseView.m[0][0],
			inverseView.m[0][1],
			inverseView.m[0][2],
			0.0f,
		};
		mappedPerView_->cameraUp = {
			inverseView.m[1][0],
			inverseView.m[1][1],
			inverseView.m[1][2],
			0.0f,
		};
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

	void ParticleRenderer3d::Draw(MadoEngine::Render::RenderLayerMask layerMask) {
		if (!isInitialized_ || instances_.empty() || layerMask == 0) {
			return;
		}

		if (isInstanceDataDirty_) {
			EnsureInstanceCapacity(instances_.size());
			std::memcpy(
				mappedInstances_,
				instances_.data(),
				instances_.size() * sizeof(ParticleInstanceForGPU)
			);
			isInstanceDataDirty_ = false;
		}

		ID3D12RootSignature* rootSignature = MadoEngine::RootSignatureManager::GetInstance().Get("Particle3d.RootSig");
		assert(rootSignature);
		commandList_->SetGraphicsRootSignature(rootSignature);
		commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
		commandList_->IASetIndexBuffer(&indexBufferView_);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->SetGraphicsRootDescriptorTable(
			kParticleRootInstances,
			MadoEngine::Core::SRVManager::GetInstance().GetGPUHandle(instanceSrvIndex_)
		);
		commandList_->SetGraphicsRootConstantBufferView(
			kParticleRootPerView,
			perViewResource_->GetGPUVirtualAddress()
		);

		for (const DrawBatch& batch : batches_) {
			if (batch.instanceCount == 0 || !MadoEngine::Render::ContainsRenderLayer(layerMask, batch.renderLayer)) {
				continue;
			}

			const MadoEngine::Render::PSODesc psoDesc = CreatePSODesc(batch.blendMode);
			commandList_->SetPipelineState(psoRegistry_->Get(psoDesc));
			commandList_->SetGraphicsRootDescriptorTable(
				kParticleRootTexture,
				MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(batch.textureIndex)
			);
			commandList_->DrawIndexedInstanced(
				6,
				batch.instanceCount,
				0,
				0,
				batch.firstInstance
			);
		}
	}

	void ParticleRenderer3d::EnsureInstanceCapacity(std::size_t requiredCount) {
		if (instanceCapacity_ >= requiredCount) {
			return;
		}

		std::size_t newCapacity = (std::max)(kInitialParticleInstanceCapacity, instanceCapacity_);
		while (newCapacity < requiredCount) {
			newCapacity *= 2;
		}

		if (mappedInstances_ && instanceResource_) {
			instanceResource_->Unmap(0, nullptr);
		}
		mappedInstances_ = CreateMappedBuffer<ParticleInstanceForGPU>(
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
		desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.vsKey = "Object3d/Particle/Particle.VS";
		desc.psKey = "Object3d/Particle/Particle.PS";
		desc.rootSigKey = "Particle3d.RootSig";
		return desc;
	}

} // namespace MadoEngine::Particle
