#pragma once
#include "../IGameObject.h"
#include "DropObjectStatus.h"

namespace DropObject {
	
	class Base : public IGameObject {
	public:
		
		void Initialize(Type type, const Vector3& position, int index);

		void Update(float deltaTime) override;

		void SetActive(bool isActive) { isActive_ = isActive; }

		void SetTargetPosition(const Vector3& targetPosition) { targetPosition_ = targetPosition; }
	private:

		Type type_ = Type::Exp;
		bool isActive_ = false;

		Vector3 targetPosition_ = { 0.0f, 0.0f, 0.0f };
	};
}