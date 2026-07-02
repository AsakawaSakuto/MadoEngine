#pragma once

namespace Weapon {
	
	/// @brief 武器の現在ステータスを管理する構造体
	struct Status {
		float damage = 10;        // 武器のダメージ量
		float size = 1.0f;       // 武器のサイズ
		float shotMaxCount = 1.0f; // 武器の最大射撃数
		float shotNowCount = 1.0f; // 武器の現在射撃数
		float shotInterval = 0.5f;   // 武器の射撃間隔
		float shotCooldown = 0.0f;   // 武器の射撃クールダウン

		float criticalChance = 0.1f; // 武器のクリティカル率
		float criticalDamage = 1.5f; // 武器のクリティカルダメージ倍率

		float bounceCount = 0.1f; // 武器のバウンス率
		float penetrationCount = 0.1f; // 武器の貫通率
		float knockbackPower = 1.0f; // 武器のノックバック力
		float lifeTime = 5.0f; // 武器の弾の寿命

		int upgradeLevel = 0; // 武器のアップグレードレベル
		int killCount = 0; // 武器のキル数
	};
}