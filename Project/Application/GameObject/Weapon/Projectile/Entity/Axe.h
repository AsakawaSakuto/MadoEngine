#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Axe : public IProjectile {
	public:
		/// @brief Axeのデストラクタ
		~Axe() override;

		/// @brief Axeを初期化する
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Axeを更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		Model* model_ = nullptr;
		Vector3 moveDirection_ = { 0.0f, 0.0f, 0.0f };
		std::string objectName_;
	};
}