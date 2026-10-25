#pragma once
#include "../IProjectile.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
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

		void OnEnemyHit() override;

	private:
		
		MadoEngine::ModelHandle model_{};
		MadoEngine::Ribbon::RibbonEffectHandle trail_{};
		std::string objectName_;
	};
}
