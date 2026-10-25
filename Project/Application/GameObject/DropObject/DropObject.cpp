#include "DropObject.h"
#include "Audio/MyAudio.h"
#include "GameObject/Player/Player.h"
#include <algorithm>

namespace DropObject {

	namespace {
		constexpr float kMoveSpeed = 20.0f;      // DropObjectのPlayerへの吸着速度
		constexpr float kArriveDistance = 0.05f; // DropObjectがPlayerに到達したと判定する距離
		constexpr float kBackDuration = 0.2f;    // DropObjectがPlayerから後退する予備動作の継続時間

		constexpr int kExpAmount = 10;           // DropObjectのExpがPlayerに加算する経験値量
		constexpr int kExpAnimationFrameCount = 4;
		constexpr float kExpAnimationFrameDuration = 0.15f; // Expシートの1コマの表示時間（秒）

		constexpr float kMoneyRiseSpeed = 8.0f;  // DropObjectのMoneyが後退中に上昇する速度
		constexpr int kMoneyAmount = 1;          // DropObjectのMoneyがPlayerに加算する所持金量
		constexpr float kHealAmount = 10.0f;     // DropObjectのHealがPlayerに加算するHP量
	}

	Base::~Base() {
		Release();
	}

	void Base::Initialize(Type type, const Vector3& position, int index) {
		type_ = type;
		isReleased_ = false;
		colliderName_ = "DropObject" + std::to_string(index);
		modelName_ = "DropObject" + std::to_string(index);

		model_ = MyModel::Create(modelName_, "Plane", SceneType::Game);

		AABB aabb;
		aabb.center = position;
		aabb.min = { -0.125f, 0.0f, -0.125f };
		aabb.max = { 0.125f, 0.25f, 0.125f };

		std::string textureName;
		switch (type_) {
		case Type::Exp:
			textureName = "ExpSheet";
			break;
		case Type::Money:
			textureName = "Coin";
			break;
		case Type::Heal:
			textureName = "Heal";
			break;
		}

		colliderShape_ = aabb;

		// 生存状態を参照して未使用Pool枠を除外し、配置ごとのCollider再登録を回避
		MyCollider::RegisterCollider(
			colliderName_,
			CollisionTag::DropObjectHitBox,
			&colliderShape_,
			&transform_.translate,
			0.0f,
			nullptr,
			&isAlive_
		);

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture(textureName);

			// 再生設定だけを渡し、コマ送りとUV範囲の管理はModelへ委譲
			if (type_ == Type::Exp) {
				MadoEngine::SpriteSheetAnimationSettings settings;
				settings.columns = kExpAnimationFrameCount;
				settings.rows = 1;
				settings.frameDuration = kExpAnimationFrameDuration;
				settings.loop = true;
				model->SetSpriteSheetAnimation(settings);
			}

			// Cameraの俯仰角とRollを含む回転で画面への正対を維持
			model->SetUseBillboard(true);
			model->SetBillboardYAxisOnly(false);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}
		Activate(position);
	}

	void Base::Activate(const Vector3& position) {

		// 上書き前の吸着状態と予備動作を持ち越さず、新しいDropとして配置
		transform_.translate = position;
		transform_.rotate = {};
		transform_.scale = { 0.2f, 0.2f, 0.2f };
		targetPosition_ = position;
		backTimer_.Reset();
		collectedRewardAmount_ = 0;
		isMoving_ = type_ == Type::Money;
		isAlive_ = true;
		if (type_ == Type::Money) {

			// Moneyだけは配置直後から上昇付きの吸着予備動作を開始
			transform_.scale = { 0.1f, 0.1f, 0.1f };
			backTimer_.Start(kBackDuration);
		}

		std::get<AABB>(colliderShape_).center = position;
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);

			// Poolの再利用直後にUV範囲と先頭コマを復元し、前のDropの再生位相の持ち越しを防止
			model->SetUVTransform({});
			if (type_ == Type::Exp) {
				model->PlaySpriteSheetAnimation();
			}
			model->SetVisible(true);
		}
	}

	void Base::Deactivate() {

		// Colliderが参照する生存状態とModel表示を同時に停止し、リソースは次の配置まで保持
		isAlive_ = false;
		isMoving_ = false;
		backTimer_.Reset();
		if (Model* model = MyModel::TryGet(model_)) {
			model->StopSpriteSheetAnimation();
			model->SetVisible(false);
		}
	}

	void Base::AttractTo(const Vector3& targetPosition) {
		if (!isAlive_) {
			return;
		}

		// 回収済み枠を復活させず、予備動作中でも現在位置から直接吸着を開始
		targetPosition_ = targetPosition;
		backTimer_.Reset();
		isMoving_ = true;
	}

	int Base::GetRewardAmount() const {
		if (type_ == Type::Exp) {
			return kExpAmount;
		}
		if (type_ == Type::Money) {
			return kMoneyAmount;
		}
		return 0;
	}

	int Base::ConsumeCollectedRewardAmount() {
		const int amount = collectedRewardAmount_;
		collectedRewardAmount_ = 0;
		return amount;
	}

	void Base::Update(float deltaTime) {
		UpdateInternal(deltaTime, nullptr);
	}

	void Base::Update(float deltaTime, Player::Base& player) {
		UpdateInternal(deltaTime, &player);
	}

	void Base::UpdateInternal(float deltaTime, Player::Base* player) {

		// 回収済みPool枠を再配置まで更新対象外に設定
		if (!isAlive_) {
			return;
		}

		const Vector3 targetPosition = [&]() {
			if (player) {
				return (player->GetPosition());
			} else {
				return (targetPosition_);
			}
		}();

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

			Deactivate();
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

		// ExpとMoneyは各Poolの上書き分とManagerで合算し、Healだけをその場で適用
		switch (type_) {
		case Type::Exp:
		case Type::Money:
			collectedRewardAmount_ = GetRewardAmount();
			break;
		case Type::Heal:
			player.RecoverHealth(kHealAmount);
			break;
		}

		// ExpのSEはManagerで同Frameの回収を集約し、個数分の同時再生を回避
		if (type_ != Type::Exp) {
			MyAudio::Play(DropObjectTypeToString(type_));
		}

		Deactivate();
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
