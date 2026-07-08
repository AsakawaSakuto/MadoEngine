#pragma once
#include <string>
#include <functional>

/// @brief 衝突タグの列挙型
enum class CollisionTag {
	PlayerHitBox,
	PlayerMovementSphere,
	PlayerDropObjectGetSphere,
	PlayerProjectileHitBox,

	EnemyHitBox,
	EnemyMovementSphere,

	MapLimitBox,

	MapBlock,
	MapSlope,

	MapEventObject,

	DropObjectHitBox,

	Count
};

/// @brief CollisionTag を文字列に変換する
/// @param tag 変換するタグ
/// @return タグ名の文字列
inline std::string CollisionTagToString(CollisionTag tag) {
	switch (tag) {
	case CollisionTag::PlayerHitBox:         return "PlayerHitBox";
	case CollisionTag::PlayerMovementSphere: return "PlayerMovementSphere";
	case CollisionTag::PlayerDropObjectGetSphere:    return "PlayerDropObjectGetSphere";

	case CollisionTag::PlayerProjectileHitBox: return "PlayerProjectileHitBox";

	case CollisionTag::EnemyHitBox:         return "EnemyHitBox";
	case CollisionTag::EnemyMovementSphere: return "EnemyMovementSphere";

	case CollisionTag::MapLimitBox: return "MapLimitBox";

	case CollisionTag::MapBlock: return "MapBlock";
	case CollisionTag::MapSlope: return "MapSlope";

	case CollisionTag::MapEventObject: return "MapEventObject";

	case CollisionTag::DropObjectHitBox: return "DropObjectHitBox";

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