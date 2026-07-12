#pragma once
#include <array>
#include <string>
#include "../WeaponStatus.h"
#include "MathHeaders.h"

namespace Projectile {
	
	struct  InitializeDesc {
		std::string projectileName;
		int projectileCount;
		Vector3 ownerPosition;
		Vector3 targetPosition;
	};


	/// @brief 武器の種類を表す列挙型
	enum class Type {
		None,
		Axe,
		FireBall,
		Pistol,
		Rock,
	};

	// ゲームロジックで使用できる武器を一か所で管理します。
	inline constexpr std::array<Type, 4> kPlayableWeaponTypes = {
		Type::Pistol,
		Type::Rock,
		Type::FireBall,
		Type::Axe,
	};

	/// @brief 武器種類をリソース名へ変換します。
	/// @param type 変換する武器種類です。
	/// @return 武器のリソース名です。
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

	/// @brief 武器種類がゲームで使用可能か確認します。
	/// @param type 確認する武器種類です。
	/// @return 使用可能な武器種類の場合はtrueを返します。
	inline bool IsPlayableWeaponType(Type type) {
		for (const Type playableType : kPlayableWeaponTypes) {
			if (playableType == type) {
				return true;
			}
		}

		return false;
	}

	/// @brief 武器種類の日本語表示名を取得します。
	/// @param type 表示名を取得する武器種類です。
	/// @return 武器種類の日本語表示名です。
	inline const char* ProjectileTypeToDisplayName(Type type) {
		switch (type) {
		case Type::Axe:      return "斧";
		case Type::FireBall: return "ファイアボール";
		case Type::Pistol:   return "ピストル";
		case Type::Rock:     return "岩";
		default:             return "不明な武器";
		}
	}
}
