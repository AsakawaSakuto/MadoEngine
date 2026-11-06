#pragma once
#include "Math/Vector3.h"

struct AABB;

/// @brief イベント種別ごとの自動発火時間と接触範囲
struct MapEventObjectSettings {
	float autoInteractionDuration = 1.0f;
	Vector3 colliderMin = { -1.0f, 0.0f, -1.0f };
	Vector3 colliderMax = { 1.0f, 1.0f, 1.0f };

	/// @brief 発火時間とColliderの各軸を有効範囲へ補正
	void Clamp();

	/// @brief 二つのイベント設定の一致を判定
	/// @param rhs 比較対象の設定
	/// @return 全設定が一致する場合はtrue
	bool IsEqualTo(const MapEventObjectSettings& rhs) const;

	/// @brief 指定座標を基準に設定済みのAABBを作成
	/// @param position Object原点のワールド座標
	/// @return ローカルオフセットを反映したAABB
	AABB CreateCollider(const Vector3& position) const;
};
