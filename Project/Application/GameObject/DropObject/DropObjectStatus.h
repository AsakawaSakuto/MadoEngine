#pragma once

namespace DropObject {

	enum class Type {
		Exp,
		Money,

		Heal,
	};

	/// @brief DropObjectの種類を名前へ変換
	/// @param type 変換するDropObjectの種類
	/// @return DropObjectの名前
	inline const char* DropObjectTypeToString(Type type) {
		switch (type) {
		case Type::Exp:   return "Exp";
		case Type::Money: return "Money";
		case Type::Heal:  return "Heal";
		default:          return "Unknown";
		}
	}
}
