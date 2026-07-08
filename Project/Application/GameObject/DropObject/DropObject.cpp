#include "DropObject.h"

namespace DropObject {

	void Base::Initialize(Type type, const Vector3& position) {
		type_ = type;
		transform_.translate = position;
		isActive_ = false;

		model_ = MyModel::Create("DropObject", "Plane", SceneType::Test);

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
	}

	void Base::Update(float deltaTime) {
		if (!isActive_) {
			return;
		}
	}
}