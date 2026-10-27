#pragma once
#include "../IProjectile.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
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

		/// @brief Enemy命中時にランダムダメージと即死抽選を適用
		void OnEnemyHit() override;

	private:
		static constexpr float kBaseSize = 0.5f;
		static constexpr float kMinSizeRate = 0.1f;
		static constexpr float kMinLifeTime = 0.1f;
		static constexpr int kMinRandomDamageBonus = 1;
		static constexpr int kMaxRandomDamageBonus = 13;
		static constexpr int kInstantKillRollMin = 1;
		static constexpr int kInstantKillRollMax = 100;
		static constexpr int kInstantKillRollSuccess = 5; // 1%の確率で即死

		MadoEngine::ModelHandle model_{};
		MadoEngine::Ribbon::RibbonEffectHandle trail_{};
		std::string objectName_;
		float baseDamage_ = 0.0f;
	};
}
