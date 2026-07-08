#include "DropObject.h"

namespace DropObject {

	void Base::Initialize(Type type, const Vector3& position, int index) {
		type_ = type;
		transform_.translate = position;
		transform_.scale = { 0.25f, 0.25f, 0.25f };
		isActive_ = false;

		model_ = MyModel::Create("DropObject" + std::to_string(index), "Plane", SceneType::Test);

		std::string textureName;
		switch (type_) {
		case Type::Exp:
			textureName = "Exp";
			break;
		case Type::Heal:
			textureName = "Heal";
			break;
		}

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