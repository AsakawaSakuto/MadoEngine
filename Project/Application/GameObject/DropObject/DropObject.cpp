#include "DropObject.h"
#include "Audio/MyAudio.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace DropObject {

	namespace {
		constexpr float kMoveSpeed = 20.0f;      // DropObjectのPlayerへの吸着速度
		constexpr float kArriveDistance = 0.05f; // DropObjectがPlayerに到達したと判定する距離
		constexpr float kBackDuration = 0.2f;    // DropObjectがPlayerから後退する予備動作の継続時間

		constexpr int kExpAmount = 10;           // DropObjectのExpがPlayerに加算する経験値量
		constexpr int kSheetAnimationFrameCount = 4;
		constexpr float kSheetAnimationFrameDuration = 0.15f; // 各Dropシートの1コマの表示時間（秒）

		constexpr float kMoneyRiseSpeed = 8.0f;            // DropObjectのMoneyが後退中に上昇する速度
		constexpr int kMoneyAmount = 1;                    // DropObjectのMoneyがPlayerに加算する所持金量

		constexpr float kHealAmount = 50.0f;               // DropObjectのHealがPlayerに加算するHP量

		constexpr float kSpecialSpawnJumpHeight = 2.0f;    // 特殊Dropの生成位置からの跳ね上がり高さ
		constexpr float kSpecialSpawnRiseDuration = 0.3f;  // 特殊Dropの上昇時間（秒）
		constexpr float kSpecialSpawnFallDuration = 0.7f;  // 特殊Dropの落下時間（秒）
	}

	Base::~Base() {
		Release();
	}

	void Base::Initialize(Type type, const Vector3& position, int index) {
		isReleased_ = false;
		colliderName_ = "DropObject" + std::to_string(index);
		modelName_ = "DropObject" + std::to_string(index);

		model_ = MyModel::Create(modelName_, "DropObjectPlane", SceneType::Game);

		AABB aabb;
		aabb.center = position;
		aabb.min = { -0.125f, 0.0f, -0.125f };
		aabb.max = { 0.125f, 0.25f, 0.125f };

		colliderShape_ = aabb;

		// 取得可否を参照して未使用枠と出現演出中を衝突対象から除外し、Collider再登録を回避
		MyCollider::RegisterCollider(
			colliderName_,
			CollisionTag::DropObjectHitBox,
			&colliderShape_,
			&transform_.translate,
			0.0f,
			nullptr,
			&isCollectible_
		);

		if (Model* model = MyModel::TryGet(model_)) {

			// Cameraの俯仰角とRollを含む回転で画面への正対を維持
			model->SetUseBillboard(true);
			model->SetBillboardYAxisOnly(false);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}
		Activate(type, position);
	}

	void Base::Activate(Type type, const Vector3& position) {
		std::string textureName;

		// 共用枠の再配置で以前の種類の画像を持ち越さないよう表示を種類から再設定
		switch (type) {
		case Type::Exp:
			textureName = "ExpSheet";
			break;
		case Type::Money:
			textureName = "Coin";
			break;
		case Type::Heal:
			textureName = "HealSheet";
			break;
		case Type::AttackUpper:
			textureName = "DamageUpperSheet";
			break;
		case Type::SpeedUpper:
			textureName = "SpeedUpperSheet";
			break;
		case Type::DefuseUpper:
			textureName = "DefenseUpperSheet";
			break;
		default:
			return;
		}
		type_ = type;
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture(textureName);
			if (type_ == Type::Money) {

				// 静止画像へ切り替える際に前の種類のシートUVを解除
				model->ClearSpriteSheetAnimation();
			} else {

				// Expと特殊4種類は同じ横4コマ構成を使用し、コマ送りをModelへ委譲
				MadoEngine::SpriteSheetAnimationSettings settings;
				settings.columns = kSheetAnimationFrameCount;
				settings.rows = 1;
				settings.frameDuration = kSheetAnimationFrameDuration;
				settings.loop = true;
				model->SetSpriteSheetAnimation(settings);
			}
		}
		Activate(position);
	}

	void Base::Activate(const Vector3& position) {

		// 上書き前の吸着状態と予備動作を持ち越さず、新しいDropとして配置
		transform_.translate = position;
		transform_.rotate = {};
		transform_.scale = { 0.2f, 0.2f, 0.2f };
		targetPosition_ = position;
		spawnPosition_ = position;
		backTimer_.Reset();
		spawnMotionTimer_.Reset();
		collectedRewardAmount_ = 0;
		isMoving_ = type_ == Type::Money;
		isAlive_ = true;
		isCollectible_ = true;
		if (type_ == Type::Money) {

			// Moneyだけは配置直後から上昇付きの吸着予備動作を開始
			transform_.scale = { 0.1f, 0.1f, 0.1f };
			backTimer_.Start(kBackDuration);
		} else if (type_ != Type::Exp) {

			// 特殊4種類は生成位置を着地点として固定し、出現演出の完了まで回収を禁止
			transform_.scale = { 0.75f, 0.75f, 0.75f };

			isCollectible_ = false;
			spawnMotionTimer_.Start(kSpecialSpawnRiseDuration + kSpecialSpawnFallDuration);
		}

		std::get<AABB>(colliderShape_).center = position;
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);

			// Poolの再利用直後にUV範囲と先頭コマを復元し、前のDropの再生位相の持ち越しを防止
			model->SetUVTransform({});
			if (type_ != Type::Money) {
				model->PlaySpriteSheetAnimation();
			}
			model->SetVisible(true);
		}
	}

	void Base::Deactivate() {

		// 回収可否と生存状態、Model表示を同時に停止し、リソースは次の配置まで保持
		isAlive_ = false;
		isCollectible_ = false;
		isMoving_ = false;
		backTimer_.Reset();
		spawnMotionTimer_.Reset();
		if (Model* model = MyModel::TryGet(model_)) {
			model->StopSpriteSheetAnimation();
			model->SetVisible(false);
		}
	}

	void Base::AttractTo(const Vector3& targetPosition) {

		// 外部からの吸着要求でも出現演出中の取得禁止を維持
		if (!isAlive_ || !isCollectible_) {
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

		// 着地したFrameもここで終了し、生成位置での停止を確定してから次回に回収を開始
		if (!isCollectible_) {
			UpdateSpawnMotion(deltaTime);
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

	void Base::UpdateSpawnMotion(float deltaTime) {

		// 不正な経過時間では進行を止め、補間計算への非有限値の混入を防止
		if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
			spawnMotionTimer_.Update(deltaTime);
		}
		const float elapsedTime = spawnMotionTimer_.GetElapsedTime();
		transform_.translate = spawnPosition_;
		if (spawnMotionTimer_.IsFinished()) {

			// 補間誤差を残さず元の生成位置で停止し、出現演出から取得可能状態へ遷移
			isCollectible_ = true;
		} else if (elapsedTime < kSpecialSpawnRiseDuration) {

			// 上昇は終端で減速する二次補間で短く跳ね上がり、頂点で速度をゼロへ収束
			const float progress = elapsedTime / kSpecialSpawnRiseDuration;
			const float remaining = 1.0f - progress;
			transform_.translate.y += kSpecialSpawnJumpHeight * (1.0f - remaining * remaining);
		} else {

			// 落下時間を上昇より長く確保し、頂点から緩やかに加速して着地点へ復帰
			const float progress = std::clamp(
				(elapsedTime - kSpecialSpawnRiseDuration) / kSpecialSpawnFallDuration, 0.0f, 1.0f);
			transform_.translate.y += kSpecialSpawnJumpHeight * (1.0f - progress * progress);
		}

		// 演出中も描画と衝突形状の座標を同期し、着地時の古い空中座標の参照を防止
		std::get<AABB>(colliderShape_).center = transform_.translate;
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetPosition(transform_.translate);
		}
	}

	void Base::Collect(Player::Base& player) {
		if (!isAlive_ || !isCollectible_) {
			return;
		}

		// 通常報酬はManagerで合算し、回復と一時効果はPlayerへ取得時点で適用
		switch (type_) {
		case Type::Exp:
		case Type::Money:
			collectedRewardAmount_ = GetRewardAmount();
			break;
		case Type::Heal:
			player.RecoverHealth(kHealAmount);
			break;
		case Type::AttackUpper:
			player.ApplyTemporaryEffect(Player::TemporaryEffectType::AttackBoost);
			break;
		case Type::SpeedUpper:
			player.ApplyTemporaryEffect(Player::TemporaryEffectType::SpeedBoost);
			break;
		case Type::DefuseUpper:
			player.ApplyTemporaryEffect(Player::TemporaryEffectType::Invincibility);
			break;
		}

		// 個別SEは音源のあるMoneyとHealに限定し、ExpのSEはManagerで同Frameの回収を集約
		if (type_ == Type::Money || type_ == Type::Heal) {
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
