#pragma once
#include "GameObject/Rarity.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace System {

	/// @brief プレイ終了の確定理由
	enum class PlayEndReason {
		Death,
		Restart,
		ReturnToTitle,
		ApplicationExit,
		SceneChange,
	};

	/// @brief 一種類の発生源による戦績
	struct PlayCombatStats {
		std::uint64_t killCount = 0;
		double damage = 0.0;
	};

	/// @brief 一つの武器インスタンスの装備履歴と戦績
	struct PlayWeaponRecord {
		std::uint64_t weaponId = 0;
		std::size_t slotIndex = 0;
		std::string name;
		int level = 1;
		PlayCombatStats combat;
		double acquiredAtSeconds = 0.0;
		double equippedSeconds = 0.0;
		bool equippedAtEnd = true;
	};

	/// @brief 終了確定時に所持していたレリック
	struct PlayRelicRecord {
		std::string name;
		int count = 0;
		Rarity rarity = Rarity::Uncommon;
	};

	/// @brief 一回のプレイに対応する保存データ
	struct PlayLogData {
		std::uint32_t seed = 0;
		PlayEndReason endReason = PlayEndReason::ApplicationExit;
		double elapsedSeconds = 0.0;
		std::uint64_t score = 0;
		PlayCombatStats totalCombat;
		double earnedMoney = 0.0;
		double endingMoney = 0.0;
		std::vector<PlayWeaponRecord> weapons;
		PlayCombatStats poison;
		PlayCombatStats burn;
		std::uint64_t rerollCount = 0;
		std::uint64_t rerollSpentMoney = 0;
		std::uint64_t paidChestCount = 0;
		std::uint64_t freeChestCount = 0;
		std::uint64_t chestSpentMoney = 0;
		std::vector<PlayRelicRecord> relics;
		std::uint64_t expJarCount = 0;
		std::uint64_t moneyJarCount = 0;
		std::vector<double> karmaInteractionSeconds;
		int endingDifficulty = 0;
	};

}
