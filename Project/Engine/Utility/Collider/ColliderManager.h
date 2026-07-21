#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>
#include <vector>
#include "CollisionFunction.h" 
#include "CollisionTag.h"
#include "Shape.h"
#include "StaticBVH.h"
#include "Math/Vector3.h"

/// @brief コライダーの管理と衝突判定を担当する
class ColliderManager {
public:
	/// @brief 衝突コールバックの型定義
	using CollisionCallback = std::function<void(CollisionTag otherTag, const std::string& otherName)>;

	/// @brief 衝突ペアごとの判定設定
	struct CollisionPairSetting {
		bool enableResolve = false; // 押し戻しを有効にするか
		bool enableCCD = false;     // 連続衝突判定を有効にするか
	};

	/// @brief コライダーの管理情報
	/// @brief Collider処理の軽量化確認に使う統計情報です。
	struct ProfileStats {
		double updateTimeMs = 0.0;
		uint32_t bvhQueryCount = 0;
		uint32_t nonBvhQueryCount = 0; // BVHを使用しない場合に必要となる総当たり比較回数
		uint32_t bvhCandidateCount = 0;
		uint32_t narrowPhaseCount = 0;
		uint32_t sphereSlopeNarrowPhaseCount = 0;
		uint32_t staticBVHColliderCount = 0;
		uint32_t staticBVHNodeCount = 0;
	};

	struct ColliderInfo {
		std::string actorName;         // 固有ID (例: "Enemy_0001")
		CollisionTag tag;              // グループ (例: CollisionTag::Enemy)
		ColliderShape* pShape = nullptr;       // 形状データ (std::variant)
		Vector3* pPosition = nullptr;  // アクターの現在座標へのポインタ
		CollisionCallback onHit = nullptr;
		float weight = 0.0f;           // 押し戻されにくさ (0.0: 通常, 1.0: 完全固定)
		Vector3 previousPosition = { 0.0f, 0.0f, 0.0f }; // CCDで使う前回座標
		bool hasPreviousPosition = false;                // 前回座標が有効か
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
	void RegisterCollider(const std::string& name, CollisionTag tag, ColliderShape* pShape, Vector3* pPos, float weight = 0.0f, CollisionCallback callback = nullptr);

	/// @brief コライダーを削除する（デストラクタで必ず呼ぶ）
	void RemoveCollider(const std::string& name);

	/// @brief 登録されているすべてのコライダーを削除する
	void RemoveColliderAll();

	/// @brief 衝突ルールを登録する（初期化時に呼ぶ。Enemy vs Enemy も可能）
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 衝突解決（めり込み防止）を有効にするか（必要に応じてtrueにする。デフォルトはfalse）
	void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve = false);

	/// @brief 衝突ルールを詳細設定付きで登録する
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param setting 押し戻しやCCDの有効状態
	void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, const CollisionPairSetting& setting);

	/// @brief 衝突ルールを押し戻しとCCD指定付きで登録する
	/// @param tagA グループA
	/// @param tagB グループB
	/// @param enableResolve 押し戻しを有効にするか
	/// @param enableCCD 連続衝突判定を有効にするか
	void RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve, bool enableCCD);

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

	/// @brief 指定したタグ同士で地面接触しているかを判定します。
	/// @param selfTag 接地を判定する側のタグです。
	/// @param targetTag 地面として扱う側のタグです。
	/// @return 接地していればtrueを返します。
	bool IsGroundContact(CollisionTag selfTag, CollisionTag targetTag);

	/// @brief Check whether the specified collider is on a slope surface.
	/// @param name Target collider name.
	/// @param targetTag Slope collider tag.
	/// @return True when the collider is touching a slope surface.
	bool IsSlopeGroundContact(const std::string& name, CollisionTag targetTag);

	/// @brief 指定したタグ同士で坂地面に接触しているかを判定します。
	/// @param selfTag 接地を判定する側のタグです。
	/// @param targetTag 坂地面として扱う側のタグです。
	/// @return 坂地面に接地していればtrueを返します。
	bool IsSlopeGroundContact(CollisionTag selfTag, CollisionTag targetTag);

	/// @brief 指定座標の直下にある地表面のY座標を取得します。
	/// @param origin 地表面を探す基準座標です。
	/// @param targetTag 地面として扱う対象タグです。
	/// @param outSurfaceY 見つかった地表面のY座標です。
	/// @param maxDistance 下方向に探索する最大距離です。
	/// @return 地表面が見つかった場合はtrueを返します。
	bool TryGetGroundSurfaceY(const Vector3& origin, CollisionTag targetTag, float& outSurfaceY, float maxDistance);

	/// @brief Sphereコライダーが追従できるSlope上面の中心Y座標を取得する
	/// @param name Sphereコライダーの識別名
	/// @param targetTag Slopeとして扱うタグ
	/// @param outCenterY Sphere中心に設定するY座標の出力先
	/// @param maxSnapDownDistance 下方向に追従できる最大距離
	/// @return 追従できるSlopeが見つかればtrue
	bool TryGetSlopeGroundCenterY(const std::string& name, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance = 1.0f);

	/// @brief 指定したタグ同士で追従できるSlope上面の中心Y座標を取得します。
	/// @param selfTag Sphereコライダーとして扱う側のタグです。
	/// @param targetTag Slopeとして扱う側のタグです。
	/// @param outCenterY Sphere中心に設定するY座標の出力先です。
	/// @param maxSnapDownDistance 下方向に追従できる最大距離です。
	/// @return 追従できるSlopeが見つかればtrueを返します。
	bool TryGetSlopeGroundCenterY(CollisionTag selfTag, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance = 1.0f);

	/// @brief Sphereコライダーが接地しているSlope上面の法線を取得する
	/// @param name Sphereコライダーの識別名
	/// @param targetTag Slopeとして扱うタグ
	/// @param outNormal Slope上面の法線の出力先
	/// @return 接地しているSlopeが見つかればtrue
	bool TryGetSlopeGroundNormal(const std::string& name, CollisionTag targetTag, Vector3& outNormal);

	/// @brief 指定したタグ同士で接地しているSlope上面の法線を取得します。
	/// @param selfTag Sphereコライダーとして扱う側のタグです。
	/// @param targetTag Slopeとして扱う側のタグです。
	/// @param outNormal Slope上面の法線の出力先です。
	/// @return 接地しているSlopeが見つかればtrueを返します。
	bool TryGetSlopeGroundNormal(CollisionTag selfTag, CollisionTag targetTag, Vector3& outNormal);

	/// @brief 毎フレームの更新処理（裏で呼ぶ）
	void Update();

	/// @brief Collider処理の統計情報を取得します。
	/// @return 直近のCollider処理統計です。
	const ProfileStats& GetProfileStats() const;

#ifdef USE_IMGUI
	/// @brief Collider処理の統計情報をImGuiへ表示します。
	void DrawImGui();
#endif // USE_IMGUI

private:
	ColliderManager() = default;
	~ColliderManager() = default;

	struct CollisionRule {
		bool isRegistered = false;
		bool enableResolve = false;
		bool enableCCD = false;
	};

	// 管理データ
	std::unordered_map<std::string, ColliderInfo> m_colliders;
	std::unordered_map<CollisionTag, std::vector<std::string>> m_colliderNamesByTag;
	std::unordered_map<CollisionTag, std::unordered_map<CollisionTag, CollisionRule>> m_matrix;
	StaticBVH staticTerrainBVH_;
	std::vector<StaticBVH::Entry> staticTerrainEntries_;
	std::vector<const std::string*> bvhQueryResults_;
	bool isStaticTerrainBVHDirty_ = true;
	ProfileStats profileStats_;

	// --- 内部処理 ---
	void SyncPositions();
	bool CheckVariantCollision(const ColliderShape& shapeA, const ColliderShape& shapeB);
	bool TryGetColliderBounds(const ColliderInfo& info, AABB& outBounds) const;
	bool IsStaticTerrainTag(CollisionTag tag) const;
	bool ShouldUseStaticTerrainBVH(CollisionTag tagA, CollisionTag tagB) const;
	void MarkStaticTerrainBVHDirtyIfNeeded(CollisionTag tag);
	void RebuildStaticTerrainBVHIfNeeded();

	/// @brief 静的地形BVHを検索し、BVH使用時と未使用時の検索統計を更新する
	/// @param bounds 検索範囲
	/// @param targetTag 総当たり時の比較対象となる静的地形タグ
	/// @param outCandidates BVH検索で見つかった候補の出力先
	void QueryStaticTerrainBVH(const AABB& bounds, CollisionTag targetTag, std::vector<const std::string*>& outCandidates);
	void ProcessCollisionPair(ColliderInfo& a, ColliderInfo& b, const CollisionRule& rule);
	void ProcessStaticTerrainPair(std::vector<ColliderInfo*>& dynamicList, CollisionTag staticTag, const CollisionRule& rule);

	/// @brief 指定したタグのコライダー名一覧を取得します。
	/// @param tag 検索対象の衝突タグです。
	/// @return コライダー名一覧へのポインタを返します。存在しない場合はnullptrを返します。
	const std::vector<std::string>* FindColliderNames(CollisionTag tag) const;

	/// @brief タグ別索引へコライダー名を追加します。
	/// @param tag 追加先の衝突タグです。
	/// @param name 追加するコライダー名です。
	void AddColliderNameToTag(CollisionTag tag, const std::string& name);

	/// @brief タグ別索引からコライダー名を削除します。
	/// @param tag 削除対象の衝突タグです。
	/// @param name 削除するコライダー名です。
	void RemoveColliderNameFromTag(CollisionTag tag, const std::string& name);

	/// @brief 前回座標から現在座標への移動で連続衝突しているか判定する
	/// @param a コライダーA
	/// @param b コライダーB
	/// @param hitTime 衝突時刻の出力先
	/// @return 連続衝突していればtrue
	bool CheckSweptCollision(ColliderInfo& a, ColliderInfo& b, float& hitTime);

	/// @brief CCDで求めた衝突時刻へコライダー位置を戻す
	/// @param a コライダーA
	/// @param b コライダーB
	/// @param hitTime 衝突時刻
	void ApplySweptCollision(ColliderInfo& a, ColliderInfo& b, float hitTime);

	/// @brief 現在座標を次回CCD用の前回座標として保存する
	void StorePreviousPositions();

	void ResolveOverlap(ColliderInfo& a, ColliderInfo& b);
};
