#include "DropObjectManager.h"

namespace DropObject {
	
	void Manager::Update(float deltaTime, const Vector3& targetPosition) {
		for (auto& dropObject : dropObjects_) {
			dropObject->SetTargetPosition(targetPosition);
			dropObject->Update(deltaTime);
		}
	}

	void Manager::Spawn(Type type, const Vector3& position) {
		auto newDropObject = std::make_unique<Base>();
		newDropObject->Initialize(type, position);
		dropObjects_.push_back(std::move(newDropObject));
	}
}