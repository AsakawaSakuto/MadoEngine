#pragma once
#include "DropObject.h"
#include <memory>
#include <vector>

namespace DropObject {
	
	class Manager {
	public:

		static Manager& GetInstance();

		/// @brief DropObjectを更新します
		/// @param deltaTime 1フレームの経過時間です
		/// @param targetPosition DropObjectが向かう座標です
		void Update(float deltaTime, const Vector3& targetPosition);

		/// @brief DropObjectを生成します
		/// @param type DropObjectの種類です
		/// @param position 生成位置です
		void Spawn(Type type, const Vector3& position);

		/// @brief すべてのDropObjectを削除します
		void Clear();

	private:
		std::vector<std::unique_ptr<Base>> dropObjects_;

		int spawnCount_ = 0;
	};
}
