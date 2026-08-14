#include "EnemyBoss.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kChaseDuration = 3.0f;
	constexpr float kWindupDuration = 0.75f;
	constexpr float kRushDuration = 0.8f;
	constexpr float kRecoveryDuration = 0.75f;
	constexpr float kRushSpeedMultiplier = 3.5f;
}

namespace Enemy {

	void Boss::OnInitialized() {
		state_ = State::Chase;
		stateTimer_ = 0.0f;
		rushTargetPosition_ = GetPosition();
	}

	void Boss::UpdateBehavior(float deltaTime) {
		const float safeDeltaTime = std::isfinite(deltaTime) ? std::max(0.0f, deltaTime) : 0.0f;
		stateTimer_ += safeDeltaTime;

		// 追跡後に予備動作を挟み、固定したPlayer座標へ突進してから硬直
		switch (state_) {
		case State::Chase:
			if (!MoveTowardPlayer(safeDeltaTime)) {
				return;
			}
			if (stateTimer_ >= kChaseDuration) {
				ChangeState(State::Windup);
			}
			break;
		case State::Windup:
			if (!MoveTowardPosition(safeDeltaTime, GetPosition(), 0.0f)) {
				return;
			}
			if (stateTimer_ >= kWindupDuration) {
				rushTargetPosition_ = GetTargetPlayerPosition();
				ChangeState(State::Rush);
			}
			break;
		case State::Rush:
			if (!MoveTowardPosition(safeDeltaTime, rushTargetPosition_, kRushSpeedMultiplier)) {
				return;
			}
			if (stateTimer_ >= kRushDuration) {
				ChangeState(State::Recovery);
			}
			break;
		case State::Recovery:
			if (!MoveTowardPosition(safeDeltaTime, GetPosition(), 0.0f)) {
				return;
			}
			if (stateTimer_ >= kRecoveryDuration) {
				ChangeState(State::Chase);
			}
			break;
		}
	}

	std::string Boss::GetModelAssetName() const {
		return "Boss";
	}

	Vector3 Boss::GetModelScale() const {
		return { 1.5f, 1.5f, 1.5f };
	}

	Vector3 Boss::GetModelOffset() const {
		return { 0.0f, -1.5f, 0.0f };
	}

	Sphere Boss::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = 1.5f;
		return sphere;
	}

	AABB Boss::CreateHitCollider() const {
		AABB aabb;
		aabb.min = { -1.5f, 0.0f, -1.5f };
		aabb.max = { 1.5f, 4.0f, 1.5f };
		return aabb;
	}

	bool Boss::ShouldDisappearOnPlayerCollision() const {
		return false;
	}

	float Boss::GetPlayerDamageInterval() const {
		return 1.0f;
	}

	void Boss::ChangeState(State nextState) {
		state_ = nextState;
		stateTimer_ = 0.0f;
	}

} // namespace Enemy
