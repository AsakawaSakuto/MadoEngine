#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>
#include "CollisionFunction.h" 
#include "CollisionTag.h"
#include "Shape.h"
#include "Math/Vector3.h"

/// @brief コライダーの管理と衝突判定を担当する
class ColliderManager {
public:
	/// @brief 衝突コールバックの型定義
	using CollisionCallback = std::function<void(CollisionTag otherTag, const std::string& otherName)>;

	/// @brief コライダーの管理情報
	struct ColliderInfo {
		std::string actorName;         // 固有ID (例: "Enemy_0001")
		CollisionTag tag;              // グループ (例: CollisionTag::Enemy)
		Shape* pShape = nullptr;       // 形状データ (std::variant)
		Vector3* pPosition = nullptr;  // アクターの現在座標へのポインタ
		CollisionCallback onHit = nullptr;
		float weight = 0.0f;           // 押し戻されにくさ (0.0: 通常, 1.0: 完全固定)
	};

	static ColliderManager& GetInstance() {
		static ColliderManager instance;
		return instance;
	}

	// --- 公開インターフェース ---

	/// @brief コライダーを登録する
	/// @param name 識別名（例: "Enemy_0001"）
	/// @param tag グループ（例: CollisionTag::Enemy）
	/// @param shape 形状データ（AABB, OBB, Sphere, OvalSphere, Plane, Segment, Lineのいずれか）
	/// @param pPos アクターの現在座標へのポインタ（Shape内のcenterを自動更新するために必要）
	/// @param callback 衝突時のコールバック関数（省略可）
	/// @param weight 押し戻されにくさ（0.0: 通常, 1.0: 完全固定。省略可）
	void RegisterCollider(const std::string& name, CollisionTag tag, Shape* pShape, Vector3* pPos, float weight = 0.0f, CollisionCallback callback = nullptr);

	/// @brief コライダーを削除する（デストラクタで必ず呼ぶ）
	void RemoveCollider(const std::string& name);

	/// @brief 衝突ルールを登録する（初期化時に呼ぶ。Enemy vs Enemy も可能）
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 衝突解決（めり込み防止）を有効にするか（必要に応じてtrueにする。デフォルトはfalse）
	void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve = false);

	/// @brief 特定の個体同士が衝突しているか
	/// @param nameA 個体Aの識別名
	/// @param nameB 個体Bの識別名
	bool IsHitName(const std::string& nameA, const std::string& nameB);

	/// @brief 特定の個体が、指定したTagを持つ誰かと衝突しているか
	/// @param name 個体の識別名
	/// @param targetTag 対象のタグ
	bool IsHitWithTag(const std::string& name, CollisionTag targetTag);

	/// @brief タグ同士の衝突判定
	/// @param tagA タグA
	/// @param tagB タグB
	bool IsHitTags(CollisionTag tagA, CollisionTag tagB);

	/// @brief 指定した個体が、対象タグのAABBの上面に乗っているかを判定する
	/// @param name 個体の識別名
	/// @param targetTag 床として扱うタグ
	/// @return 床面に接触していればtrue
	bool IsGroundContact(const std::string& name, CollisionTag targetTag);

	/// @brief Check whether the specified collider is on a slope surface.
	/// @param name Target collider name.
	/// @param targetTag Slope collider tag.
	/// @return True when the collider is touching a slope surface.
	bool IsSlopeGroundContact(const std::string& name, CollisionTag targetTag);

	/// @brief 毎フレームの更新処理（裏で呼ぶ）
	void Update();

private:
	ColliderManager() = default;
	~ColliderManager() = default;

	struct CollisionRule {
		bool isRegistered = false;
		bool enableResolve = false;
	};

	// 管理データ
	std::unordered_map<std::string, ColliderInfo> m_colliders;
	std::unordered_map<CollisionTag, std::unordered_map<CollisionTag, CollisionRule>> m_matrix;

	// --- 内部処理 ---
	void SyncPositions();
	bool CheckVariantCollision(const Shape& shapeA, const Shape& shapeB);
	void ResolveOverlap(ColliderInfo& a, ColliderInfo& b);
};
