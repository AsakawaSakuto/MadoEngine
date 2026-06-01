#pragma once
#include <string>
#include <functional>

/// @brief 衝突タグの列挙型
enum class CollisionTag {
	PlayerAABB,
	PlayerSphere,
	Enemy,
	Stage,

	Plane,
	Sphere,
	AABB,
	MapBlock,

	Count
};

/// @brief CollisionTag を文字列に変換する
/// @param tag 変換するタグ
/// @return タグ名の文字列
inline std::string CollisionTagToString(CollisionTag tag) {
	switch (tag) {
	case CollisionTag::PlayerAABB: return "PlayerAABB";
	case CollisionTag::PlayerSphere: return "PlayerSphere";
	case CollisionTag::Enemy:  return "Enemy";
	case CollisionTag::Stage:  return "Stage";
	case CollisionTag::Plane:  return "Plane";
	case CollisionTag::Sphere:  return "Sphere";
	case CollisionTag::AABB:     return "AABB";
	case CollisionTag::MapBlock: return "MapBlock";
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