#pragma once
#include "IParticleEmitterRuntime.h"
#include "ParticleEffectAsset.h"
#include "ParticleEmitterRuntimeFactory.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace MadoEngine::Ribbon {

	class RibbonEffectRenderer3d;

}

namespace MadoEngine::Particle {

	class ParticleRenderer3d;

	/// @brief 1つのEmitterの再生状態とCPU Simulatorを保持する
	class ParticleEmitterInstance final {
	public:
		/// @brief Emitter再生状態を初期化
		/// @param config 参照するEmitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @param loopOverride Loop設定の上書き値
		/// @param emitterTransform Emitterの初期Transform
		void Initialize(
			const EmitterConfig& config,
			uint32_t randomSeed,
			const std::optional<bool>& loopOverride,
			const Transform3D& emitterTransform,
			const std::optional<ParticleBackend>& backendOverride,
			const ParticleEmitterRuntimeFactory& runtimeFactory
		);

		/// @brief Emitterと生存Particleを更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param emitterTransform EmitterのTransform
		void Update(float deltaTime, const Transform3D& emitterTransform);

		/// @brief Emitterを停止
		/// @param mode 停止方法
		void Stop(StopMode mode);

		/// @brief RuntimeへEmitter Transformの変更を通知
		/// @param emitterTransform 最新のEmitter Transform
		void SetTransform(const Transform3D& emitterTransform);

		/// @brief Emitterと全Particleを初期状態へ復元
		void Reset();

		/// @brief GPU Simulation Commandを記録
		/// @param commandList 記録可能なDirect CommandList
		/// @param submissionFenceValue Command提出へ紐付けるFence値
		void RecordGpuSimulation(
			ID3D12GraphicsCommandList* commandList,
			uint64_t submissionFenceValue
		);

		/// @brief 記録済みGPU処理の完了後処理
		/// @param completedFenceValue GPUが完了済みのFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue);

		/// @brief 再生が完了したか確認
		/// @return 発生停止済みかつ生存Particleが0の場合はtrue
		bool IsFinished() const;

		/// @brief 生存ParticleをRendererへ登録
		/// @param renderer 登録先Renderer
		/// @param trailRenderer Trail登録先Ribbon Renderer
		/// @param emitterTransform EmitterのTransform
		/// @param renderLayer 描画先Layer
		void SubmitRenderData(
			ParticleRenderer3d& renderer,
			MadoEngine::Ribbon::RibbonEffectRenderer3d& trailRenderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const;

		/// @brief 生存Particle数を取得
		/// @return 生存Particle数
		uint32_t GetAliveCount() const {
			return runtime_ ? runtime_->GetAliveCount() : 0;
		}

		/// @brief Runtime情報を取得
		/// @return Editor表示用Runtime情報
		ParticleEmitterRuntimeInfo GetRuntimeInfo() const;

	private:
		/// @brief 指定時間区間に含まれるBurstを発生
		/// @param previousLocalTime 前フレームのEmitterローカル時間
		/// @param currentLocalTime 現在のEmitterローカル時間
		/// @param emitterTransform EmitterのTransform
		void EmitBursts(
			float previousLocalTime,
			float currentLocalTime,
			const Transform3D& emitterTransform
		);

		std::optional<EmitterConfig> config_;
		std::unique_ptr<IParticleEmitterRuntime> runtime_;
		ParticleBackend requestedBackend_ = ParticleBackend::Auto;
		std::string fallbackReason_;
		float playbackTime_ = 0.0f;
		float spawnAccumulator_ = 0.0f;
		bool isEmitting_ = true;
		bool isLoop_ = false;
		bool hasProcessedEmission_ = false;
	};

	/// @brief Particle Effect Assetの1回分の再生状態
	class ParticleEffectInstance final {
	public:
		/// @brief Effect Instanceを初期化
		/// @param asset 再生するParticle Effect Asset
		/// @param desc 再生設定
		void Initialize(
			std::shared_ptr<const ParticleEffectAsset> asset,
			const PlayDesc& desc,
			const ParticleEmitterRuntimeFactory& runtimeFactory
		);

		/// @brief 全Emitterを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 全GPU EmitterのSimulation Commandを記録
		/// @param commandList 記録可能なDirect CommandList
		/// @param submissionFenceValue Command提出へ紐付けるFence値
		void RecordGpuSimulation(
			ID3D12GraphicsCommandList* commandList,
			uint64_t submissionFenceValue
		);

		/// @brief 全GPU EmitterへGPU完了を通知
		/// @param completedFenceValue GPUが完了済みのFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue);

		/// @brief Effectを停止
		/// @param mode 停止方法
		void Stop(StopMode mode);

		/// @brief Effectの時間進行を一時停止
		void Pause();

		/// @brief Effectの時間進行を再開
		void Resume();

		/// @brief Effectの再生速度を設定
		/// @param playbackSpeed 設定する再生速度
		/// @return 有効な再生速度を設定できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed);

		/// @brief Effectが一時停止中か確認
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return isPaused_;
		}

		/// @brief Effectの再生が完了したか確認
		/// @return 全Emitterの再生が完了している場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認
		/// @param sceneType 現在のScene種別
		/// @param layerMask 描画対象LayerMask
		/// @return 描画条件に一致する場合はtrue
		bool Matches(SceneType sceneType, MadoEngine::Render::RenderLayerMask layerMask) const;

		/// @brief 全Emitterの生存ParticleをRendererへ登録
		/// @param renderer 登録先Renderer
		/// @param trailRenderer Trail登録先Ribbon Renderer
		void SubmitRenderData(
			ParticleRenderer3d& renderer,
			MadoEngine::Ribbon::RibbonEffectRenderer3d& trailRenderer
		) const;

		/// @brief EffectのTransformを設定
		/// @param transform 設定するTransform
		void SetTransform(const Transform3D& transform);

		/// @brief EffectのTransformを取得
		/// @return EffectのTransform
		const Transform3D& GetTransform() const { return transform_; }

		/// @brief Effectが属するSceneを取得
		/// @return Effectが属するScene
		SceneType GetSceneType() const { return sceneType_; }

		/// @brief Effectが属する描画Layerを取得
		/// @return Effectが属する描画Layer
		MadoEngine::Render::RenderLayer GetRenderLayer() const { return renderLayer_; }

		/// @brief Effect内の生存Particle総数を取得
		/// @return 生存Particle総数
		std::size_t GetAliveCount() const;

		/// @brief 全EmitterのRuntime情報を取得
		/// @return Editor表示用Runtime情報
		std::vector<ParticleEmitterRuntimeInfo> GetRuntimeInfo() const;

	private:
		std::shared_ptr<const ParticleEffectAsset> asset_;
		std::vector<ParticleEmitterInstance> emitters_;
		Transform3D transform_;
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		float playbackSpeed_ = 1.0f;
		bool isPaused_ = false;
	};

} // namespace MadoEngine::Particle
