#pragma once
#include "Math/Vector3.h"

struct AABB;

/// @brief 装飾種別ごとのCamera遮蔽判定範囲
struct MapDecorationObjectSettings {
	Vector3 colliderMin = { -1.0f, 0.0f, -1.0f };
	Vector3 colliderMax = { 1.0f, 1.0f, 1.0f };

	/// @brief 遮蔽判定範囲の各軸を有限で有効な範囲へ補正
	void Clamp();

	/// @brief 二つの装飾設定の一致を判定
	/// @param rhs 比較対象の設定
	/// @return 全設定が一致する場合はtrue
	bool IsEqualTo(const MapDecorationObjectSettings& rhs) const;

	/// @brief 指定座標を基準に設定済みの遮蔽判定用AABBを作成
	/// @param position 装飾の原点のワールド座標
	/// @return 回転と拡大縮小に依存しない固定オフセットのAABB
	AABB CreateCollider(const Vector3& position) const;
};
