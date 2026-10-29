#include "WeaponUpgradeSystem.h"
#include "Audio/MyAudio.h"
#include "GameObject/Player/Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Weapon {
	namespace {
		constexpr std::size_t kChoiceCount = 3;
		constexpr const char* kUpgradeTimingSoundKey = "UpgradeTiming";

		/// @brief 二つの強化値が表示精度内で一致するか確認
		/// @param left 比較する強化値
		/// @param right 比較する強化値
		/// @return 一致する場合はtrue
		bool IsSameUpgradeValue(float left, float right) {
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
		rerollCost_ = kInitialRerollCost;
		rerollPayments_.clear();
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

		Logger::Output("[Application] 外部イベントによる武器アップグレードを追加しました。", Logger::Level::Debug);
	}

	bool UpgradeSystem::RerollChoices(const Inventory& inventory, Player::Base& player) {
		if (!IsUpgrading() || choices_.empty()) {
			return false;
		}

		const int paidCost = rerollCost_;
		if (!player.CanAfford(paidCost)) {
			Logger::Output(
				"[Application] 所持金が不足しているため武器アップグレード候補をリロールできません。",
				Logger::Level::Debug
			);
			return false;
		}

		// 候補生成の失敗時に料金だけを失わないよう再抽選の成立後に支払いを確定
		if (!GenerateChoices(inventory)) {
			return false;
		}

		if (!player.TrySpendMoney(paidCost)) {
			Logger::Output(
				"[Application] リロール成立後の所持金消費に失敗しました。",
				Logger::Level::Error
			);
			return false;
		}

		rerollPayments_.push_back(paidCost);

		// 長時間のプレイでも整数Overflowを起こさない範囲で次回料金を増加
		if (rerollCost_ <= (std::numeric_limits<int>::max)() - kRerollCostIncrement) {
			rerollCost_ += kRerollCostIncrement;
		} else {
			rerollCost_ = (std::numeric_limits<int>::max)();
		}

		Logger::Output(
			"[Application] 所持金" + std::to_string(paidCost) +
			"を消費して武器アップグレード候補をリロールしました。",
			Logger::Level::Application
		);
		return true;
	}

	std::vector<int> UpgradeSystem::ConsumeRerollPayments() {
		std::vector<int> payments;

		// Debug用の無料再抽選を含めず支払い成功分だけを一度配送
		payments.swap(rerollPayments_);
		return payments;
	}

	bool UpgradeSystem::RerollChoicesForDebug(const Inventory& inventory) {
		if (!IsUpgrading() || choices_.empty()) {
			return false;
		}

		// デバッグ操作ではPlayerの所持金と通常リロール料金を変更せず候補だけを更新
		return GenerateChoices(inventory);
	}

	bool UpgradeSystem::GenerateChoices(const Inventory& inventory) {
		hasGenerationAttempted_ = true;
		lastGenerationAttemptRevision_ = inventory.GetRevision();

		std::vector<Projectile::Type> newWeaponCandidates;
		std::vector<Projectile::Type> ownedWeaponCandidates;
		newWeaponCandidates.reserve(Projectile::kPlayableWeaponTypes.size());
		ownedWeaponCandidates.reserve(Projectile::kPlayableWeaponTypes.size());

		// 種類ごとの候補数が出現確率へ影響しないよう新規武器と所持武器強化を分離
		for (const Projectile::Type weaponType : Projectile::kPlayableWeaponTypes) {
			const BaseWeapon* weapon = inventory.GetWeapon(weaponType);
			if (weapon) {
				if (!weapon->GetSelectableUpgradeStatTypes().empty()) {
					ownedWeaponCandidates.push_back(weaponType);
				}
			} else if (inventory.HasEmptySlot()) {
				newWeaponCandidates.push_back(weaponType);
			}
		}

		if (newWeaponCandidates.size() + ownedWeaponCandidates.size() < kChoiceCount) {
			if (!generationFailureLogged_) {
				Logger::Output("[Application] 異なる武器を対象とした三つの強化候補を生成できません。", Logger::Level::Error);
				generationFailureLogged_ = true;
			}
			return false;
		}

		const std::uint64_t generation = nextGeneration_;
		std::vector<UpgradeChoice> generatedChoices;
		generatedChoices.reserve(kChoiceCount);

		for (std::size_t choiceIndex = 0; choiceIndex < kChoiceCount; ++choiceIndex) {

			// 両方に候補がある間は50%ずつで種類を決定し、片方が尽きた場合は残る側を選択
			std::vector<Projectile::Type>* candidateWeapons = &ownedWeaponCandidates;
			if (ownedWeaponCandidates.empty()) {
				candidateWeapons = &newWeaponCandidates;
			} else if (!newWeaponCandidates.empty() && random_.Int(0, 1) == 0) {
				candidateWeapons = &newWeaponCandidates;
			}

			// 選択した種類の中では等確率で抽選し、抽選済み武器を除外して三種類の武器を保証
			const int randomIndex = random_.Int(0, static_cast<int>(candidateWeapons->size()) - 1);
			const Projectile::Type weaponType = (*candidateWeapons)[static_cast<std::size_t>(randomIndex)];
			candidateWeapons->erase(candidateWeapons->begin() + randomIndex);

			UpgradeChoice choice;
			choice.weaponType = weaponType;
			choice.generation = generation;
			choice.weaponDisplayName = Projectile::ProjectileTypeToDisplayName(weaponType);
			choice.weaponDescription = Projectile::ProjectileTypeToDescription(weaponType);

			const BaseWeapon* weapon = inventory.GetWeapon(weaponType);
			if (!weapon) {

				// 未所持武器はステータス強化ではなく新規装備候補として構築
				choice.choiceType = UpgradeChoiceType::NewWeapon;
				choice.choiceTypeDisplayName = "新しい武器を装備";
				generatedChoices.push_back(std::move(choice));
				continue;
			}

			const std::vector<UpgradeStatType> selectableStats = weapon->GetSelectableUpgradeStatTypes();
			if (selectableStats.empty()) {
				if (!generationFailureLogged_) {
					Logger::Output("[Application] 所持武器に強化対象ステータスがありません。", Logger::Level::Error);
					generationFailureLogged_ = true;
				}
				return false;
			}

			const int statIndex = random_.Int(0, static_cast<int>(selectableStats.size()) - 1);
			const UpgradeStatType statType = selectableStats[static_cast<std::size_t>(statIndex)];
			const Rarity rarity = DrawRarity();
			const UpgradeValue* upgradeValue = weapon->GetUpgradeValue(statType);
			float calculatedAmount = 0.0f;
			if (!upgradeValue || !weapon->CalculateUpgradeAmount(statType, rarity, calculatedAmount)) {

				// 不完全な候補群を表示しないよう生成全体を取り消し
				if (!generationFailureLogged_) {
					Logger::Output("[Application] 武器の強化変化量を計算できません。", Logger::Level::Error);
					generationFailureLogged_ = true;
				}
				return false;
			}

			choice.choiceType = UpgradeChoiceType::OwnedWeaponUpgrade;
			choice.choiceTypeDisplayName = "所持武器を強化";
			choice.statType = statType;
			choice.rarity = rarity;
			choice.currentValue = upgradeValue->value;
			choice.calculatedAmount = calculatedAmount;
			choice.statDisplayName = UpgradeStatTypeToDisplayName(statType);
			choice.rarityDisplayName = GetRarityDisplayName(rarity);
			choice.rarityDisplayColor = GetRarityDefaultColor(rarity);
			generatedChoices.push_back(std::move(choice));
		}

		// 全候補の構築後にまとめて公開して失敗時は現在の選択肢を維持
		choices_ = std::move(generatedChoices);
		choiceInventoryRevision_ = inventory.GetRevision();
		++nextGeneration_;
		generationFailureLogged_ = false;
		Logger::Output("[Application] 武器アップグレード候補を三つ生成しました。", Logger::Level::Debug);

		// 新しいUpgrade選択肢が公開される瞬間に通知SEを再生
		MyAudio::Play(kUpgradeTimingSoundKey);
		return true;
	}

	Rarity UpgradeSystem::DrawRarity() {
		int totalWeight = 0;
		for (const RarityDefinition& definition : kRarityDefinitions) {
			totalWeight += definition.selectionWeight;
		}

		const int draw = random_.Int(1, totalWeight);
		int cumulativeWeight = 0;
		Rarity fallbackRarity = Rarity::Uncommon;

		// 累積区間へ抽選値を対応付けて重み付きレアリティを決定
		for (const RarityDefinition& definition : kRarityDefinitions) {
			if (definition.selectionWeight <= 0) {
				continue;
			}
			fallbackRarity = definition.rarity;
			cumulativeWeight += definition.selectionWeight;
			if (draw <= cumulativeWeight) {
				return definition.rarity;
			}
		}

		return fallbackRarity;
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
			const UpgradeValue* currentUpgradeValue = weapon->GetUpgradeValue(*choice.statType);

			// 表示後の現在値・設定変更や不正な候補改変を適用直前の再計算で拒否
			if (!currentUpgradeValue ||
				!IsSameUpgradeValue(currentUpgradeValue->value, choice.currentValue) ||
				!weapon->CalculateUpgradeAmount(*choice.statType, *choice.rarity, recalculatedAmount) ||
				!IsSameUpgradeValue(recalculatedAmount, choice.calculatedAmount)) {
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
