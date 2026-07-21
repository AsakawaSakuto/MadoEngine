#pragma once
#include "CpuParticleSimulator.h"
#include "ParticleEffectAsset.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace MadoEngine::Particle {

	class ParticleRenderer3d;

	/// @brief 1つのEmitterの再生状態とCPU Simulatorを保持する
	class ParticleEmitterInstance final {
	public:
		/// @brief Emitter再生状態を初期化する
		/// @param config 参照するEmitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @param loopOverride Loop設定の上書き値
		/// @param emitterTransform Emitterの初期Transform
		void Initialize(
			const EmitterConfig& config,
			uint32_t randomSeed,
			const std::optional<bool>& loopOverride,
			const Transform3D& emitterTransform
		);

		/// @brief Emitterと生存Particleを更新する
		/// @param deltaTime 前フレームからの経過時間
		/// @param emitterTransform EmitterのTransform
		void Update(float deltaTime, const Transform3D& emitterTransform);

		/// @brief Emitterを停止する
		/// @param mode 停止方法
		void Stop(StopMode mode);

		/// @brief Emitterと全Particleを初期状態へ戻す
		void Reset();

		/// @brief 再生が完了したか確認する
		/// @return 発生停止済みかつ生存Particleが0の場合はtrue
		bool IsFinished() const;

		/// @brief 生存ParticleをRendererへ登録する
		/// @param renderer 登録先Renderer
		/// @param emitterTransform EmitterのTransform
		/// @param renderLayer 描画先Layer
		void SubmitRenderData(
			ParticleRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const;

		/// @brief 生存Particle数を取得する
		/// @return 生存Particle数
		uint32_t GetAliveCount() const { return simulator_.GetAliveCount(); }

	private:
		/// @brief 指定時間区間に含まれるBurstを発生させる
		/// @param previousLocalTime 前フレームのEmitterローカル時間
		/// @param currentLocalTime 現在のEmitterローカル時間
		/// @param emitterTransform EmitterのTransform
		void EmitBursts(
			float previousLocalTime,
			float currentLocalTime,
			const Transform3D& emitterTransform
		);

		std::optional<EmitterConfig> config_;
		CpuParticleSimulator simulator_;
		Random random_;
		float playbackTime_ = 0.0f;
		float spawnAccumulator_ = 0.0f;
		bool isEmitting_ = true;
		bool isLoop_ = false;
		bool hasProcessedEmission_ = false;
	};

	/// @brief Particle Effect Assetの1回分の再生状態
	class ParticleEffectInstance final {
	public:
		/// @brief Effect Instanceを初期化する
		/// @param asset 再生するParticle Effect Asset
		/// @param desc 再生設定
		void Initialize(std::shared_ptr<const ParticleEffectAsset> asset, const PlayDesc& desc);

		/// @brief 全Emitterを更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Effectを停止する
		/// @param mode 停止方法
		void Stop(StopMode mode);

		/// @brief Effectの再生が完了したか確認する
		/// @return 全Emitterの再生が完了している場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認する
		/// @param sceneType 現在のScene種別
		/// @param layerMask 描画対象LayerMask
		/// @return 描画条件に一致する場合はtrue
		bool Matches(SceneType sceneType, MadoEngine::Render::RenderLayerMask layerMask) const;

		/// @brief 全Emitterの生存ParticleをRendererへ登録する
		/// @param renderer 登録先Renderer
		void SubmitRenderData(ParticleRenderer3d& renderer) const;

		/// @brief EffectのTransformを設定する
		/// @param transform 設定するTransform
		void SetTransform(const Transform3D& transform) { transform_ = transform; }

		/// @brief EffectのTransformを取得する
		/// @return EffectのTransform
		const Transform3D& GetTransform() const { return transform_; }

		/// @brief Effectが属するSceneを取得する
		/// @return Effectが属するScene
		SceneType GetSceneType() const { return sceneType_; }

		/// @brief Effectが属する描画Layerを取得する
		/// @return Effectが属する描画Layer
		MadoEngine::Render::RenderLayer GetRenderLayer() const { return renderLayer_; }

		/// @brief Effect内の生存Particle総数を取得する
		/// @return 生存Particle総数
		std::size_t GetAliveCount() const;

	private:
		std::shared_ptr<const ParticleEffectAsset> asset_;
		std::vector<ParticleEmitterInstance> emitters_;
		Transform3D transform_;
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
	};

} // namespace MadoEngine::Particle
