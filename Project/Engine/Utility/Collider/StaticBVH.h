#pragma once
#include "Utility/Collider/Shape/AABB.h"
#include <string>
#include <vector>

/// @brief 静的Colliderの外接AABBだけを保持するBVH
class StaticBVH {
public:
	/// @brief BVHに登録する静的Colliderの検索用エントリ
	struct Entry {
		std::string colliderName;
		AABB bounds;
	};

	/// @brief BVHを空にする
	void Clear();

	/// @brief 静的ColliderのAABB一覧からBVHを構築する
	/// @param entries 構築に使う静的ColliderのAABB一覧
	void Build(const std::vector<Entry>& entries);

	/// @brief 指定AABBと交差するCollider名を検索する
	/// @param bounds 検索するAABB
	/// @param outCandidates 検索結果を格納する配列、毎回clearされる
	void Query(const AABB& bounds, std::vector<const std::string*>& outCandidates) const;

	/// @brief 登録済みCollider数を取得する
	/// @return 登録済みCollider数
	size_t GetEntryCount() const;

	/// @brief BVHノード数を取得する
	/// @return BVHノード数
	size_t GetNodeCount() const;

	/// @brief BVHが空かどうかを取得する
	/// @return 空であればtrueを返す
	bool IsEmpty() const;

private:
	struct Node {
		AABB bounds;
		int left = -1;
		int right = -1;
		uint32_t first = 0;
		uint32_t count = 0;
	};

	static constexpr uint32_t kLeafEntryCount = 4;

	std::vector<Entry> entries_;
	std::vector<Node> nodes_;

	uint32_t BuildNode(uint32_t first, uint32_t count);
};
