#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	/// @brief 上向きの放物線を描いて目標地点へ飛ぶProjectile
	class Orb : public IProjectile {
	public:
		/// @brief Orbのデストラクタ
		~Orb() override;

		/// @brief Orbを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Orbを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:
		static constexpr float kBaseModelScale = 0.5f;
		static constexpr float kBaseHitboxRadius = 0.5f;
		static constexpr float kMinSizeRate = 0.1f;
		static constexpr float kMinMoveSpeed = 0.01f;
		static constexpr float kMinFlightDuration = 0.1f;
		static constexpr float kArcHeightDistanceRate = 0.25f;
		static constexpr float kMinArcHeight = 2.0f;
		static constexpr float kMaxArcHeight = 8.0f;
		static constexpr float kArrivalHoldDuration = 0.1f;
		static constexpr float kRotationSpeed = 3.14f;

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		Vector3 startPosition_ = {};
		float flightDuration_ = kMinFlightDuration;
		float flightElapsedTime_ = 0.0f;
		float arcHeight_ = kMinArcHeight;
		float arrivalElapsedTime_ = 0.0f;
		bool hasArrived_ = false;
	};
}
