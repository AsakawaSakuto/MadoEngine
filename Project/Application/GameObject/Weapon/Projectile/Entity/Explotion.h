#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Explotion : public IProjectile {
	public:
		/// @brief Explotionのデストラクタ
		~Explotion() override;

		/// @brief Explotionを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Explotionを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		float deadTime_ = 0.0f;
		float deadTimeLimit_ = 0.5f;
		std::string objectName_;
	};
}