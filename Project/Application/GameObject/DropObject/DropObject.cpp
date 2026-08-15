#include "DropObject.h"
#include "GameObject/Player/Player.h"
#include <algorithm>

namespace DropObject {

	namespace {
		constexpr float kMoveSpeed = 20.0f;      // DropObjectのPlayerへの吸着速度
		constexpr float kArriveDistance = 0.05f; // DropObjectがPlayerに到達したと判定する距離
		constexpr float kBackDuration = 0.2f;    // DropObjectがPlayerから後退する予備動作の継続時間

		constexpr int kExpAmount = 10;           // DropObjectのExpがPlayerに加算する経験値量

		constexpr float kMoneyRiseSpeed = 8.0f;  // DropObjectのMoneyが後退中に上昇する速度
		constexpr int kMoneyAmount = 1;          // DropObjectのMoneyがPlayerに加算する所持金量
	}

	Base::~Base() {
		Release();
	}

	void Base::Initialize(Type type, const Vector3& position, int index) {
		type_ = type;
		transform_.translate = position;
		transform_.scale = { 0.2f, 0.2f, 0.2f };
		isMoving_ = type_ == Type::Money;
		if (type_ == Type::Money) {

			// 生成直後のMoneyに上昇しながら後退する予備動作を付与
			backTimer_.Start(kBackDuration);
		}
		isAlive_ = true;
		isReleased_ = false;
		colliderName_ = "DropObject" + std::to_string(index);
		modelName_ = "DropObject" + std::to_string(index);

		model_ = MyModel::Create(modelName_, "Plane", SceneType::Game);

		AABB aabb;
		aabb.center = transform_.translate;
		aabb.min = { -0.125f, 0.0f, -0.125f };
		aabb.max = { 0.125f, 0.25f, 0.125f };

		std::string textureName;
		switch (type_) {
		case Type::Exp:
			textureName = "Exp";
			break;
		case Type::Money:
			textureName = "Coin";
			transform_.scale = { 0.1f, 0.1f, 0.1f };
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

		const Vector3 targetPosition = player ? player->GetPosition() : targetPosition_;

		if (isMoving_) {

			// 回収開始直後は一度Playerから離して吸着軌道に溜めを作成
			if (backTimer_.IsActive()) {
				Vector3 toTarget = targetPosition - transform_.translate;
				const float distance = toTarget.Length();
				if (distance > kArriveDistance) {
					const float moveDistance = std::min(kMoveSpeed * deltaTime, distance);
					transform_.translate -= toTarget / distance * moveDistance;
				}

				if (type_ == Type::Money) {

					// 後退中だけ高さを加えて追尾開始前の跳ね上がりを表現
					transform_.translate.y += kMoneyRiseSpeed * deltaTime;
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
		if (MyCollider::IsHitWithTag(colliderName_, CollisionTag::PlayerDropObjectGetSphere)) {
			if (!isMoving_) {
				isMoving_ = true;
				backTimer_.Start(kBackDuration);
			}
		}

		// Player本体へ到達した時点で報酬を確定して生存状態を終了
		const bool isMoneySpawnMotionActive = type_ == Type::Money && backTimer_.IsActive();
		if (!isMoneySpawnMotionActive && MyCollider::IsHitWithTag(colliderName_, CollisionTag::PlayerHitBox)) {
			if (player) {
				Collect(*player);
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

	void Base::Collect(Player::Base& player) {
		if (!isAlive_) {
			return;
		}

		// 報酬の加算先をDropObjectの種類ごとに分離
		switch (type_) {
		case Type::Exp:
			player.AddExp(kExpAmount);
			break;
		case Type::Money:
			player.AddMoney(kMoneyAmount);
			break;
		case Type::Heal:
			break;
		}

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
