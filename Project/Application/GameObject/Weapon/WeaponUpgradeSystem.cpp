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

		// 新武器取得専用のUniqueを除外した武器強化用抽選テーブル
		constexpr std::array<RarityWeight, 4> kRarityWeights = {
			RarityWeight{ Rarity::Uncommon,  60 },
			RarityWeight{ Rarity::Rare,      25 },
			RarityWeight{ Rarity::Epic,      10 },
			RarityWeight{ Rarity::Legendary, 5 },
		};

		/// @brief レアリティの日本語表示名を取得
		/// @param rarity 表示名を取得するレアリティ
		/// @return レアリティの日本語表示名
		const char* GetRarityDisplayName(Rarity rarity) {
			switch (rarity) {
			case Rarity::Uncommon:  return "アンコモン";
			case Rarity::Rare:      return "レア";
			case Rarity::Epic:      return "エピック";
			case Rarity::Legendary: return "レジェンダリー";
			default:                return "不明";
			}
		}

		/// @brief レアリティの表示色を取得
		/// @param rarity 表示色を取得するレアリティ
		/// @return RGBA形式の表示色
		std::array<float, 4> GetRarityDisplayColor(Rarity rarity) {
			switch (rarity) {
			case Rarity::Uncommon:  return { 0.20f, 0.85f, 0.30f, 1.0f };
			case Rarity::Rare:      return { 0.20f, 0.55f, 1.00f, 1.0f };
			case Rarity::Epic:      return { 0.70f, 0.30f, 1.00f, 1.0f };
			case Rarity::Legendary: return { 1.00f, 0.85f, 0.15f, 1.0f };
			default:                return { 1.00f, 1.00f, 1.00f, 1.0f };
			}
		}

		/// @brief 二つの強化加算値が表示精度内で一致するか確認
		/// @param left 比較する加算値
		/// @param right 比較する加算値
		/// @return 一致する場合はtrue
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

			// 大きなレベル差でも未処理回数の整数Overflowを防止
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

		// レベル低下時も未処理回数を維持して次回差分の比較基準だけを同期
		previousPlayerLevel_ = currentPlayerLevel;

		if (!choices_.empty() && inventory.GetRevision() != choiceInventoryRevision_) {

			// 装備変更前の候補を選択できないよう表示候補を破棄
			choices_.clear();
			hasGenerationAttempted_ = false;
			Logger::Output("[Application] 装備状態が変化したため武器アップグレード候補を更新します。", Logger::Level::Debug);
		}

		if (!IsUpgrading() || !choices_.empty()) {
			return;
		}

		const bool inventoryChanged = !hasGenerationAttempted_ ||
			lastGenerationAttemptRevision_ != inventory.GetRevision();

		// 生成失敗を毎フレーム再試行せず状態変化時だけ再生成
		if (levelIncreased || inventoryChanged) {
			GenerateChoices(inventory);
		}
	}

	void UpgradeSystem::RequestUpgrade(const Inventory& inventory) {

		// 大量の未処理要求が重なった場合も整数Overflowを防止
		if (pendingUpgradeCount_ == std::numeric_limits<int>::max()) {
			Logger::Output("[Application] 未処理アップグレード回数が上限に達しているため追加できません。", Logger::Level::Warning);
			return;
		}

		++pendingUpgradeCount_;
		if (choices_.empty()) {

			// 選択待機へ直ちに遷移できるよう現在の装備状態から候補を生成
			GenerateChoices(inventory);
		}

		Logger::Output("[Application] Chest開封による武器アップグレードを追加しました。", Logger::Level::Debug);
	}

	bool UpgradeSystem::GenerateChoices(const Inventory& inventory) {
		hasGenerationAttempted_ = true;
		lastGenerationAttemptRevision_ = inventory.GetRevision();
		choices_.clear();

		std::vector<Projectile::Type> candidateWeapons;
		candidateWeapons.reserve(Projectile::kPlayableWeaponTypes.size());

		// 所持武器は強化可能なもの、未所持武器は空きSlotがある場合だけ候補化
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

		// 候補ごとに抽選済み武器を除外して三種類の武器を保証
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

				// 未所持武器はステータス強化ではなく新規装備候補として構築
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

				// 不完全な候補群を表示しないよう生成全体を取り消し
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

		// 累積区間へ抽選値を対応付けて重み付きレアリティを決定
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

		// 候補種別ごとに現在の装備状態を再検証してから適用
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

			// 表示後の設定変更や不正な候補改変を適用直前の再計算で拒否
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

		// 一回分を消費し、残数がある場合は新しい世代の候補を即時生成
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
