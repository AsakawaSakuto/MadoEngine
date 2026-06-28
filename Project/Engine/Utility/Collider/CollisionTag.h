#pragma once
#include <string>
#include <functional>

/// @brief 衝突タグの列挙型
enum class CollisionTag {
	PlayerHitBox,
	PlayerMovementSphere,
	
	EnemyHitBox,
	EnemyMovementSphere,

	MapBlock,
	MapSlope,

	MapEventObject,

	Count
};

/// @brief CollisionTag を文字列に変換する
/// @param tag 変換するタグ
/// @return タグ名の文字列
inline std::string CollisionTagToString(CollisionTag tag) {
	switch (tag) {
	case CollisionTag::PlayerHitBox:         return "PlayerHitBox";
	case CollisionTag::PlayerMovementSphere: return "PlayerMovementSphere";

	case CollisionTag::EnemyHitBox:         return "EnemyHitBox";
	case CollisionTag::EnemyMovementSphere: return "EnemyMovementSphere";

	case CollisionTag::MapBlock: return "MapBlock";
	case CollisionTag::MapSlope: return "MapSlope";

	case CollisionTag::MapEventObject: return "MapEventObject";

	default:                     return "Unknown";
	}
}

// unordered_map で CollisionTag をキーとして使用するためのハッシュ特殊化
namespace std {
	template<>
	struct hash<CollisionTag> {
		size_t operator()(CollisionTag tag) const noexcept {
			return std::hash<int>()(static_cast<int>(tag));
		}
	};
}