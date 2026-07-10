#pragma once
#include "ColliderManager.h"

namespace MyCollider {

	using CollisionCallback = ColliderManager::CollisionCallback;
	using CollisionPairSetting = ColliderManager::CollisionPairSetting;
	using ProfileStats = ColliderManager::ProfileStats;

	/// @brief コライダーを登録する
	/// @param name 識別名（例: "Enemy_0001"）
	/// @param tag グループ（例: CollisionTag::Enemy）
	/// @param pShape 形状データ（AABB, OBB, Sphere, OvalSphere, Plane, Segment, Lineのいずれか）
	/// @param pPos アクターの現在座標へのポインタ（Shape内のcenterを自動更新するために必要）
	/// @param callback 衝突時のコールバック関数（省略可）
	/// @param weight 押し戻しの重み（省略可、デフォルトは0.5f）
	inline void RegisterCollider(const std::string& name, CollisionTag tag, ColliderShape* pShape, Vector3* pPos, float weight = 0.5f, CollisionCallback callback = nullptr) {
		ColliderManager::GetInstance().RegisterCollider(name, tag, pShape, pPos, weight, callback);
	}

	/// @brief コライダーを削除する（デストラクタで必ず呼ぶ）
	/// @param name 識別名
	inline void RemoveCollider(const std::string& name) {
		ColliderManager::GetInstance().RemoveCollider(name);
	}

	/// @brief 衝突ルールを登録する（初期化時に呼ぶ。Enemy vs Enemy も可能）
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 衝突解決（めり込み防止）を有効にするか（必要に応じてtrueにする。デフォルトはfalse）
	inline void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve = false) {
		ColliderManager::GetInstance().RegisterCollisionPair(tagA, tagB, enableResolve);
	}

	/// @brief 衝突ルールを詳細設定付きで登録する
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param setting 押し戻しやCCDの有効状態
	inline void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, const CollisionPairSetting& setting) {
		ColliderManager::GetInstance().RegisterCollisionPair(tagA, tagB, setting);
	}

	/// @brief 衝突ルールを押し戻しとCCD指定付きで登録する
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 押し戻しを有効にするか
	/// @param enableCCD 連続衝突判定を有効にするか
	inline void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve, bool enableCCD) {
		ColliderManager::GetInstance().RegisterCollisionPair(tagA, tagB, enableResolve, enableCCD);
	}

	/// @brief 特定の個体同士が衝突しているか
	/// @param nameA 個体Aの識別名
	/// @param nameB 個体Bの識別名
	/// @return 衝突していればtrue
	inline bool IsHitName(const std::string& nameA, const std::string& nameB) {
		return ColliderManager::GetInstance().IsHitName(nameA, nameB);
	}

	/// @brief 特定の個体が、指定したTagを持つ誰かと衝突しているか
	/// @param name 個体の識別名
	/// @param targetTag 対象のタグ
	/// @return 衝突していればtrue
	inline bool IsHitWithTag(const std::string& name, CollisionTag targetTag) {
		return ColliderManager::GetInstance().IsHitWithTag(name, targetTag);
	}

	/// @brief タグ同士の衝突判定
	/// @param tagA タグA
	/// @param tagB タグB
	/// @return 衝突していればtrue
	inline bool IsHitTags(CollisionTag tagA, CollisionTag tagB) {
		return ColliderManager::GetInstance().IsHitTags(tagA, tagB);
	}

	/// @brief 指定した個体が、対象タグのAABB上面に乗っているかを判定する
	/// @param name 個体の識別名
	/// @param targetTag 床として扱うタグ
	/// @return 床面に接触していればtrue
	inline bool IsGroundContact(const std::string& name, CollisionTag targetTag) {
		return ColliderManager::GetInstance().IsGroundContact(name, targetTag);
	}

	/// @brief 指定したタグ同士で地面接触しているかを判定します。
	/// @param selfTag 接地を判定する側のタグです。
	/// @param targetTag 地面として扱う側のタグです。
	/// @return 接地していればtrueを返します。
	inline bool IsGroundContact(CollisionTag selfTag, CollisionTag targetTag) {
		return ColliderManager::GetInstance().IsGroundContact(selfTag, targetTag);
	}

	/// @brief 指定した個体が、対象タグのスロープ面に接触しているかを判定する
	/// @param name 個体の識別名
	/// @param targetTag スロープとして扱うタグ
	/// @return スロープ面に接触していればtrue
	inline bool IsSlopeGroundContact(const std::string& name, CollisionTag targetTag) {
		return ColliderManager::GetInstance().IsSlopeGroundContact(name, targetTag);
	}

	/// @brief 指定したタグ同士で坂地面に接触しているかを判定します。
	/// @param selfTag 接地を判定する側のタグです。
	/// @param targetTag 坂地面として扱う側のタグです。
	/// @return 坂地面に接地していればtrueを返します。
	inline bool IsSlopeGroundContact(CollisionTag selfTag, CollisionTag targetTag) {
		return ColliderManager::GetInstance().IsSlopeGroundContact(selfTag, targetTag);
	}

	/// @brief 指定座標の直下にある地表面のY座標を取得します。
	/// @param origin 地表面を探す基準座標です。
	/// @param targetTag 地面として扱う対象タグです。
	/// @param outSurfaceY 見つかった地表面のY座標です。
	/// @param maxDistance 下方向に探索する最大距離です。
	/// @return 地表面が見つかった場合はtrueを返します。
	inline bool TryGetGroundSurfaceY(const Vector3& origin, CollisionTag targetTag, float& outSurfaceY, float maxDistance) {
		return ColliderManager::GetInstance().TryGetGroundSurfaceY(origin, targetTag, outSurfaceY, maxDistance);
	}

	/// @brief Sphereコライダーが追従できるSlope上面の中心Y座標を取得する
	/// @param name Sphereコライダーの識別名
	/// @param targetTag Slopeとして扱うタグ
	/// @param outCenterY Sphere中心に設定するY座標の出力先
	/// @param maxSnapDownDistance 下方向に追従できる最大距離
	/// @return 追従できるSlopeが見つかればtrue
	inline bool TryGetSlopeGroundCenterY(const std::string& name, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance = 1.0f) {
		return ColliderManager::GetInstance().TryGetSlopeGroundCenterY(name, targetTag, outCenterY, maxSnapDownDistance);
	}

	/// @brief 指定したタグ同士で追従できるSlope上面の中心Y座標を取得します。
	/// @param selfTag Sphereコライダーとして扱う側のタグです。
	/// @param targetTag Slopeとして扱う側のタグです。
	/// @param outCenterY Sphere中心に設定するY座標の出力先です。
	/// @param maxSnapDownDistance 下方向に追従できる最大距離です。
	/// @return 追従できるSlopeが見つかればtrueを返します。
	inline bool TryGetSlopeGroundCenterY(CollisionTag selfTag, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance = 1.0f) {
		return ColliderManager::GetInstance().TryGetSlopeGroundCenterY(selfTag, targetTag, outCenterY, maxSnapDownDistance);
	}

	/// @brief Sphereコライダーが接地しているSlope上面の法線を取得する
	/// @param name Sphereコライダーの識別名
	/// @param targetTag Slopeとして扱うタグ
	/// @param outNormal Slope上面の法線の出力先
	/// @return 接地しているSlopeが見つかればtrue
	inline bool TryGetSlopeGroundNormal(const std::string& name, CollisionTag targetTag, Vector3& outNormal) {
		return ColliderManager::GetInstance().TryGetSlopeGroundNormal(name, targetTag, outNormal);
	}

	/// @brief 指定したタグ同士で接地しているSlope上面の法線を取得します。
	/// @param selfTag Sphereコライダーとして扱う側のタグです。
	/// @param targetTag Slopeとして扱う側のタグです。
	/// @param outNormal Slope上面の法線の出力先です。
	/// @return 接地しているSlopeが見つかればtrueを返します。
	inline bool TryGetSlopeGroundNormal(CollisionTag selfTag, CollisionTag targetTag, Vector3& outNormal) {
		return ColliderManager::GetInstance().TryGetSlopeGroundNormal(selfTag, targetTag, outNormal);
	}

	/// @brief 登録済みコライダーの衝突判定と押し戻しを更新する
	inline void Update() {
		ColliderManager::GetInstance().Update();
	}

	/// @brief Collider処理の統計情報を取得します。
	/// @return 直近のCollider処理統計です。
	inline const ProfileStats& GetProfileStats() {
		return ColliderManager::GetInstance().GetProfileStats();
	}

#ifdef USE_IMGUI
	/// @brief Collider処理の統計情報をImGuiへ表示します。
	inline void DrawImGui() {
		ColliderManager::GetInstance().DrawImGui();
	}
#endif // USE_IMGUI

	inline void RemoveColliderAll() {
		ColliderManager::GetInstance().RemoveColliderAll();
	}
};
