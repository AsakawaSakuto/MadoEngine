#include "DropObject.h"

namespace DropObject {

	void Base::Initialize(Type type, const Vector3& position, int index) {
		type_ = type;
		transform_.translate = position;
		transform_.scale = { 0.25f, 0.25f, 0.25f };
		isActive_ = false;

		model_ = MyModel::Create("DropObject" + std::to_string(index), "Plane", SceneType::Test);

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
		MyCollider::RegisterCollider("DropObject" + std::to_string(index), CollisionTag::DropObjectHitBox, &colliderShape_, &transform_.translate, 0.0f);

		model_->SetTexture(textureName);
		model_->SetUseBillboard(true);
		model_->SetCastShadow(false);
		model_->SetReceiveShadow(false);
		model_->SetLightingEnabled(false);
		model_->SetTransform(transform_);
	}

	void Base::Update(float deltaTime) {
		if (!isActive_) {
			return;
		}
	}
}