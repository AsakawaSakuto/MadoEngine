#include "DropObject.h"
#include <algorithm>

namespace DropObject {

	namespace {
		constexpr float kMoveSpeed = 12.0f;
		constexpr float kArriveDistance = 0.05f;
	}

	Base::~Base() {
		Release();
	}

	void Base::Initialize(Type type, const Vector3& position, int index) {
		type_ = type;
		transform_.translate = position;
		transform_.scale = { 0.25f, 0.25f, 0.25f };
		isMoving_ = false;
		isAlive_ = true;
		isReleased_ = false;
		colliderName_ = "DropObject" + std::to_string(index);
		modelName_ = "DropObject" + std::to_string(index);

		model_ = MyModel::Create(modelName_, "Plane", SceneType::Test);

		AABB aabb;

		std::string textureName;
		switch (type_) {
		case Type::Exp:
			textureName = "Exp";
			aabb.center = transform_.translate;
			aabb.min = { -0.125f, 0.0f, -0.125f };
			aabb.max = { 0.125f, 0.25f, 0.125f };
			break;
		case Type::Heal:
			textureName = "Heal";
			break;
		}

		colliderShape_ = aabb;
		MyCollider::RegisterCollider(
			colliderName_,
			CollisionTag::DropObjectHitBox,
			&colliderShape_,
			&transform_.translate,
			0.0f
		);

		if (model_) {
			model_->SetTexture(textureName);
			model_->SetUseBillboard(true);
			model_->SetCastShadow(false);
			model_->SetReceiveShadow(false);
			model_->SetLightingEnabled(false);
			model_->SetTransform(transform_);
		}
	}

	void Base::Update(float deltaTime) {
		if (!isAlive_) {
			return;
		}

		if (isMoving_) {
			Vector3 toTarget = targetPosition_ - transform_.translate;
			const float distance = toTarget.Length();
			if (distance <= kArriveDistance) {
				transform_.translate = targetPosition_;
			} else {
				const float moveDistance = std::min(kMoveSpeed * deltaTime, distance);
				transform_.translate += toTarget / distance * moveDistance;
			}
		}

		if (MyCollider::IsHitWithTag(colliderName_,	CollisionTag::PlayerDropObjectGetSphere)) {
			isMoving_ = true;
		}

		if (MyCollider::IsHitWithTag(colliderName_,	CollisionTag::PlayerHitBox)) {
			isAlive_ = false;
		}

		if (model_) {
			model_->SetPosition(transform_.translate);
			model_->SetScale(transform_.scale);
		}
	}

	void Base::Release() {
		if (isReleased_) {
			return;
		}

		if (!colliderName_.empty()) {
			MyCollider::RemoveCollider(colliderName_);
		}
		if (!modelName_.empty()) {
			MyModel::Destroy(modelName_);
			model_ = nullptr;
		}

		isReleased_ = true;
	}
}
