#pragma once
#include "WeaponStatus.h"
#include <nlohmann/json_fwd.hpp>

namespace Weapon {

	/// @brief アップグレード値をJsonへ変換
	/// @param value 変換するアップグレード値
	/// @return 変換後のJson
	nlohmann::json UpgradeValueToJson(const UpgradeValue& value);

	/// @brief Jsonからアップグレード値を読み込み
	/// @param json 読み込み元のJson
	/// @param value 読み込み先のアップグレード値
	/// @return 有効な値を読み込めた場合はtrue
	bool UpgradeValueFromJson(const nlohmann::json& json, UpgradeValue& value);

	/// @brief 継続ダメージ型状態異常の強化設定をJsonへ変換
	/// @param status 変換する状態異常強化設定
	/// @return 変換後のJson
	nlohmann::json DamageStatusEffectUpgradeStatusToJson(
		const DamageStatusEffectUpgradeStatus& status);

	/// @brief 凍結状態異常の強化設定をJsonへ変換
	/// @param status 変換する状態異常強化設定
	/// @return 変換後のJson
	nlohmann::json FrozenStatusEffectUpgradeStatusToJson(
		const FrozenStatusEffectUpgradeStatus& status);

	/// @brief 武器の状態異常強化設定をJsonへ変換
	/// @param status 変換する状態異常強化設定
	/// @return 使用する状態異常だけを格納したJson
	nlohmann::json StatusEffectUpgradeStatusToJson(const StatusEffectUpgradeStatus& status);

	/// @brief Jsonから継続ダメージ型状態異常の強化設定を読み込み
	/// @param json 読み込み元のJson
	/// @param status 読み込み先の状態異常強化設定
	/// @return 有効な設定を読み込めた場合はtrue
	bool DamageStatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		DamageStatusEffectUpgradeStatus& status);

	/// @brief Jsonから凍結状態異常の強化設定を読み込み
	/// @param json 読み込み元のJson
	/// @param status 読み込み先の状態異常強化設定
	/// @return 有効な設定を読み込めた場合はtrue
	bool FrozenStatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		FrozenStatusEffectUpgradeStatus& status);

	/// @brief Jsonから武器の状態異常強化設定を読み込み
	/// @param json 読み込み元のJson
	/// @param status 読み込み先の状態異常強化設定
	/// @return 有効な設定を読み込めた場合はtrue
	bool StatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		StatusEffectUpgradeStatus& status);

	/// @brief 武器の初期ステータスをJsonへ変換
	/// @param status 変換する初期ステータス
	/// @return 変換後のJson
	nlohmann::json UpgradeStatusToJson(const UpgradeStatus& status);

	/// @brief Jsonから武器の初期ステータスを読み込み
	/// @param json 読み込み元のJson
	/// @param status 読み込み先の初期ステータス
	/// @return 有効なステータスを読み込めた場合はtrue
	bool UpgradeStatusFromJson(const nlohmann::json& json, UpgradeStatus& status);

} // namespace Weapon
