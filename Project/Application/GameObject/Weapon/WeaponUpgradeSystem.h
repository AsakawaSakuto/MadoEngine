#pragma once
#include "WeaponInventory.h"
#include "Utility/Random.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Weapon {

	/// @brief 武器アップグレード選択肢の種類です。
	enum class UpgradeChoiceType {
		NewWeapon,
		OwnedWeaponUpgrade,
	};

	/// @brief UIへ渡す武器アップグレード選択肢です。
	struct UpgradeChoice {
		UpgradeChoiceType choiceType = UpgradeChoiceType::NewWeapon;
		Projectile::Type weaponType = Projectile::Type::None;
		std::optional<UpgradeStatType> statType;
		std::optional<Rarity> rarity;
		float calculatedAmount = 0.0f;
		std::uint64_t generation = 0;
		std::string weaponDisplayName;
		std::string choiceTypeDisplayName;
		std::string statDisplayName;
		std::string rarityDisplayName;
		std::array<float, 4> rarityDisplayColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	/// @brief Playerレベルに応じた武器アップグレード進行を管理します。
	class UpgradeSystem {
	public:
		/// @brief アップグレードシステムを初期化します。
		/// @param currentPlayerLevel 初期化時点のPlayerレベルです。
		/// @param randomSeed 武器アップグレード抽選用の乱数シードです。
		void Initialize(int currentPlayerLevel, std::uint32_t randomSeed);

		/// @brief Playerレベル差分を検知して選択肢を生成します。
		/// @param currentPlayerLevel 現在のPlayerレベルです。
		/// @param inventory 現在の武器インベントリです。
		void UpdatePlayerLevel(int currentPlayerLevel, const Inventory& inventory);

		/// @brief 武器アップグレード中か確認します。
		/// @return 未処理アップグレードがある場合はtrueを返します。
		bool IsUpgrading() const { return pendingUpgradeCount_ > 0; }

		/// @brief 未処理アップグレード回数を取得します。
		/// @return 未処理アップグレード回数です。
		int GetPendingUpgradeCount() const { return pendingUpgradeCount_; }

		/// @brief 現在の武器アップグレード選択肢を取得します。
		/// @return 現在の選択肢へのconst参照です。
		const std::vector<UpgradeChoice>& GetChoices() const { return choices_; }

		/// @brief 指定した武器アップグレード選択肢を適用します。
		/// @param choiceIndex 適用する選択肢の番号です。
		/// @param generation UIが取得した選択肢の世代番号です。
		/// @param inventory 適用先の武器インベントリです。
		/// @return 選択肢の適用に成功した場合はtrueを返します。
		bool SelectChoice(std::size_t choiceIndex, std::uint64_t generation, Inventory& inventory);

	private:
		/// @brief 現在の装備状態から三つの選択肢を生成します。
		/// @param inventory 候補生成に使用する武器インベントリです。
		/// @return 候補生成に成功した場合はtrueを返します。
		bool GenerateChoices(const Inventory& inventory);

		/// @brief ウェイトに従って武器強化レアリティを抽選します。
		/// @return 抽選した武器強化レアリティです。
		Rarity DrawRarity();

		int previousPlayerLevel_ = 0;
		int pendingUpgradeCount_ = 0;
		std::vector<UpgradeChoice> choices_;
		Random random_;
		std::uint64_t nextGeneration_ = 1;
		std::uint64_t choiceInventoryRevision_ = 0;
		std::uint64_t lastGenerationAttemptRevision_ = 0;
		bool hasGenerationAttempted_ = false;
		bool generationFailureLogged_ = false;
	};
}
