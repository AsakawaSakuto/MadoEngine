#pragma once
#include "ParticleTypes.h"
#include <cstdint>

struct ID3D12GraphicsCommandList;

namespace MadoEngine::Particle {

	class ParticleRenderer3d;

	/// @brief Emitter単位のParticle Simulation Backend境界
	class IParticleEmitterRuntime {
	public:
		virtual ~IParticleEmitterRuntime() = default;

		/// @brief Runtimeを初期化
		/// @param config Emitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @return 初期化に成功した場合はtrue
		virtual bool Initialize(const EmitterConfig& config, uint32_t randomSeed) = 0;

		/// @brief Particle状態を1フレーム分更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param emitterTransform 現在のEmitter Transform
		virtual void Update(float deltaTime, const Transform3D& emitterTransform) = 0;

		/// @brief Particle発生を要求
		/// @param count 発生数
		/// @param emitterTransform 発生時のEmitter Transform
		virtual void Emit(uint32_t count, const Transform3D& emitterTransform) = 0;

		/// @brief Emitter Transformの変更をRuntimeへ通知
		/// @param emitterTransform 最新のEmitter Transform
		virtual void SetTransform(const Transform3D& emitterTransform) = 0;

		/// @brief Particleを停止
		/// @param mode 停止方式
		virtual void Stop(StopMode mode) = 0;

		/// @brief Runtimeを初期状態へ復元
		virtual void Reset() = 0;

		/// @brief GPU Simulation Commandを記録
		/// @param commandList 記録可能なDirect CommandList
		/// @param submissionFenceValue Command提出へ紐付けるFence値
		virtual void RecordGpuSimulation(
			ID3D12GraphicsCommandList* commandList,
			uint64_t submissionFenceValue
		) = 0;

		/// @brief 記録済みGPU処理の完了後処理
		/// @param completedFenceValue GPUが完了済みのFence値
		virtual void OnGpuFrameCompleted(uint64_t completedFenceValue) = 0;

		/// @brief 描画データをRendererへ登録
		/// @param renderer 登録先Renderer
		/// @param emitterTransform 現在のEmitter Transform
		/// @param renderLayer 描画Layer
		virtual void SubmitRenderData(
			ParticleRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const = 0;

		/// @brief 生存Particleと未記録要求が存在しないか確認
		/// @return Runtimeが空の場合はtrue
		virtual bool IsIdle() const = 0;

		/// @brief 現在のBackendを取得
		/// @return 使用中Backend
		virtual ParticleBackend GetBackend() const = 0;

		/// @brief 遅延を許容した生存Particle数を取得
		/// @return 生存Particle数
		virtual uint32_t GetAliveCount() const = 0;

		/// @brief 最大Particle数を取得
		/// @return 最大Particle数
		virtual uint32_t GetMaxParticleCount() const = 0;

		/// @brief GPU Buffer容量を取得
		/// @return GPU Buffer総容量
		virtual uint64_t GetGpuBufferCapacityBytes() const = 0;
	};

} // MadoEngine::Particle名前空間
