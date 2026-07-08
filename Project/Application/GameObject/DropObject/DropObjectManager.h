#pragma once
#include "DropObject.h"

namespace DropObject {
	
	class Manager {
	public:

		void Update(float deltaTime, const Vector3& targetPosition);

		void Spawn(Type type, const Vector3& position);

	private:
		std::vector<std::unique_ptr<Base>> dropObjects_;
	};
}