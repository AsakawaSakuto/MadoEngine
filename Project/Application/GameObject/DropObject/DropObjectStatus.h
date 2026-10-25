#pragma once

namespace DropObject {

	enum class Type {
		Exp,
		Money,

		Heal,
		AttackUpper,
		SpeedUpper,
		DefuseUpper,
	};

	/// @brief 特殊Dropの出現確率を決定する発生元
	enum class SpecialDropSource {
		Jar,
		EliteEnemy,
	};

	/// @brief DropObjectの種類を名前へ変換
	/// @param type 変換するDropObjectの種類
	/// @return DropObjectの名前
	inline const char* DropObjectTypeToString(Type type) {
		switch (type) {
		case Type::Exp:         return "Exp";
		case Type::Money:       return "Money";
		case Type::Heal:        return "Heal";
		case Type::AttackUpper: return "AttackUpper";
		case Type::SpeedUpper:  return "SpeedUpper";
		case Type::DefuseUpper: return "DefuseUpper";
		default:                return "Unknown";
		}
	}
}
