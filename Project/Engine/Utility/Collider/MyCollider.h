#pragma once
#include "ColliderManager.h"

namespace MyCollider {

	using CollisionCallback = ColliderManager::CollisionCallback;

	/// @brief コライダーを登録する
	/// @param name 識別名（例: "Enemy_0001"）
	/// @param tag グループ（例: "Enemy"）
	/// @param pShape 形状データ（AABB, OBB, Sphere, OvalSphere, Plane, Segment, Lineのいずれか）
	/// @param pPos アクターの現在座標へのポインタ（Shape内のcenterを自動更新するために必要）
	/// @param callback 衝突時のコールバック関数（省略可）
	inline void RegisterCollider(const std::string& name, const std::string& tag, Shape* pShape, Vector3* pPos, CollisionCallback callback = nullptr) {
		ColliderManager::GetInstance()->RegisterCollider(name, tag, pShape, pPos, callback);
	}

	/// @brief コライダーを削除する（デストラクタで必ず呼ぶ）
	/// @param name 識別名
	inline void RemoveCollider(const std::string& name) {
		ColliderManager::GetInstance()->RemoveCollider(name);
	}

	/// @brief 衝突ルールを登録する（初期化時に呼ぶ。「Enemy」vs「Enemy」も可能）
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 衝突解決（めり込み防止）を有効にするか（必要に応じてtrueにする。デフォルトはfalse）
	inline void RegisterCollisionPair(const std::string& tagA, const std::string& tagB, bool enableResolve = false) {
		ColliderManager::GetInstance()->RegisterCollisionPair(tagA, tagB, enableResolve);
	}

	/// @brief 特定の個体同士が衝突しているか
	/// @param nameA 個体Aの識別名
	/// @param nameB 個体Bの識別名
	/// @return 衝突していればtrue
	inline bool IsHitName(const std::string& nameA, const std::string& nameB) {
		return ColliderManager::GetInstance()->IsHitName(nameA, nameB);
	}

	/// @brief 特定の個体が、指定したTagを持つ誰かと衝突しているか
	/// @param name 個体の識別名
	/// @param targetTag 対象のタグ
	/// @return 衝突していればtrue
	inline bool IsHitWithTag(const std::string& name, const std::string& targetTag) {
		return ColliderManager::GetInstance()->IsHitWithTag(name, targetTag);
	}

	/// @brief タグ同士の衝突判定
	/// @param tagA タグA
	/// @param tagB タグB
	/// @return 衝突していればtrue
	inline bool IsHitTags(const std::string& tagA, const std::string& tagB) {
		return ColliderManager::GetInstance()->IsHitTags(tagA, tagB);
	}

};