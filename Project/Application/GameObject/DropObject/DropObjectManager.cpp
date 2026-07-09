#include "DropObjectManager.h"
#include "GameObject/Player/Player.h"
#include <algorithm>

namespace DropObject {

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Update(float deltaTime, Player::Base& player) {
		const Vector3 targetPosition = player.GetPosition();

		for (auto& dropObject : dropObjects_) {
			dropObject->SetTargetPosition(targetPosition);
			dropObject->Update(deltaTime, player);
		}

		dropObjects_.erase(
			std::remove_if(
				dropObjects_.begin(),
				dropObjects_.end(),
				[](const std::unique_ptr<Base>& dropObject) {
					return !dropObject || !dropObject->IsAlive();
				}),
			dropObjects_.end());
	}

	void Manager::Spawn(Type type, const Vector3& position) {
		auto newDropObject = std::make_unique<Base>();
		newDropObject->Initialize(type, position, spawnCount_);
		dropObjects_.push_back(std::move(newDropObject));
		spawnCount_++;
	}

	void Manager::Clear() {
		dropObjects_.clear();
		spawnCount_ = 0;
	}
}
