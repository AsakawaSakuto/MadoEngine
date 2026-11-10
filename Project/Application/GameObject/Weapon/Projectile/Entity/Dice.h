#pragma once
#include "../IProjectile.h"

namespace Projectile {

	/// @brief 敵との接触地点で範囲攻撃を生成するProjectile
	class Dice : public IProjectile {
	public:
		/// @brief Diceのデストラクタ
		~Dice() override;

		/// @brief Diceの初期化と追加ダメージ・即死の抽選
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Diceの移動と寿命の更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Enemyとの接触地点へ爆発を生成
		void OnEnemyHit() override;

		/// @brief Dice本体の接触でダメージを与えるか確認
		/// @return 接触したEnemyへ本体の攻撃情報を渡す場合はtrue
		bool DealsContactDamage() const override { return false; }

	private:
		static constexpr float kModelScale = 0.3f;
		static constexpr float kHitRadius = 0.5f;
		static constexpr float kRotationSpeed = 8.0f;
		static constexpr float kExplosionDamageRate = 1.0f;
		static constexpr int kMinRandomDamageBonus = 1;
		static constexpr int kMaxRandomDamageBonus = 12;

		MadoEngine::ModelHandle model_{};
	};
}
