#include "Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Player {
	namespace {
		constexpr float kMovementSphereRadius = 0.5f;
		constexpr float kShadowGroundOffset = 0.01f;                        // 影の描画座標を地面より少し上にずらすことでZファイティングを回避
		constexpr float kModelForwardYawOffset = std::numbers::pi_v<float>; // Modelの前方がZ軸負方向を向くため、Y軸回転を180度補正
		constexpr float kHealthRegenerationInterval = 6.0f;                 // 被弾後にHP回復が開始されるまでの待機時間
		constexpr float kHealthRegenerationAmount = 1.0f;                   // HP回復量
	}

	void Base::Initialize(const Vector3& spawnGroundPosition) {
		transform_.translate = spawnGroundPosition;

		// 球Colliderの下端を地表へ一致させて埋まりと初期落下を防止
		transform_.translate.y += kMovementSphereRadius;
		transform_.SetAllScale(0.5f);

		AABB aabb;
		aabb.min = { -0.5f, 0.0f, -0.5f };
		aabb.max = { 0.5f, 2.0f, 0.5f };
		hitAABB_ = aabb;

		Sphere s;
		s.radius = kMovementSphereRadius;
		colliderShape_ = s;

		Sphere s2;
		s2.radius = 2.5f;
		expGetSphere_ = s2;

		Sphere s3;
		s3.radius = 50.0f;
		attackRangeSphere_ = s3;

		// 移動解決、被弾、経験値回収、攻撃索引を独立させるため用途別Colliderを登録
		MyCollider::RegisterCollider("PlayerMovementSphere", CollisionTag::PlayerMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerHitBox", CollisionTag::PlayerHitBox, &hitAABB_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerExpGetSphere", CollisionTag::PlayerDropObjectGetSphere, &expGetSphere_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerAttackRangeSphere", CollisionTag::PlayerAttackRangeSphere, &attackRangeSphere_, &transform_.translate, 0.0f);

		model_ = MyModel::Create("Player", "walk", SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
			model->SetTexture("white2x2");
			model->SetPosition(transform_.translate + Vector3{ 0.0f, -kMovementSphereRadius, 0.0f });
			model->SetScale(transform_.scale);
			animationController_.Initialize(*model);
		}

		shadowTransform_.scale = { 0.5f, 0.1f, 0.5f };

		movement_.Initialize();

		// Loop再生を維持したまま空中時だけ描画する着地点Markerの初期化
		MadoEngine::EffectSequence::EffectSequencePlayDesc landingMarkerDesc;
		landingMarkerDesc.sceneType = SceneType::Game;
		landingMarkerDesc.loopOverride = true;
		landingMarker_.Play("LandingMarker", landingMarkerDesc);
		landingMarker_.SetVisible(false);

		MadoEngine::Particle::PlayDesc desc;
		if (Model* model = MyModel::TryGet(model_)) {
			desc.transform.translate = model->GetVertexPosition(218);
		}
		desc.sceneType = SceneType::Game;
		desc.loopOverride = true;
	}

	void Base::AddMoney(int amount) {

		// 不正な加算によって所持金が減少しないよう0以下を無視
		if (amount <= 0) {
			return;
		}

		status_.currentMoney += amount;
	}

	void Base::AddExp(int amount) {

		// 不正な加算によって経験値が減少しないよう0以下を無視
		if (amount <= 0) {
			return;
		}

		status_.currentExp += amount;
		ProcessLevelUp();
	}

	void Base::ProcessLevelUp() {
		if (status_.expToNextLevel <= 0.0f) {
			Logger::Output("[Engine] Playerの必要経験値が不正なため、レベルアップ処理をスキップしました。", Logger::Level::Warning);
			return;
		}

		// 一度に大量の経験値を得た場合も取りこぼさないよう連続してレベルアップ
		while (status_.currentExp >= status_.expToNextLevel) {
			status_.currentExp -= status_.expToNextLevel;
			status_.level++;
			status_.expToNextLevel += 25.0f;
			Logger::Output("[Engine] Playerのレベルが" + std::to_string(status_.level) + "に上がりました。", Logger::Level::Application);
		}
	}

	void Base::TakeDamage(float damage) {
		if (damage <= 0 || status_.currentHealth <= 0) {
			return;
		}

		status_.currentHealth = std::max(0.0f, status_.currentHealth - damage);
		if (status_.currentHealth <= 0.0f) {
			regenerationTimer_.Stop();
			return;
		}

		// 被弾時点を起点に回復待機時間を再計測
		regenerationTimer_.Start(kHealthRegenerationInterval, true);
	}

	void Base::UpdateHealthRegeneration(float deltaTime) {

		// 死亡中または最大HPが不正な状態では回復周期を停止
		if (status_.currentHealth <= 0.0f || status_.maxHealth <= 0.0f) {
			regenerationTimer_.Stop();
			return;
		}

		// 最大HPを超える値を残さず回復済みならTimerを停止
		if (status_.currentHealth >= status_.maxHealth) {
			status_.currentHealth = status_.maxHealth;
			regenerationTimer_.Stop();
			return;
		}

		// Timer停止中にHPが減った場合も自動で回復周期を再開
		if (!regenerationTimer_.IsActive()) {
			regenerationTimer_.Start(kHealthRegenerationInterval, true);
		}

		regenerationTimer_.Update(deltaTime);

		// 毎フレームではなく設定した回復間隔を迎えたフレームだけHPを加算
		if (!regenerationTimer_.WasLoopedThisFrame()) {
			return;
		}

		status_.currentHealth = std::min(
			status_.maxHealth,
			status_.currentHealth + kHealthRegenerationAmount
		);

		if (status_.currentHealth >= status_.maxHealth) {
			regenerationTimer_.Stop();
		}
	}

	void Base::Update(float deltaTime) {
		lastDeltaTime_ = std::max(0.0f, deltaTime);
		UpdateHealthRegeneration(lastDeltaTime_);
		controller_.Update();
		lastMoveInput_ = controller_.GetMoveInput();

		// Colliderへ最新座標を渡すため全Colliderの更新前に入力移動と重力落下を反映
		movement_.Update(lastDeltaTime_, transform_, camera_, lastMoveInput_);

		transform_.translate.x = std::clamp(transform_.translate.x, mapLimit_.min.x, mapLimit_.max.x);
		transform_.translate.y = std::clamp(transform_.translate.y, mapLimit_.min.y, mapLimit_.max.y);
		transform_.translate.z = std::clamp(transform_.translate.z, mapLimit_.min.z, mapLimit_.max.z);

		if (MyInput::GetKeybord()->IsTrigger(DIK_F3)) {
			transform_.translate = { 0.0f, 100.0f, 0.0f };
		}

		if (MyInput::GetKeybord()->IsTrigger(DIK_F4)) {
			status_.currentExp += status_.expToNextLevel;
			ProcessLevelUp();
		}
	}

	void Base::ResolveAfterCollision() {

		// Collider解決後の接地結果を移動状態とModel姿勢へ反映
		const bool isGroundContact = MyCollider::IsGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock);
		const bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope);
		movement_.SetGroundContact(isGroundContact, isSlopeGroundContact, lastMoveInput_);
		movement_.UpdateWallClimb(lastDeltaTime_, lastMoveInput_, transform_);
		Model* model = MyModel::TryGet(model_);
		movement_.UpdateModelTransform(lastDeltaTime_, transform_, model, isSlopeGroundContact, kModelForwardYawOffset);

		if (model) {
			model->SetPosition(transform_.translate + Vector3{ 0.0f, -kMovementSphereRadius, 0.0f });
			model->SetScale(transform_.scale);
			const Vector3 slideVelocity = movement_.GetSlideVelocity();
			const bool isCrouchingMoving =
				slideVelocity.x * slideVelocity.x + slideVelocity.z * slideVelocity.z > 1e-6f;
			animationController_.Update(
				movement_.GetCurrentMotion(),
				isCrouchingMoving,
				movement_.IsGrounded(),
				movement_.WasJumpStartedThisFrame(),
				*model
			);
		}

		UpdateShadowTransform();

		if (model && movement_.GetCurrentMotion() == Player::Motion::Crouching) {
			model->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
		} else if (model) {
			model->SetColor(gamingColor_.Update(lastDeltaTime_, 1.0f));
		}

		// 描画結果とColliderのずれを検証できるよう判定形状を可視化
		Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
		MyDebugLine::AddShape(std::get<Sphere>(colliderShape_), color);
		MyDebugLine::AddShape(std::get<Sphere>(expGetSphere_), Vector4{ 0.0f,0.0f,1.0f,1.0f });
		MyDebugLine::AddShape(std::get<Sphere>(attackRangeSphere_), Vector4{ 1.0f,0.0f,0.0f,1.0f });
	}

	void Base::UpdateShadowTransform() {
		
		const float maxGroundDistance = mapLimit_.max.y - mapLimit_.min.y;
		float groundY = 0.0f;
		float surfaceY = 0.0f;
		bool foundGround = false;

		if (MyCollider::TryGetGroundSurfaceY(transform_.translate, CollisionTag::MapBlock, surfaceY, maxGroundDistance)) {
			groundY = surfaceY;
			foundGround = true;
		}

		// MapBlockとSlopeが重なる場所ではPlayerに最も近い上側の地表を採用
		if (MyCollider::TryGetGroundSurfaceY(transform_.translate, CollisionTag::MapSlope, surfaceY, maxGroundDistance) &&
			(!foundGround || surfaceY > groundY)) {
			groundY = surfaceY;
			foundGround = true;
		}

		landingMarker_.SetVisible(foundGround && !movement_.IsGrounded());
		if (!foundGround) {
			return;
		}

		shadowTransform_.translate = {
			transform_.translate.x,
			groundY,
			transform_.translate.z
		};
		
		// ShadowModelの拡縮に影響されないよう着地点Markerには地表座標だけを同期
		Transform3D landingMarkerTransform;
		landingMarkerTransform.translate = shadowTransform_.translate;
		landingMarker_.SetTransform(landingMarkerTransform);
	}

	Vector3 Base::GetModelPosition() const {
		if (Model* model = MyModel::TryGet(model_)) {
			return model->GetPosition();
		}

		// Modelが利用できない間もゲーム座標から同じ基準位置を返却
		return transform_.translate + Vector3{ 0.0f, -kMovementSphereRadius, 0.0f };
	}

	void Base::DrawImGui() {

#ifdef USE_IMGUI

		ImGui::Begin("プレイヤー");
		const Vector3 slideVelocity = movement_.GetSlideVelocity();
		const Vector3 jumpMoveVelocity = movement_.GetJumpMoveVelocity();
		const float velocityY = movement_.GetVelocityY();
		const float horizontalVelocityX = slideVelocity.x + jumpMoveVelocity.x;
		const float horizontalVelocityZ = slideVelocity.z + jumpMoveVelocity.z;
		const float horizontalSpeed = std::sqrt(horizontalVelocityX * horizontalVelocityX + horizontalVelocityZ * horizontalVelocityZ);
		const float jumpMoveBoostSpeed = std::sqrt(jumpMoveVelocity.x * jumpMoveVelocity.x + jumpMoveVelocity.z * jumpMoveVelocity.z);
		const float currentSpeed = std::sqrt(horizontalSpeed * horizontalSpeed + velocityY * velocityY);
		MovementParams& movementParams = movement_.GetParams();
		ImGui::Text("現在の動作: %s", ToMotionText(movement_.GetCurrentMotion()));
		ImGui::Text("速度: %.2f", currentSpeed);
		ImGui::Text("水平速度: %.2f", horizontalSpeed);
		ImGui::Text("Y速度: %.2f", velocityY);
		ImGui::Text("ジャンプ横初速: %.2f", jumpMoveBoostSpeed);
		ImGui::Text("壁上り: %s", movement_.IsWallClimbing() ? "有効" : "無効");
		ImGui::Separator();
		ImGui::DragFloat("移動速度", &movementParams.moveSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("ジャンプ力", &movementParams.jumpPower_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("重力", &movementParams.gravity_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("スライド開始速度", &movementParams.slideStartSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("スライド方向補正率", &movementParams.slideSteerRate_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("斜面スライド加速度", &movementParams.slopeSlideAcceleration_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("最大スライド速度", &movementParams.maxSlideSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("スライド摩擦", &movementParams.slideFriction_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("ジャンプ回数", &movementParams.jumpCount_, 1.0f, 0.0f, 1000.0f);
		ImGui::DragFloat("ジャンプ横初速", &movementParams.jumpMoveBoostSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("壁上り速度", &movementParams.wallClimbSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::End();

		ImGui::Begin("プレイヤーステータス");

		ImGui::Text("Health : %d / %d", status_.currentHealth, status_.maxHealth);
		ImGui::Text("Shield : %d / %d", status_.currentShield, status_.maxShield);
		ImGui::Separator();
		ImGui::Text("Lv : %d", status_.level);
		ImGui::Text("CurrentExp : %d", status_.currentExp);
		ImGui::Text("NextLvExp  : %d", status_.expToNextLevel);
		ImGui::Separator();
		ImGui::Text("Money : %d", status_.currentMoney);

		ImGui::End();

#endif // USE_IMGUI
	}

}
