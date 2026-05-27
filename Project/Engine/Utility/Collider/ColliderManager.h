#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>
#include "CollisionFunction.h" 
#include "Shape.h"
#include "Math/Vector3.h"

/// @brief コライダーの管理と衝突判定を担当する
class ColliderManager {
public:
	/// @brief 衝突コールバックの型定義
    using CollisionCallback = std::function<void(const std::string& otherTag, const std::string& otherName)>;

    /// @brief コライダーの管理情報
    struct ColliderInfo {
        std::string actorName;         // 固有ID (例: "Enemy_0001")
        std::string tag;               // グループ (例: "Enemy")
        Shape* pShape = nullptr;       // 形状データ (std::variant)
        Vector3* pPosition = nullptr;  // アクターの現在座標へのポインタ
        CollisionCallback onHit = nullptr;
    };

    static ColliderManager* GetInstance() {
        static ColliderManager instance;
        return &instance;
    }

    // --- 公開インターフェース ---

    /// @brief コライダーを登録する
	/// @param name 識別名（例: "Enemy_0001"）
	/// @param tag グループ（例: "Enemy"）
	/// @param shape 形状データ（AABB, OBB, Sphere, OvalSphere, Plane, Segment, Lineのいずれか）
	/// @param pPos アクターの現在座標へのポインタ（Shape内のcenterを自動更新するために必要）
	/// @param callback 衝突時のコールバック関数（省略可）
    void RegisterCollider(const std::string& name, const std::string& tag, Shape* pShape, Vector3* pPos, CollisionCallback callback = nullptr);

    /// @brief コライダーを削除する（デストラクタで必ず呼ぶ）
    void RemoveCollider(const std::string& name);

    /// @brief 衝突ルールを登録する（初期化時に呼ぶ。「Enemy」vs「Enemy」も可能）
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 衝突解決（めり込み防止）を有効にするか（必要に応じてtrueにする。デフォルトはfalse）
    void RegisterCollisionPair(const std::string& tagA, const std::string& tagB, bool enableResolve = false);

	/// @brief 特定の個体同士が衝突しているか
	/// @param nameA 個体Aの識別名
	/// @param nameB 個体Bの識別名
    bool IsHitName(const std::string& nameA, const std::string& nameB);

    /// @brief 特定の個体が、指定したTagを持つ誰かと衝突しているか
    /// @param name 個体の識別名
    /// @param targetTag 対象のタグ
    bool IsHitWithTag(const std::string& name, const std::string& targetTag);

    /// @brief タグ同士の衝突判定
    /// @param tagA タグA
    /// @param tagB タグB
    bool IsHitTags(const std::string& tagA, const std::string& tagB);

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
    std::unordered_map<std::string, std::unordered_map<std::string, CollisionRule>> m_matrix;

    // --- 内部処理 ---
    void SyncPositions();
    bool CheckVariantCollision(const Shape& shapeA, const Shape& shapeB);
    void ResolveOverlap(ColliderInfo& a, ColliderInfo& b);
};