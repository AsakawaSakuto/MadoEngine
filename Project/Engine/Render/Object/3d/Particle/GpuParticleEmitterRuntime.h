#pragma once
#include "GpuParticleTypes.h"
#include "IParticleEmitterRuntime.h"
#include "ParticleTrailHistory.h"
#include "Render/PSO/ComputePSORegistry.h"
#include <array>
#include <cstdint>
#include <vector>
#include <wrl/client.h>

namespace MadoEngine::Particle {

	/// @brief Compute ShaderでParticleを更新するEmitter Runtime
	class GpuParticleEmitterRuntime final : public IParticleEmitterRuntime {
	public:
		/// @brief GPU Runtimeへ必要な共有基盤を設定
		/// @param device D3D12 Device
		/// @param computePsoRegistry Compute PSO Registry
		GpuParticleEmitterRuntime(
			ID3D12Device* device,
			MadoEngine::Render::ComputePSORegistry* computePsoRegistry
		);

		~GpuParticleEmitterRuntime() override;

		/// @brief Runtimeを初期化
		/// @param config Emitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @return 初期化に成功した場合はtrue
		bool Initialize(const EmitterConfig& config, uint32_t randomSeed) override;

		/// @brief GPU更新要求を蓄積
		/// @param deltaTime 前フレームからの経過時間
		/// @param emitterTransform 現在のEmitter Transform
		void Update(float deltaTime, const Transform3D& emitterTransform) override;

		/// @brief GPU発生要求を蓄積
		/// @param count 発生数
		/// @param emitterTransform 発生時のEmitter Transform
		void Emit(uint32_t count, const Transform3D& emitterTransform) override;

		/// @brief Local Particleの描画へ最新Transformを反映
		/// @param emitterTransform 最新のEmitter Transform
		void SetTransform(const Transform3D& emitterTransform) override;

		/// @brief Particleを停止
		/// @param mode 停止方式
		void Stop(StopMode mode) override;

		/// @brief GPU Runtimeを初期状態へ復元
		void Reset() override;

		/// @brief GPU Simulation Commandを記録
		/// @param commandList 記録可能なDirect CommandList
		/// @param submissionFenceValue Command提出へ紐付けるFence値
		void RecordGpuSimulation(
			ID3D12GraphicsCommandList* commandList,
			uint64_t submissionFenceValue
		) override;

		/// @brief 非同期ReadbackをGPU完了後に反映
		/// @param completedFenceValue GPUが完了済みのFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue) override;

		/// @brief GPU描画データをRendererへ登録
		/// @param renderer 登録先Renderer
		/// @param emitterTransform 現在のEmitter Transform
		/// @param renderLayer 描画Layer
		void SubmitRenderData(
			ParticleRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const override;

		/// @brief GPU ParticleからReadbackしたTrailをRibbon Rendererへ登録
		/// @param renderer 登録先Ribbon Renderer
		/// @param emitterTransform 現在のEmitter Transform
		/// @param renderLayer 描画Layer
		void SubmitTrailRenderData(
			MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const override;

		/// @brief 生存Particleと未完了GPU処理が存在しないか確認
		/// @return Runtimeが空の場合はtrue
		bool IsIdle() const override;

		/// @brief 現在のBackendを取得
		/// @return GPU Backend
		ParticleBackend GetBackend() const override { return ParticleBackend::Gpu; }

		/// @brief 遅延Readback済みの生存Particle数を取得
		/// @return 生存Particle数
		uint32_t GetAliveCount() const override { return cachedAliveCount_; }

		/// @brief 最大Particle数を取得
		/// @return 最大Particle数
		uint32_t GetMaxParticleCount() const override { return config_.emission.maxParticles; }

		/// @brief GPU Buffer容量を取得
		/// @return GPU Buffer総容量
		uint64_t GetGpuBufferCapacityBytes() const override { return gpuBufferCapacityBytes_; }

	private:
		static constexpr uint32_t kFrameResourceCount = 3;

		struct BufferResource {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		};

		struct ReadbackSlot {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			bool isPending = false;
			uint64_t sequence = 0;
			uint64_t fenceValue = 0;
		};

		/// @brief Particle設定をGPU用PODへ変換
		/// @param config 変換元Emitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @return GPU用Emitter設定
		static GpuParticleEmitterParameters MakeEmitterParameters(
			const EmitterConfig& config,
			uint32_t randomSeed
		);

		/// @brief Default Heap Bufferを生成
		/// @param sizeInBytes Buffer Size
		/// @param initialState 初期Resource State
		/// @param outBuffer 生成先
		/// @return 生成に成功した場合はtrue
		bool CreateDefaultBuffer(
			uint64_t sizeInBytes,
			D3D12_RESOURCE_STATES initialState,
			BufferResource& outBuffer
		);

		/// @brief Upload Heap Bufferを生成してMap
		/// @param sizeInBytes Buffer Size
		/// @param outResource 生成先Resource
		/// @param outMappedData Map先Pointer
		/// @return 生成に成功した場合はtrue
		bool CreateUploadBuffer(
			uint64_t sizeInBytes,
			Microsoft::WRL::ComPtr<ID3D12Resource>& outResource,
			void** outMappedData
		);

		/// @brief Readback Heap Bufferを生成
		/// @param sizeInBytes Buffer Size
		/// @param outResource 生成先Resource
		/// @return 生成に成功した場合はtrue
		bool CreateReadbackBuffer(
			uint64_t sizeInBytes,
			Microsoft::WRL::ComPtr<ID3D12Resource>& outResource
		);

		/// @brief Resource Stateを必要な場合だけ遷移
		/// @param commandList CommandList
		/// @param buffer 対象Buffer
		/// @param nextState 遷移先State
		void Transition(
			ID3D12GraphicsCommandList* commandList,
			BufferResource& buffer,
			D3D12_RESOURCE_STATES nextState
		);

		/// @brief UAV Barrierを記録
		/// @param commandList CommandList
		/// @param resource 対象Resource、nullptrの場合は全UAV
		void AddUavBarrier(
			ID3D12GraphicsCommandList* commandList,
			ID3D12Resource* resource = nullptr
		) const;

		/// @brief Compute Root Parameterへ全Bufferを設定
		/// @param commandList CommandList
		/// @param inputAliveIndex 読み込みAlive Buffer Index
		/// @param outputAliveIndex 書き込みAlive Buffer Index
		/// @param perFrameAddress PerFrame Constant Buffer Address
		void BindComputeResources(
			ID3D12GraphicsCommandList* commandList,
			uint32_t inputAliveIndex,
			uint32_t outputAliveIndex,
			D3D12_GPU_VIRTUAL_ADDRESS perFrameAddress
		);

		/// @brief GPU Bufferを描画参照可能なStateへ遷移
		/// @param commandList CommandList
		void TransitionForDraw(ID3D12GraphicsCommandList* commandList);

		/// @brief GPU Buffer容量を集計
		void CalculateGpuBufferCapacity();

		ID3D12Device* device_ = nullptr;
		MadoEngine::Render::ComputePSORegistry* computePsoRegistry_ = nullptr;
		EmitterConfig config_;

		BufferResource stateBuffer_;
		BufferResource drawInstanceBuffer_;
		std::array<BufferResource, 2> aliveIndexBuffers_;
		std::array<BufferResource, 2> aliveCounterBuffers_;
		BufferResource freeIndexBuffer_;
		BufferResource freeCounterBuffer_;
		BufferResource indirectArgumentBuffer_;
		BufferResource trailSampleBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> emitterParameterBuffer_;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kFrameResourceCount> perFrameBuffers_;
		std::array<GpuParticlePerFrameParameters*, kFrameResourceCount> mappedPerFrameParameters_{};
		std::array<ReadbackSlot, kFrameResourceCount> readbackSlots_;
		ParticleTrailHistory trailHistory_;
		std::vector<ParticleTrailSample> trailSamples_;

		Transform3D pendingEmitterTransform_;
		float pendingDeltaTime_ = 0.0f;
		uint32_t pendingEmitCount_ = 0;
		uint32_t nextEmitSequence_ = 0;
		uint32_t currentAliveIndex_ = 0;
		uint32_t nextFrameResourceIndex_ = 0;
		uint32_t cachedAliveCount_ = 0;
		uint64_t gpuBufferCapacityBytes_ = 0;
		uint64_t nextReadbackSequence_ = 1;
		uint64_t lastAppliedReadbackSequence_ = 0;
		uint64_t lastAppliedTrailReadbackSequence_ = 0;
		uint64_t aliveIndexReadbackOffset_ = 0;
		uint64_t trailSampleReadbackOffset_ = 0;
		uint64_t readbackBufferSize_ = sizeof(uint32_t);
		bool isInitialized_ = false;
		bool needsGpuInitialize_ = true;
		bool hasPendingUpdate_ = false;
		bool hasGpuWorkInFlight_ = false;
		bool hasCompletedSimulation_ = false;
		bool suppressDraw_ = false;
	};

} // MadoEngine::Particle名前空間
