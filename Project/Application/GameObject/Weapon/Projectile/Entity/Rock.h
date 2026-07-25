#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Rock : public IProjectile {
	public:
		/// @brief Rockのデストラクタ
		~Rock() override;

		/// @brief Rockを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Rockを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Enemy命中時にダメージを2倍にする
		void OnEnemyHit() override;

	private:
		static constexpr float kHitDamageMultiplier = 2.0f; // 命中時のダメージ倍率

		Model* model_ = nullptr;
		std::string objectName_;
	};
}
