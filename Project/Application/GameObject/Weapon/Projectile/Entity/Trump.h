#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	/// @brief 目標地点へ直進するProjectile
	class Trump : public IProjectile {
	public:
		/// @brief Trumpのデストラクタ
		~Trump() override;

		/// @brief Trumpを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Trumpを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Enemy命中時に1から13のランダムダメージを加算
		void OnEnemyHit() override;

	private:
		static constexpr float kBaseSize = 0.5f;
		static constexpr float kMinSizeRate = 0.1f;
		static constexpr float kMinLifeTime = 0.1f;
		static constexpr int kMinRandomDamageBonus = 1;
		static constexpr int kMaxRandomDamageBonus = 13;

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		float baseDamage_ = 0.0f;
	};
}
