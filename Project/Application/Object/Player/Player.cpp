#include "Player.h"
#include <algorithm>
#include <cmath>

namespace Player {

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

		MyCollider::RegisterCollider("PlayerMovementSphere", CollisionTag::PlayerMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider("PlayerHitBox", CollisionTag::PlayerHitBox, &hitAABB_, &transform_.translate, 0.0f);

		model_ = MyModel::Create("Player", "walk", SceneType::Test);
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
		model_->SetTexture("white16x16");

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
	}

	void Base::TakeDamage(float damage) {
		if (damage <= 0 || status_.currentHealth <= 0) {
			return;
		}

		status_.currentHealth = std::max(0.0f, status_.currentHealth - damage);
	}

	void Base::Update(float deltaTime) {
		controller_.Update();
		const MoveInput& moveInput = controller_.GetMoveInput();

		// 入力移動と重力による落下処理を先に行う。
		movement_.Update(deltaTime, transform_, camera_, moveInput);

		transform_.translate.x = std::clamp(transform_.translate.x, mapLimit_.min.x, mapLimit_.max.x);
		transform_.translate.y = std::clamp(transform_.translate.y, mapLimit_.min.y, mapLimit_.max.y);
		transform_.translate.z = std::clamp(transform_.translate.z, mapLimit_.min.z, mapLimit_.max.z);

		// 移動後の位置で押し戻しを行い、描画位置にも解決後の座標を反映する。
		MyCollider::Update();

		const bool isGroundContact = MyCollider::IsGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock);
		const bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope);
		movement_.SetGroundContact(isGroundContact, isSlopeGroundContact, moveInput);
		movement_.UpdateModelTransform(deltaTime, transform_, model_, isSlopeGroundContact);

		model_->SetPosition(transform_.translate + Vector3{ 0.0f, -0.5f, 0.0f });
		model_->SetRotation(transform_.rotate);
		model_->SetScale(transform_.scale);

		if (movement_.GetCurrentMotion() == Player::Motion::Crouching) {
			model_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
		} else {
			model_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		if (MyInput::GetKeybord()->IsTrigger(DIK_F3)) {
			transform_.translate = { 0.0f, 100.0f, 0.0f };
		}

		// デバッグ表示
		Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
		MyDebugLine::AddShape(std::get<Sphere>(colliderShape_), color);
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
