#include "PlayLogRecorder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace System {

	namespace {
		constexpr double kScorePerKill = 1.0;
		constexpr double kScorePerJar = 5.0;
		constexpr double kScorePerChest = 50.0;
		constexpr double kScoreMultiplierDoubleSeconds = 300.0;
	}

	void PlayLogRecorder::Initialize(std::uint32_t seed) {
		data_ = {};
		data_.seed = seed;
		state_ = State::Recording;
	}

	void PlayLogRecorder::AdvancePlayTime(double deltaTime) {

		// 時間制限とは独立して加算し、停止中や不正値による履歴の破損を防止
		if (state_ != State::Recording || !std::isfinite(deltaTime) || deltaTime <= 0.0) {
			return;
		}
		data_.elapsedSeconds += deltaTime;
		for (PlayWeaponRecord& weapon : data_.weapons) {
			if (weapon.equippedAtEnd) {
				weapon.equippedSeconds += deltaTime;
			}
		}
	}

	void PlayLogRecorder::RecordWeaponChange(const Weapon::InventoryChangeEvent& event) {
		if (state_ != State::Recording || event.weaponId == 0) {
			return;
		}
		PlayWeaponRecord* weapon = FindWeapon(event.weaponId);

		// 同じSlotへの再取得も別インスタンスとして保持し、削除後の遅延命中を元武器へ帰属
		if (event.type == Weapon::InventoryChangeType::Acquired) {
			if (weapon) {
				return;
			}
			PlayWeaponRecord record;
			record.weaponId = event.weaponId;
			record.slotIndex = event.slotIndex;
			record.name = event.displayName;
			record.level = event.level;
			record.acquiredAtSeconds = data_.elapsedSeconds;
			data_.weapons.push_back(std::move(record));
		} else if (event.type == Weapon::InventoryChangeType::Removed && weapon) {
			weapon->level = event.level;
			weapon->equippedAtEnd = false;
		}
	}

	void PlayLogRecorder::UpdateWeaponLevel(std::uint64_t weaponId, int level) {
		if (state_ != State::Recording) {
			return;
		}
		if (PlayWeaponRecord* weapon = FindWeapon(weaponId)) {
			weapon->level = level;
		}
	}

	void PlayLogRecorder::RecordDamage(const Combat::DamageEvent& event) {
		if (state_ != State::Recording || !std::isfinite(event.appliedDamage) || event.appliedDamage <= 0.0f) {
			return;
		}
		PlayCombatStats* stats = nullptr;

		// 状態異常はsourceWeaponIdを参照せず独立した発生源へ分類
		switch (event.sourceType) {
		case Combat::DamageSourceType::Weapon:
			if (PlayWeaponRecord* weapon = FindWeapon(event.sourceWeaponId)) {
				stats = &weapon->combat;
			}
			break;
		case Combat::DamageSourceType::Poison:
			stats = &data_.poison;
			break;
		case Combat::DamageSourceType::Burn:
			stats = &data_.burn;
			break;
		}
		if (!stats) {
			return;
		}

		// 各分類と全体へ同じ結果を加算し、状態異常の二重計上を防止
		stats->damage += event.appliedDamage;
		data_.totalCombat.damage += event.appliedDamage;
		if (event.wasKilled) {
			++stats->killCount;
			++data_.totalCombat.killCount;
		}
	}

	void PlayLogRecorder::RecordMoneyGain(double amount) {
		if (state_ == State::Recording && std::isfinite(amount) && amount > 0.0) {
			data_.earnedMoney += amount;
		}
	}

	void PlayLogRecorder::RecordReroll(int paidMoney) {
		if (state_ != State::Recording || paidMoney < 0) {
			return;
		}
		++data_.rerollCount;
		data_.rerollSpentMoney += static_cast<std::uint64_t>(paidMoney);
	}

	void PlayLogRecorder::RecordInteraction(const MapInteractionEvent& event) {
		if (state_ != State::Recording || event.paidMoney < 0) {
			return;
		}

		// 成功通知だけを受け取り、料金不足や入力試行は回数へ含めない方針
		switch (event.type) {
		case MapInteractionType::PaidChest:
			++data_.paidChestCount;
			data_.chestSpentMoney += static_cast<std::uint64_t>(event.paidMoney);
			break;
		case MapInteractionType::FreeChest:
			++data_.freeChestCount;
			break;
		case MapInteractionType::ExpJar:
			++data_.expJarCount;
			break;
		case MapInteractionType::MoneyJar:
			++data_.moneyJarCount;
			break;
		case MapInteractionType::Karma:
			data_.karmaInteractionSeconds.push_back(data_.elapsedSeconds);
			break;
		case MapInteractionType::Other:
			break;
		}
	}

	bool PlayLogRecorder::Finish(PlayEndReason reason, double endingMoney, int endingDifficulty, std::vector<PlayRelicRecord> relics) {

		// 死亡後の再開操作やFinalizeの重複による終了理由とSnapshotの上書きを防止
		if (state_ != State::Recording) {
			return false;
		}
		data_.endReason = reason;
		if (std::isfinite(endingMoney)) {
			data_.endingMoney = (std::max)(0.0, endingMoney);
		}
		data_.endingDifficulty = (std::max)(0, endingDifficulty);
		data_.relics = std::move(relics);
		data_.score = CalculateScore();
		state_ = State::Finished;
		return true;
	}

	PlayWeaponRecord* PlayLogRecorder::FindWeapon(std::uint64_t weaponId) {
		for (PlayWeaponRecord& weapon : data_.weapons) {
			if (weapon.weaponId == weaponId) {
				return &weapon;
			}
		}
		return nullptr;
	}

	std::uint64_t PlayLogRecorder::CalculateScore() const {

		// 報酬種別や支払いの有無によらず、探索に成功した回数を同じ配点で評価
		const double jarCount = static_cast<double>(data_.expJarCount) + static_cast<double>(data_.moneyJarCount);
		const double chestCount = static_cast<double>(data_.paidChestCount) + static_cast<double>(data_.freeChestCount);
		const double baseScore = static_cast<double>(data_.totalCombat.killCount) * kScorePerKill +
			jarCount * kScorePerJar + chestCount * kScorePerChest;

		// 1 + 生存秒数 / 300の倍率を秒単位で適用し、分の境界での得点の急変を回避
		const double survivalBonus = baseScore * data_.elapsedSeconds / kScoreMultiplierDoubleSeconds;
		const double totalScore = baseScore + survivalBonus;

		// 整数型の範囲外への変換を避け、保存可能なスコアの上限へ制限
		const auto maxScore = (std::numeric_limits<std::uint64_t>::max)();
		if (totalScore >= static_cast<double>(maxScore)) {
			return maxScore;
		}
		return static_cast<std::uint64_t>(totalScore);
	}

}
