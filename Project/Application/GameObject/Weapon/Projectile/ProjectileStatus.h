#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "../WeaponStatus.h"
#include "MathHeaders.h"

namespace Projectile {
	
	struct  InitializeDesc {
		std::uint64_t projectileId = 0; // 投射物の一意なID
		std::string projectileName;     // 投射物の名前
		int projectileCount;            // 投射物の生成順序番号
		Vector3 ownerPosition;          // 投射物の生成元の座標
		Vector3 targetPosition;         // 投射物の目標座標

		float damage = 10.0f;           // 投射物のダメージ量
		float moveSpeed = 10.0f;        // 投射物の移動速度
		float sizeRate = 1.0f;          // 投射物のサイズ倍率
		float lifeTime = 5.0f;          // 投射物の寿命
		int bounceCount = 0;             // 投射物の残り跳弾回数
		int penetrationCount = 0;        // 投射物の残り貫通回数

		float explotionDamageDecreaseRate = 10.0f; // 爆発ダメージの減衰率（1.0fで減衰なし、0.0fで爆発ダメージなし）
		float explosionRadius = 1.0f;              // 爆発の半径
	};

	/// @brief 武器の種類を表す列挙型
	enum class Type {
		None,

		Axe,
		FireBall,
		Pistol,
		Rock,

		Explosion, // 爆発で使用される、武器ではない
	};

	// ゲームロジックで使用できる武器を一か所で管理
	inline constexpr std::array<Type, 4> kPlayableWeaponTypes = {
		Type::Pistol,
		Type::Rock,
		Type::FireBall,
		Type::Axe,
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
		case Type::Rock:     return "Rock";
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
	/// @return 使用可能な武器種類の場合はtrueを返す
	inline bool IsPlayableWeaponType(Type type) {
		for (const Type playableType : kPlayableWeaponTypes) {
			if (playableType == type) {
				return true;
			}
		}

		return false;
	}

	/// @brief 武器種類の日本語表示名を取得
	/// @param type 表示名を取得する武器種類
	/// @return 武器種類の日本語表示名
	inline const char* ProjectileTypeToDisplayName(Type type) {
		switch (type) {
		case Type::None:     return "None";
		case Type::Axe:      return "Axe";
		case Type::FireBall: return "FireBall";
		case Type::Pistol:   return "Pistol";
		case Type::Rock:     return "Rock";
		default:             return "Unknown";
		}
	}
}
