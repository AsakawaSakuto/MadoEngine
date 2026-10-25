#pragma once
#include "../IProjectile.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
#include <string>

namespace Projectile {

	class Pistol : public IProjectile {
	public:
		/// @brief Pistolのデストラクタ
		~Pistol() override;

		/// @brief Pistolを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Pistolを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		MadoEngine::ModelHandle model_{};
		MadoEngine::Ribbon::RibbonEffectHandle trail_{};
		std::string objectName_;
	};
}
