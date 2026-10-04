#pragma once
#include "GameObject/Rarity.h"
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace Relic {

	using Id = std::string;

	/// @brief レリック効果の適用先ステータス
	enum class Stat {
		OutgoingDamage,
		MaxHealth,
		MovementSpeed,
		WeaponShotMaxCount,
		MoneyGain,
		HealthRegenerationSpeed,
		ExpGain,
		ProjectileCriticalChance,
		WeaponShotCooldown,
		Count,
	};

	/// @brief ステータス補正値の演算方法
	enum class ModifierOperation {
		AddFlat,
		AddPercent,
	};

	/// @brief 一種類のステータス補正定義
	struct StatModifierEffect {
		Stat stat = Stat::OutgoingDamage;
		ModifierOperation operation = ModifierOperation::AddFlat;
		float valuePerStack = 0.0f;
	};

	/// @brief 一種類のレリック定義
	struct Definition {
		Id id;
		std::string displayName;
		std::string description;
		std::string textureName;
		Rarity rarity = Rarity::Uncommon;
		int maxStack = 1;
		std::vector<StatModifierEffect> effects;
	};

	/// @brief 全レリックから集約したステータス補正値
	class ModifierSnapshot {
	public:
		/// @brief 補正値を初期状態へ設定
		ModifierSnapshot();

		/// @brief 補正値を初期状態へ復元
		void Reset();

		/// @brief ステータス補正をスタック数に応じて加算
		/// @param effect 加算対象の効果定義
		/// @param stackCount 適用する所持数
		void Accumulate(const StatModifierEffect& effect, int stackCount);

		/// @brief 基準値へ集約済み補正を適用
		/// @param stat 適用対象ステータス
		/// @param baseValue 補正前の基準値
		/// @return 全補正を適用した値
		float Resolve(Stat stat, float baseValue) const;

		/// @brief 加算値を取得
		/// @param stat 取得対象ステータス
		/// @return 集約済み加算値
		float GetFlat(Stat stat) const;

		/// @brief 割合加算値を取得
		/// @param stat 取得対象ステータス
		/// @return 集約済み割合加算値
		float GetAdditiveRate(Stat stat) const;

	private:
		static constexpr std::size_t kStatCount = static_cast<std::size_t>(Stat::Count);
		std::array<float, kStatCount> flat_{};
		std::array<float, kStatCount> additiveRate_{};
	};

	/// @brief レアリティを表示用文字列へ変換
	/// @param rarity 変換対象のレアリティ
	/// @return 表示用文字列
	const char* ToDisplayName(Rarity rarity);

}
