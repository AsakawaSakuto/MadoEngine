#pragma once
#include <string>
#include <utility>
namespace Weapon {

	/// @brief 武器のアップグレード値を管理する構造体
	struct UpgradeValue {
		float value = 0.0f; // 加算される値
		int maxLevel = 0;   // 最大レベル、0なら強化の選択肢に出現しない
	};

	/// @brief 武器のアップグレードしたステータスを管理する構造体
	struct UpgradeStatus {
		UpgradeValue damage           = { 1.0f, 99 }; // 武器のダメージ量
		UpgradeValue shotMaxCount     = { 1.0f, 99 }; // 武器の最大射撃数
		UpgradeValue shotCooldown     = { 1.0f, 99 }; // 武器の射撃クールダウン
		UpgradeValue criticalChance   = { 1.0f, 99 }; // 武器のクリティカル率
		UpgradeValue criticalDamage   = { 1.0f, 99 }; // 武器のクリティカルダメージ倍率
		UpgradeValue size             = { 1.0f, 99 }; // 武器のサイズ
		UpgradeValue bounceCount      = { 1.0f, 99 }; // 武器の跳弾回数
		UpgradeValue penetrationCount = { 1.0f, 99 }; // 武器の貫通回数
		UpgradeValue knockbackPower   = { 1.0f, 99 }; // 武器のノックバック力
		UpgradeValue lifeTime         = { 1.0f, 99 }; // 武器の弾の寿命
		UpgradeValue speed            = { 1.0f, 99 }; // 武器の弾の速度
	};
}