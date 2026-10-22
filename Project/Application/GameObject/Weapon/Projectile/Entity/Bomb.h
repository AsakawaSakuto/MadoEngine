#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	/// @brief 上向きの放物線を描いて目標地点へ飛ぶProjectile
	class Bomb : public IProjectile {
	public:
		/// @brief Bombのデストラクタ
		~Bomb() override;

		/// @brief Bombを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Bombを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 消滅確定時に現在座標へ爆発を生成
		void OnDestroy() override;

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
