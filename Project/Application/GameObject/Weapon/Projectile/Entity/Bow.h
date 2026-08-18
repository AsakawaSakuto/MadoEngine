#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Bow : public IProjectile {
	public:
		/// @brief Bowのデストラクタ
		~Bow() override;

		/// @brief Bowを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Bowを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Enemy命中時のダメージを2倍化
		void OnEnemyHit() override;

	private:
		static constexpr float kHitDamageMultiplier = 2.0f; // 命中時のダメージ倍率

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
	};
}
