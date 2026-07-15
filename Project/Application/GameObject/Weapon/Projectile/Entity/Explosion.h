#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Explosion : public IProjectile {
	public:
		/// @brief Explosionのデストラクタ
		~Explosion() override;

		/// @brief Explosionを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Explotionを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		std::string objectName_;
	};
}