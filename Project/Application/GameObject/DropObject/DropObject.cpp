#include "DropObject.h"
#include "GameObject/Player/Player.h"
#include <algorithm>

namespace DropObject {

	namespace {
		constexpr float kMoveSpeed = 12.0f;
		constexpr float kArriveDistance = 0.05f;
		constexpr int kExpAmount = 10;
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

		model_ = MyModel::Create(modelName_, "Plane", SceneType::Game);

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

		// 回収判定とPlayer本体への到達判定を分離するため専用Tagで登録
		MyCollider::RegisterCollider(
			colliderName_,
			CollisionTag::DropObjectHitBox,
			&colliderShape_,
			&transform_.translate,
			0.0f
		);

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture(textureName);
			model->SetUseBillboard(true);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
			model->SetTransform(transform_);
		}
	}

	void Base::Update(float deltaTime) {
		UpdateInternal(deltaTime, nullptr);
	}

	void Base::Update(float deltaTime, Player::Base& player) {
		UpdateInternal(deltaTime, &player);
	}

	void Base::UpdateInternal(float deltaTime, Player::Base* player) {

		// 回収済みObjectのModelとColliderをManager削除まで更新対象外に設定
		if (!isAlive_) {
			return;
		}

		Vector3 targetPosition = player->GetPosition();

		if (isMoving_) {

			// 回収開始直後は一度Playerから離して吸着軌道に溜めを作成
			if (backTimer_.IsActive()) {
				Vector3 toTarget = targetPosition - transform_.translate;
				const float distance = toTarget.Length();
				if (distance <= kArriveDistance) {
					transform_.translate = targetPosition;
				} else {
					const float moveDistance = std::min(kMoveSpeed * deltaTime, distance);
					transform_.translate -= toTarget / distance * moveDistance;
				}
			} else {
				Vector3 toTarget = targetPosition - transform_.translate;
				const float distance = toTarget.Length();
				if (distance <= kArriveDistance) {
					transform_.translate = targetPosition;
				} else {
					const float moveDistance = std::min(kMoveSpeed * deltaTime, distance);
					transform_.translate += toTarget / distance * moveDistance;
				}
			}
		}

		// 広い回収範囲への侵入を契機にPlayerへの吸着を開始
		if (MyCollider::IsHitWithTag(colliderName_,	CollisionTag::PlayerDropObjectGetSphere)) {
			if (!isMoving_) {
				isMoving_ = true;
				backTimer_.Start(0.2f);
			}
		}

		// Player本体へ到達した時点で報酬を確定して生存状態を終了
		if (MyCollider::IsHitWithTag(colliderName_,	CollisionTag::PlayerHitBox)) {
			if (player && type_ == Type::Exp) {
				CollectExp(*player);
				return;
			}

			isAlive_ = false;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetPosition(transform_.translate);
			model->SetScale(transform_.scale);
		}

		backTimer_.Update(deltaTime);
	}

	void Base::CollectExp(Player::Base& player) {
		if (!isAlive_) {
			return;
		}

		player.AddExp(kExpAmount);
		isAlive_ = false;
	}

	void Base::Release() {

		// Destructorと明示解放の重複呼び出しからリソースを保護
		if (isReleased_) {
			return;
		}

		if (!colliderName_.empty()) {
			MyCollider::RemoveCollider(colliderName_);
		}
		if (!modelName_.empty()) {
			MyModel::RequestDestroy(model_);
			model_ = {};
		}

		isReleased_ = true;
	}
}
