#pragma once
#include "DropObject.h"
#include <memory>
#include <vector>

namespace Player {
	class Base;
}

namespace DropObject {
	
	class Manager {
	public:

		static Manager& GetInstance();

		/// @brief DropObjectを更新
		/// @param deltaTime 1フレームの経過時間
		/// @param player 経験値を受け取るPlayer
		void Update(float deltaTime, Player::Base& player);

		/// @brief DropObjectを生成
		/// @param type DropObjectの種類
		/// @param position 生成位置
		void Spawn(Type type, const Vector3& position);

		/// @brief すべてのDropObjectを削除
		void Clear();

	private:
		std::vector<std::unique_ptr<Base>> dropObjects_;

		int spawnCount_ = 0;
	};
}
