#include "EnemyStatusEffect.h"
#include <algorithm>
#include <cmath>

namespace Enemy::StatusEffect {
	namespace {
		constexpr float kBurnDamageTickInterval = 1.0f;
		constexpr float kPoisonDamageTickInterval = 1.0f;
	}

	void Controller::Clear() {
		states_ = {};
	}

	bool Controller::Apply(const ApplyRequest& request) {
		if (!IsValidDefinition(request.type, request.definition)) {
			return false;
		}

		const Definition normalizedDefinition = NormalizeDefinition(request.type, request.definition);
		ActiveState& state = states_[ToIndex(request.type)];
		if (!state.isActive) {
			state.definition = normalizedDefinition;
			state.remainingTime = normalizedDefinition.duration;
			state.tickAccumulator = 0.0f;
			state.sourceWeaponId = request.sourceWeaponId;
			state.isActive = true;
			return true;
		}

		// 同種の再付与では残り時間を短縮せず、弱い効果による性能上書きを防止
		state.remainingTime = std::max(state.remainingTime, normalizedDefinition.duration);
		if (IsStronger(request.type, state.definition, normalizedDefinition)) {
			state.definition = normalizedDefinition;
			state.sourceWeaponId = request.sourceWeaponId;
			const float damageTickInterval = GetDamageTickInterval(request.type);
			if (damageTickInterval > 0.0f) {
				state.tickAccumulator = std::fmod(state.tickAccumulator, damageTickInterval);
			}
		}

		return true;
	}

	UpdateResult Controller::Update(float deltaTime) {
		UpdateResult result;
		if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return result;
		}

		for (std::size_t index = 0; index < states_.size(); ++index) {
			ActiveState& state = states_[index];
			if (!state.isActive) {
				continue;
			}

			// 効果時間を越えたFrame時間を周期Damageの計算へ含めないよう有効区間だけを使用
			const float activeDeltaTime = std::min(deltaTime, state.remainingTime);
			const float damageTickInterval = GetDamageTickInterval(static_cast<Type>(index));
			if (damageTickInterval > 0.0f && state.definition.damagePerTick > 0.0f) {
				state.tickAccumulator += activeDeltaTime;
				const float tickCount = std::floor(state.tickAccumulator / damageTickInterval);
				if (tickCount >= 1.0f) {

					// Frame落ちでも本来発生する周期回数を失わないよう同種のDamageを一件へ集約
					const float damage = state.definition.damagePerTick * tickCount;
					if (std::isfinite(damage) && damage > 0.0f && result.damageEventCount < result.damageEvents.size()) {
						result.damageEvents[result.damageEventCount++] = {
							static_cast<Type>(index),
							damage,
							state.sourceWeaponId,
						};
					}
					state.tickAccumulator = std::fmod(state.tickAccumulator, damageTickInterval);
				}
			}

			state.remainingTime = std::max(0.0f, state.remainingTime - activeDeltaTime);
			if (state.remainingTime <= 0.0f) {
				state = {};
			}
		}

		return result;
	}

	float Controller::GetMoveSpeedMultiplier() const {
		float multiplier = 1.0f;
		for (const ActiveState& state : states_) {
			if (!state.isActive) {
				continue;
			}

			// 基礎速度を変更せず有効な速度補正だけを合成して解除時の復元を不要化
			multiplier *= state.definition.moveSpeedMultiplier;
		}

		return std::clamp(multiplier, 0.0f, 1.0f);
	}

	bool Controller::IsActive(Type type) const {
		if (type >= Type::Count) {
			return false;
		}

		return states_[ToIndex(type)].isActive;
	}

	float Controller::GetRemainingTime(Type type) const {
		if (type >= Type::Count) {
			return 0.0f;
		}

		const ActiveState& state = states_[ToIndex(type)];
		return state.isActive ? state.remainingTime : 0.0f;
	}

	std::size_t Controller::ToIndex(Type type) {
		return static_cast<std::size_t>(type);
	}

	bool Controller::IsValidDefinition(Type type, const Definition& definition) {
		if (type >= Type::Count || !std::isfinite(definition.duration) || definition.duration <= 0.0f) {
			return false;
		}

		switch (type) {
		case Type::Burn:
		case Type::Poison:
			return std::isfinite(definition.damagePerTick) && definition.damagePerTick > 0.0f;
		case Type::Frozen:
			return std::isfinite(definition.moveSpeedMultiplier) &&
				definition.moveSpeedMultiplier >= 0.0f && definition.moveSpeedMultiplier < 1.0f;
		default:
			return false;
		}
	}

	Definition Controller::NormalizeDefinition(Type type, const Definition& definition) {
		Definition normalizedDefinition = definition;
		switch (type) {
		case Type::Burn:
		case Type::Poison:

			// 継続Damage系へ移動速度補正を混在させず種類ごとの責務を固定
			normalizedDefinition.moveSpeedMultiplier = 1.0f;
			break;
		case Type::Frozen:

			// Frozenへ不要なダメージ値を保持せず効果判定を移動速度補正へ限定
			normalizedDefinition.damagePerTick = 0.0f;
			break;
		default:
			break;
		}

		return normalizedDefinition;
	}

	bool Controller::IsStronger(
		Type type,
		const Definition& currentDefinition,
		const Definition& newDefinition) {
		switch (type) {
		case Type::Burn:
		case Type::Poison:
			return newDefinition.damagePerTick > currentDefinition.damagePerTick;
		case Type::Frozen:
			return newDefinition.moveSpeedMultiplier < currentDefinition.moveSpeedMultiplier;
		default:
			return false;
		}
	}

	float Controller::GetDamageTickInterval(Type type) {
		switch (type) {
		case Type::Burn:
			return kBurnDamageTickInterval;
		case Type::Poison:
			return kPoisonDamageTickInterval;
		default:
			return 0.0f;
		}
	}
} // namespace Enemy::StatusEffect
