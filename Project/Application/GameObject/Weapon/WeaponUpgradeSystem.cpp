#include "WeaponUpgradeSystem.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Weapon {
	namespace {
		constexpr std::size_t kChoiceCount = 3;

		struct RarityWeight {
			Rarity rarity;
			int weight;
		};

		// Uniqueを含めず、武器強化専用の抽選テーブルを明示します。
		constexpr std::array<RarityWeight, 4> kRarityWeights = {
			RarityWeight{ Rarity::Uncommon,  60 },
			RarityWeight{ Rarity::Rare,      25 },
			RarityWeight{ Rarity::Epic,      10 },
			RarityWeight{ Rarity::Legendary, 5 },
		};

		/// @brief レアリティの日本語表示名を取得します。
		/// @param rarity 表示名を取得するレアリティです。
		/// @return レアリティの日本語表示名です。
		const char* GetRarityDisplayName(Rarity rarity) {
			switch (rarity) {
			case Rarity::Uncommon:  return "アンコモン";
			case Rarity::Rare:      return "レア";
			case Rarity::Epic:      return "エピック";
			case Rarity::Legendary: return "レジェンダリー";
			default:                return "不明";
			}
		}

		/// @brief レアリティの表示色を取得します。
		/// @param rarity 表示色を取得するレアリティです。
		/// @return RGBA形式の表示色です。
		std::array<float, 4> GetRarityDisplayColor(Rarity rarity) {
			switch (rarity) {
			case Rarity::Uncommon:  return { 0.20f, 0.85f, 0.30f, 1.0f };
			case Rarity::Rare:      return { 0.20f, 0.55f, 1.00f, 1.0f };
			case Rarity::Epic:      return { 0.70f, 0.30f, 1.00f, 1.0f };
			case Rarity::Legendary: return { 1.00f, 0.85f, 0.15f, 1.0f };
			default:                return { 1.00f, 1.00f, 1.00f, 1.0f };
			}
		}

		/// @brief 二つの強化加算値が表示精度内で一致するか確認します。
		/// @param left 比較する加算値です。
		/// @param right 比較する加算値です。
		/// @return 一致する場合はtrueを返します。
		bool IsSameUpgradeAmount(float left, float right) {
			if (!std::isfinite(left) || !std::isfinite(right)) {
				return false;
			}

			const float scale = std::max({ 1.0f, std::fabs(left), std::fabs(right) });
			return std::fabs(left - right) <= scale * 0.00001f;
		}
	}

	void UpgradeSystem::Initialize(int currentPlayerLevel, std::uint32_t randomSeed) {
		previousPlayerLevel_ = currentPlayerLevel;
		pendingUpgradeCount_ = 0;
		choices_.clear();
		random_.SetSeed(randomSeed);
		nextGeneration_ = 1;
		choiceInventoryRevision_ = 0;
		lastGenerationAttemptRevision_ = 0;
		hasGenerationAttempted_ = false;
		generationFailureLogged_ = false;
	}

	void UpgradeSystem::UpdatePlayerLevel(int currentPlayerLevel, const Inventory& inventory) {
		bool levelIncreased = false;
		if (currentPlayerLevel > previousPlayerLevel_) {
			const int levelDifference = currentPlayerLevel - previousPlayerLevel_;
			if (pendingUpgradeCount_ <= std::numeric_limits<int>::max() - levelDifference) {
				pendingUpgradeCount_ += levelDifference;
			} else {
				pendingUpgradeCount_ = std::numeric_limits<int>::max();
				Logger::Output("[Application] 未処理アップグレード回数が上限に達しました。", Logger::Level::Warning);
			}

			levelIncreased = true;
			Logger::Output(
				"[Application] Playerのレベル差分" + std::to_string(levelDifference) +
				"を武器アップグレード回数へ追加しました。",
				Logger::Level::Debug
			);
		}

		// レベルが下がった場合も未処理回数は維持し、比較基準だけを同期します。
		previousPlayerLevel_ = currentPlayerLevel;

		if (!choices_.empty() && inventory.GetRevision() != choiceInventoryRevision_) {
			choices_.clear();
			hasGenerationAttempted_ = false;
			Logger::Output("[Application] 装備状態が変化したため武器アップグレード候補を更新します。", Logger::Level::Debug);
		}

		if (!IsUpgrading() || !choices_.empty()) {
			return;
		}

		const bool inventoryChanged = !hasGenerationAttempted_ ||
			lastGenerationAttemptRevision_ != inventory.GetRevision();
		if (levelIncreased || inventoryChanged) {
			GenerateChoices(inventory);
		}
	}

	bool UpgradeSystem::GenerateChoices(const Inventory& inventory) {
		hasGenerationAttempted_ = true;
		lastGenerationAttemptRevision_ = inventory.GetRevision();
		choices_.clear();

		std::vector<Projectile::Type> candidateWeapons;
		candidateWeapons.reserve(Projectile::kPlayableWeaponTypes.size());
		for (const Projectile::Type weaponType : Projectile::kPlayableWeaponTypes) {
			const BaseWeapon* weapon = inventory.GetWeapon(weaponType);
			if (weapon) {
				if (!weapon->GetSelectableUpgradeStatTypes().empty()) {
					candidateWeapons.push_back(weaponType);
				}
			} else if (inventory.HasEmptySlot()) {
				candidateWeapons.push_back(weaponType);
			}
		}

		if (candidateWeapons.size() < kChoiceCount) {
			if (!generationFailureLogged_) {
				Logger::Output("[Application] 異なる武器を対象とした三つの強化候補を生成できません。", Logger::Level::Error);
				generationFailureLogged_ = true;
			}
			return false;
		}

		const std::uint64_t generation = nextGeneration_++;
		choiceInventoryRevision_ = inventory.GetRevision();
		choices_.reserve(kChoiceCount);

		for (std::size_t choiceIndex = 0; choiceIndex < kChoiceCount; ++choiceIndex) {
			const int randomIndex = random_.Int(0, static_cast<int>(candidateWeapons.size()) - 1);
			const Projectile::Type weaponType = candidateWeapons[static_cast<std::size_t>(randomIndex)];
			candidateWeapons.erase(candidateWeapons.begin() + randomIndex);

			UpgradeChoice choice;
			choice.weaponType = weaponType;
			choice.generation = generation;
			choice.weaponDisplayName = Projectile::ProjectileTypeToDisplayName(weaponType);

			const BaseWeapon* weapon = inventory.GetWeapon(weaponType);
			if (!weapon) {
				choice.choiceType = UpgradeChoiceType::NewWeapon;
				choice.choiceTypeDisplayName = "新しい武器を装備";
				choices_.push_back(std::move(choice));
				continue;
			}

			const std::vector<UpgradeStatType> selectableStats = weapon->GetSelectableUpgradeStatTypes();
			if (selectableStats.empty()) {
				choices_.clear();
				if (!generationFailureLogged_) {
					Logger::Output("[Application] 所持武器に強化対象ステータスがありません。", Logger::Level::Error);
					generationFailureLogged_ = true;
				}
				return false;
			}

			const int statIndex = random_.Int(0, static_cast<int>(selectableStats.size()) - 1);
			const UpgradeStatType statType = selectableStats[static_cast<std::size_t>(statIndex)];
			const Rarity rarity = DrawRarity();
			float calculatedAmount = 0.0f;
			if (!weapon->CalculateUpgradeAmount(statType, rarity, calculatedAmount)) {
				choices_.clear();
				if (!generationFailureLogged_) {
					Logger::Output("[Application] 武器の強化加算値を計算できません。", Logger::Level::Error);
					generationFailureLogged_ = true;
				}
				return false;
			}

			choice.choiceType = UpgradeChoiceType::OwnedWeaponUpgrade;
			choice.choiceTypeDisplayName = "所持武器を強化";
			choice.statType = statType;
			choice.rarity = rarity;
			choice.calculatedAmount = calculatedAmount;
			choice.statDisplayName = UpgradeStatTypeToDisplayName(statType);
			choice.rarityDisplayName = GetRarityDisplayName(rarity);
			choice.rarityDisplayColor = GetRarityDisplayColor(rarity);
			choices_.push_back(std::move(choice));
		}

		generationFailureLogged_ = false;
		Logger::Output("[Application] 武器アップグレード候補を三つ生成しました。", Logger::Level::Debug);
		return true;
	}

	Rarity UpgradeSystem::DrawRarity() {
		int totalWeight = 0;
		for (const RarityWeight& entry : kRarityWeights) {
			totalWeight += entry.weight;
		}

		const int draw = random_.Int(1, totalWeight);
		int cumulativeWeight = 0;
		for (const RarityWeight& entry : kRarityWeights) {
			cumulativeWeight += entry.weight;
			if (draw <= cumulativeWeight) {
				return entry.rarity;
			}
		}

		return Rarity::Legendary;
	}

	bool UpgradeSystem::SelectChoice(std::size_t choiceIndex, std::uint64_t generation, Inventory& inventory) {
		if (!IsUpgrading() || choices_.empty()) {
			Logger::Output("[Application] 適用できる武器アップグレード候補がありません。", Logger::Level::Error);
			return false;
		}

		if (choiceIndex >= choices_.size()) {
			Logger::Output("[Application] 武器アップグレードの選択番号が不正です。", Logger::Level::Error);
			return false;
		}

		const UpgradeChoice choice = choices_[choiceIndex];
		if (choice.generation != generation || inventory.GetRevision() != choiceInventoryRevision_) {
			Logger::Output("[Application] 古い武器アップグレード候補は適用できません。", Logger::Level::Error);
			return false;
		}

		bool applied = false;
		if (choice.choiceType == UpgradeChoiceType::NewWeapon) {
			if (choice.statType || choice.rarity || inventory.HasWeapon(choice.weaponType) || !inventory.HasEmptySlot()) {
				Logger::Output("[Application] 新武器取得候補と現在の装備状態が一致しません。", Logger::Level::Error);
				return false;
			}

			applied = inventory.AddWeapon(choice.weaponType);
		} else {
			if (!choice.statType || !choice.rarity) {
				Logger::Output("[Application] 所持武器強化候補の内容が不正です。", Logger::Level::Error);
				return false;
			}

			BaseWeapon* weapon = inventory.GetWeapon(choice.weaponType);
			if (!weapon) {
				Logger::Output("[Application] 所持していない武器は強化できません。", Logger::Level::Error);
				return false;
			}

			float recalculatedAmount = 0.0f;
			if (!weapon->CalculateUpgradeAmount(*choice.statType, *choice.rarity, recalculatedAmount) ||
				!IsSameUpgradeAmount(recalculatedAmount, choice.calculatedAmount)) {
				Logger::Output("[Application] 表示時と適用時の武器強化値が一致しません。", Logger::Level::Error);
				return false;
			}

			float appliedAmount = 0.0f;
			applied = weapon->ApplyUpgrade(
				*choice.statType,
				*choice.rarity,
				choice.calculatedAmount,
				appliedAmount
			);
		}

		if (!applied) {
			Logger::Output("[Application] 武器アップグレード候補の適用に失敗しました。", Logger::Level::Error);
			return false;
		}

		--pendingUpgradeCount_;
		Logger::Output(
			"[Application] 武器アップグレードを適用しました: " + choice.weaponDisplayName +
			" / " + choice.choiceTypeDisplayName,
			Logger::Level::Application
		);

		choices_.clear();
		hasGenerationAttempted_ = false;
		if (IsUpgrading()) {
			GenerateChoices(inventory);
		}

		return true;
	}
}
