#pragma once
#include "ParticleTypes.h"
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace MadoEngine::Ribbon {

	class RibbonEffectRenderer3d;

}

namespace MadoEngine::Particle {

	/// @brief Particle Trail履歴へ反映する粒子Sample
	struct ParticleTrailSample {
		uint64_t identifier = 0;
		Vector3 position{};
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	/// @brief Backendに依存せずParticle Trail履歴を管理するクラス
	class ParticleTrailHistory final {
	public:
		/// @brief Trail設定とSimulation空間を初期化
		/// @param trail Trail設定
		/// @param simulationSpace ParticleのSimulation空間
		void Initialize(
			const ParticleTrailModule& trail,
			SimulationSpace simulationSpace
		);

		/// @brief Trail Pointの経過時間を更新
		/// @param deltaTime 前Frameからの経過時間
		void Advance(float deltaTime);

		/// @brief 生存Particle SampleからTrail先端と生存状態を更新
		/// @param samples 生存Particle Sample一覧
		void UpdateParticles(std::span<const ParticleTrailSample> samples);

		/// @brief Trail履歴を消去
		void Clear();

		/// @brief Trail描画データをRibbon Rendererへ登録
		/// @param renderer 登録先Ribbon Renderer
		/// @param emitterTransform 現在のEmitter Transform
		/// @param renderLayer 描画Layer
		void SubmitRenderData(
			MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		) const;

		/// @brief Trail履歴が空か確認
		/// @return Trail履歴が空の場合はtrue
		bool IsEmpty() const { return trails_.empty(); }

	private:
		struct TrailPoint {
			Vector3 position{};
			float age = 0.0f;
			Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct TrailState {
			std::vector<TrailPoint> points;
			Vector3 latestPosition{};
			Vector4 latestColor = { 1.0f, 1.0f, 1.0f, 1.0f };
			bool hasLatestPosition = false;
			bool wasParticleAlive = false;
			bool isParticleAlive = false;
		};

		/// @brief 最小間隔を満たすTrail Pointを追加
		/// @param state 追加対象Trail状態
		/// @param position 追加候補位置
		/// @param color Point生成時のParticle色
		void TryAddPoint(TrailState& state, const Vector3& position, const Vector4& color);

		ParticleTrailModule config_;
		SimulationSpace simulationSpace_ = SimulationSpace::Local;
		std::unordered_map<uint64_t, TrailState> trails_;
	};

} // namespace MadoEngine::Particle
