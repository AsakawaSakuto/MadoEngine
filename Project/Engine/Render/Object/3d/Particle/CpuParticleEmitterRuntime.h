#pragma once
#include "CpuParticleSimulator.h"
#include "IParticleEmitterRuntime.h"
#include "Utility/Random.h"

namespace MadoEngine::Particle {

	/// @brief 既存CPU Simulatorを使用するEmitter Runtime
	class CpuParticleEmitterRuntime final : public IParticleEmitterRuntime {
	public:
		/// @brief Runtimeを初期化する
		/// @param config Emitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @return 初期化に成功した場合はtrue
		bool Initialize(const EmitterConfig& config, uint32_t randomSeed) override;

		/// @brief Particle状態を1フレーム分更新する
		/// @param deltaTime 前フレームからの経過時間
		/// @param emitterTransform 現在のEmitter Transform
		void Update(float deltaTime, const Transform3D& emitterTransform) override;

		/// @brief Particleを発生させる
		/// @param count 発生数
		/// @param emitterTransform 発生時のEmitter Transform
		void Emit(uint32_t count, const Transform3D& emitterTransform) override;

		/// @brief CPU Backendでは描画登録時に最新Transformを使用する
		/// @param emitterTransform 最新のEmitter Transform
		void SetTransform(const Transform3D& emitterTransform) override;

		/// @brief Particleを停止する
		/// @param mode 停止方式
		void Stop(StopMode mode) override;

		/// @brief Runtimeを初期状態へ戻す
		void Reset() override;

		/// @brief CPU BackendではGPU Commandを記録しない
		/// @param commandList 未使用
		/// @param submissionFenceValue 未使用
		void RecordGpuSimulation(
			ID3D12GraphicsCommandList* commandList,
			uint64_t submissionFenceValue
		) override;

		/// @brief CPU BackendではGPU完了処理を行わない
		/// @param completedFenceValue 未使用
		void OnGpuFrameCompleted(uint64_t completedFenceValue) override;

		/// @brief CPU ParticleをRendererへ登録する
		/// @param renderer 登録先Renderer
		/// @param emitterTransform 現在のEmitter Transform
		/// @param renderLayer 描画Layer
		void SubmitRenderData(
			ParticleRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const override;

		/// @brief 生存Particleが存在しないか確認する
		/// @return 生存Particleが存在しない場合はtrue
		bool IsIdle() const override;

		/// @brief 現在のBackendを取得する
		/// @return CPU Backend
		ParticleBackend GetBackend() const override { return ParticleBackend::Cpu; }

		/// @brief 生存Particle数を取得する
		/// @return 生存Particle数
		uint32_t GetAliveCount() const override { return simulator_.GetAliveCount(); }

		/// @brief 最大Particle数を取得する
		/// @return 最大Particle数
		uint32_t GetMaxParticleCount() const override { return config_.emission.maxParticles; }

		/// @brief CPU BackendのGPU Buffer容量を取得する
		/// @return 常に0
		uint64_t GetGpuBufferCapacityBytes() const override { return 0; }

	private:
		EmitterConfig config_;
		CpuParticleSimulator simulator_;
		Random random_;
	};

} // MadoEngine::Particle名前空間
