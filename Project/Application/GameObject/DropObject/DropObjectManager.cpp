#include "DropObjectManager.h"

namespace DropObject {

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Update(float deltaTime, const Vector3& targetPosition) {
		for (auto& dropObject : dropObjects_) {
			dropObject->SetTargetPosition(targetPosition);
			dropObject->Update(deltaTime);
		}
	}

	void Manager::Spawn(Type type, const Vector3& position) {
		auto newDropObject = std::make_unique<Base>();
		newDropObject->Initialize(type, position, spawnCount_);
		dropObjects_.push_back(std::move(newDropObject));
		spawnCount_++;
	}
}