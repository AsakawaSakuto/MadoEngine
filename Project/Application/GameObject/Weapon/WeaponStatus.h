#pragma once
#include <string>

namespace Weapon {
	
	/// @brief 武器の基本ステータスを管理する構造体
	struct DefaultStatus {
		float damage = 10.0f;        // 武器のダメージ量
								     
		float shotMaxCount = 1.0f;   // 武器の最大射撃数
		int   shotNowCount = 0;      // 武器の現在射撃数
		float shotInterval = 1.0f;   // 武器の射撃間隔
		float shotCooldown = 1.0f;   // 武器の射撃クールダウン

		float criticalChance = 1.0f; // 武器のクリティカル率
	};

	/// @brief 武器の特殊ステータスを管理する構造体
	struct SpecialStatus {
		float criticalDamage = 1.5f;   // 武器のクリティカルダメージ倍率
		float size = 1.0f;             // 武器のサイズ

		float bounceCount = 0.0f;      // 武器の跳弾回数
		float penetrationCount = 0.0f; // 武器の貫通回数

		float knockbackPower = 1.0f;   // 武器のノックバック力
		float lifeTime = 1.0f;         // 武器の弾の寿命

		float speed = 1.0f;            // 武器の弾の速度
	};

	/// @brief 武器の種類を表す列挙型
	enum class Type {
		None,
		Pistol,
		Rock,
	};

	inline std::string WeaponTypeToString(Type type) {
		switch (type) {
		case Type::None:   return "None";
		case Type::Pistol: return "Pistol";
		case Type::Rock:   return "Rock";
		default:           return "Unknown";
		}
	}
}