#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "../WeaponStatus.h"
#include "MathHeaders.h"

namespace Projectile {
	
	struct  InitializeDesc {
		std::uint64_t projectileId = 0;   // 投射物の一意なID
		std::uint64_t sourceWeaponId = 0; // 発射元Weaponインスタンスの識別番号
		std::string projectileName;       // 投射物の名前
		int projectileCount;              // 投射物の生成順序番号
		int shotMaxCount = 1;             // 補正適用後の最大発射数、Swordでは連続回転数
		Vector3 ownerPosition;            // 投射物の生成元の座標
		Vector3 targetPosition;           // 投射物の目標座標

		float damage = 10.0f;        // 投射物のダメージ量
		float criticalChance = 0.0f; // 投射物のクリティカル率
		float criticalDamage = 1.0f; // 投射物のクリティカルダメージ倍率
		float knockbackPower = 0.0f; // 投射物のノックバック力
		float moveSpeed = 10.0f;     // 投射物の移動速度
		float sizeRate = 1.0f;       // 投射物のサイズ倍率
		float lifeTime = 5.0f;       // 投射物の寿命
		int bounceCount = 0;         // 投射物の残り跳弾回数
		int penetrationCount = 0;    // 投射物の残り貫通回数

		Weapon::StatusEffectStatus statusEffects; // 投射物が付与できる状態異常設定

		float explosionDamageDecreaseRate = 10.0f; // 爆発ダメージの減衰率（1.0fで減衰なし、0.0fで爆発ダメージなし）
		float explosionRadius = 1.0f;              // 爆発の半径
	};

	/// @brief 武器の種類を表す列挙型
	enum class Type {
		None,

		Axe,
		FireBall,
		Pistol,
		Bow,
		Orb,
		Trump,
		Eye,
		ToxicBoots,
		Thunder,
		Hammer,
		SnowCrystal,
		Sword,

		Explosion, // 爆発で使用される、武器ではない
	};

	// ゲームロジックで使用できる武器を一か所で管理
	inline constexpr std::array<Type, 12> kPlayableWeaponTypes = {
		Type::Pistol,
		Type::Bow,
		Type::FireBall,
		Type::Axe,
		Type::Orb,
		Type::Trump,
		Type::Eye,
		Type::ToxicBoots,
		Type::Thunder,
		Type::Hammer,
		Type::SnowCrystal,
		Type::Sword,
	};

	/// @brief 武器種類をリソース名へ変換
	/// @param type 変換する武器種類
	/// @return 武器のリソース名
	inline std::string ProjectileTypeToString(Type type) {
		switch (type) {
		case Type::None:     return "None";
		case Type::Axe:      return "Axe";
		case Type::FireBall: return "FireBall";
		case Type::Pistol:   return "Pistol";
		case Type::Bow:      return "Bow";
		case Type::Orb:      return "Orb";
		case Type::Trump:    return "Trump";
		case Type::Eye:      return "Eye";
		case Type::ToxicBoots: return "ToxicBoots";
		case Type::Thunder:   return "Thunder";
		case Type::Hammer:    return "Hammer";
		case Type::SnowCrystal: return "SnowCrystal";
		case Type::Sword:       return "Sword";
		default:             return "Unknown";
		}
	}

	/// @brief 武器種類に対応するJsonファイル名を取得
	/// @param type ファイル名を取得する武器種類
	/// @return 拡張子を除くJsonファイル名
	inline std::string ProjectileTypeToJsonFileName(Type type) {
		if (type == Type::FireBall) {
			return "Fireball";
		}

		return ProjectileTypeToString(type);
	}

	/// @brief 武器種類がゲームで使用可能か確認
	/// @param type 確認する武器種類
	/// @return 使用可能な武器種類の場合はtrue
	inline bool IsPlayableWeaponType(Type type) {
		for (const Type playableType : kPlayableWeaponTypes) {
			if (playableType == type) {
				return true;
			}
		}

		return false;
	}

	/// @brief 常時展開型の武器種類か確認
	/// @param type 確認する武器種類
	/// @return 常時展開型の武器種類の場合はtrue
	inline bool IsPersistentWeaponType(Type type) {
		return type == Type::Eye;
	}

	/// @brief 武器種類の日本語表示名を取得
	/// @param type 表示名を取得する武器種類
	/// @return 武器種類の日本語表示名
	inline const char* ProjectileTypeToDisplayName(Type type) {
		switch (type) {
		case Type::None:       return "なし";
		case Type::Axe:        return "斧";
		case Type::FireBall:   return "火球";
		case Type::Pistol:     return "拳銃";
		case Type::Bow:        return "弓";
		case Type::Orb:        return "スピオーブ";
		case Type::Trump:      return "デストランプ";
		case Type::Eye:        return "魔眼";
		case Type::ToxicBoots: return "毒靴";
		case Type::Thunder:   return "雷";
		case Type::Hammer:    return "ハンマー";
		case Type::SnowCrystal: return "雪の結晶";
		case Type::Sword:       return "剣";
		default:               return "不明";
		}
	}

	/// @brief 武器種類の固有説明を取得
	/// @param type 説明を取得する武器種類
	/// @return 武器種類に対応する固有説明
	inline const char* ProjectileTypeToDescription(Type type) {
		switch (type) {
		case Type::Axe:         return "前方へ投擲して\nその場に滞留する";
		case Type::FireBall:    return "着弾時に範囲攻撃\n稀に火傷を付与";
		case Type::Pistol:      return "近くの敵に向かって\n弾丸を連射する";
		case Type::Bow:         return "近くの敵に向かって\n貫通する矢を放つ";
		case Type::Orb:         return "弧を描くオーブを放つ\n稀に凍結を付与";
		case Type::Trump:       return "1～13のダメージ\n稀にJokerが即死させる";
		case Type::Eye:         return "足元にエリアを展開\n触れた敵にダメージ";
		case Type::ToxicBoots:  return "足元に魔法陣を設置\n触れた敵に毒を付与";
		case Type::Thunder:     return "敵の頭上に雷を生成\n跳弾して敵を攻撃する";
		case Type::Hammer:      return "頭上にハンマーを構え\n敵へ向けて振り下ろす";
		case Type::SnowCrystal: return "前方へ結晶を投げ\n自分のもとへ戻ってくる";
		case Type::Sword:       return "自分を中心に回転\n周囲の敵を薙ぎ払う";
		default:                return "武器の詳細情報なし";
		}
	}
}
