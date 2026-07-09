#pragma once
#include "DropObject.h"
#include <memory>
#include <vector>

namespace DropObject {
	
	class Manager {
	public:

		static Manager& GetInstance();

		/// @brief DropObjectを更新します
		/// @param deltaTime 1フレームの経過時間
		/// @param targetPosition DropObjectが向かう座標
		void Update(float deltaTime, const Vector3& targetPosition);

		/// @brief DropObjectを生成します
		/// @param type DropObjectの種類
		/// @param position 生成位置
		void Spawn(Type type, const Vector3& position);

		/// @brief すべてのDropObjectを削除します
		void Clear();

	private:
		std::vector<std::unique_ptr<Base>> dropObjects_;

		int spawnCount_ = 0;
	};
}
