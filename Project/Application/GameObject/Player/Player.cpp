#include "Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace Player {
	namespace {
		constexpr float kShadowGroundOffset = 0.01f;
	}

	void Base::Initialize() {
		transform_.translate = { 0.0f, 100.0f, 0.0f };
		transform_.SetAllScale(0.5f);

		AABB aabb;
		aabb.min = { -0.5f, 0.0f, -0.5f };
		aabb.max = { 0.5f, 2.0f, 0.5f };
		hitAABB_ = aabb;

		Sphere s;
		s.radius = 0.5f;
		colliderShape_ = s;

		Sphere s2;
		s2.radius = 2.5f;
		expGetSphere_ = s2;

		MyCollider::RegisterCollider("PlayerMovementSphere", CollisionTag::PlayerMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerHitBox", CollisionTag::PlayerHitBox, &hitAABB_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerExpGetSphere", CollisionTag::PlayerDropObjectGetSphere, &expGetSphere_, &transform_.translate, 0.0f);

		model_ = MyModel::Create("Player", "walk", SceneType::Test);
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
		model_->SetTexture("white16x16");
		model_->SetCastShadow(false);

		shadowTransform_.scale = { 0.5f, 0.1f, 0.5f };
		shadowModel_ = MyModel::Create("PlayerShadow", "walk", SceneType::Test);
		shadowModel_->SetRenderLayer(MadoEngine::Render::RenderLayer::Default);
		shadowModel_->SetTexture("white16x16");
		shadowModel_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
		shadowModel_->SetCastShadow(false);
		shadowModel_->SetReceiveShadow(false);
		shadowModel_->SetLightingEnabled(false);

		movement_.Initialize();
	}

	void Base::AddMoney(int amount) {
		if (amount <= 0) {
			return;
		}

		status_.currentMoney += amount;
	}

	void Base::AddExp(int amount) {
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
	}

	void Base::Update(float deltaTime) {
		lastDeltaTime_ = std::max(0.0f, deltaTime);
		controller_.Update();
		lastMoveInput_ = controller_.GetMoveInput();

		// 全Colliderを更新する前に入力移動と重力による落下を反映する。
		movement_.Update(lastDeltaTime_, transform_, camera_, lastMoveInput_);

		transform_.translate.x = std::clamp(transform_.translate.x, mapLimit_.min.x, mapLimit_.max.x);
		transform_.translate.y = std::clamp(transform_.translate.y, mapLimit_.min.y, mapLimit_.max.y);
		transform_.translate.z = std::clamp(transform_.translate.z, mapLimit_.min.z, mapLimit_.max.z);

		if (MyInput::GetKeybord()->IsTrigger(DIK_F3)) {
			transform_.translate = { 0.0f, 100.0f, 0.0f };
		}
	}

	void Base::ResolveAfterCollision() {
		const bool isGroundContact = MyCollider::IsGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock);
		const bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope);
		movement_.SetGroundContact(isGroundContact, isSlopeGroundContact, lastMoveInput_);
		movement_.UpdateModelTransform(lastDeltaTime_, transform_, model_, isSlopeGroundContact);

		model_->SetPosition(transform_.translate + Vector3{ 0.0f, -0.5f, 0.0f });
		model_->SetRotation(transform_.rotate);
		model_->SetScale(transform_.scale);

		UpdateShadowTransform();

		if (movement_.GetCurrentMotion() == Player::Motion::Crouching) {
			model_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
		} else {
			model_->SetColor(gamingColor_.Update(lastDeltaTime_, 1.0f));
		}

		// デバッグ表示
		Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
		MyDebugLine::AddShape(std::get<Sphere>(colliderShape_), color);
		MyDebugLine::AddShape(std::get<Sphere>(expGetSphere_), Vector4{ 0.0f,0.0f,1.0f,1.0f });
	}

	/// @brief Player直下の地面へ影の描画座標を更新します。
	void Base::UpdateShadowTransform() {
		if (!shadowModel_) {
			return;
		}

		const float maxGroundDistance = mapLimit_.max.y - mapLimit_.min.y;
		float groundY = 0.0f;
		float surfaceY = 0.0f;
		bool foundGround = false;

		if (MyCollider::TryGetGroundSurfaceY(transform_.translate, CollisionTag::MapBlock, surfaceY, maxGroundDistance)) {
			groundY = surfaceY;
			foundGround = true;
		}

		if (MyCollider::TryGetGroundSurfaceY(transform_.translate, CollisionTag::MapSlope, surfaceY, maxGroundDistance) &&
			(!foundGround || surfaceY > groundY)) {
			groundY = surfaceY;
			foundGround = true;
		}

		shadowModel_->SetVisible(foundGround);
		if (!foundGround) {
			return;
		}

		shadowTransform_.translate = {
			transform_.translate.x,
			groundY,
			transform_.translate.z
		};
		
		shadowModel_->SetPosition(shadowTransform_.translate);
		shadowModel_->SetRotation(transform_.rotate);
		shadowModel_->SetScale(shadowTransform_.scale);
	}

	/// @brief Playerの描画Model座標を取得します。
	/// @return PlayerのModelワールド座標です。
	Vector3 Base::GetModelPosition() const {
		if (model_) {
			return model_->GetPosition();
		}

		return transform_.translate + Vector3{ 0.0f, -0.5f, 0.0f };
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
