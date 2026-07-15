#include "StaticBVH.h"
#include <algorithm>
#include <cfloat>

#undef min
#undef max

namespace {

/// @brief 最小最大座標からAABBを作成する
/// @param min 最小座標
/// @param max 最大座標
/// @return 作成したAABB
AABB MakeBounds(const Vector3& min, const Vector3& max) {
	AABB bounds;
	bounds.center = { 0.0f, 0.0f, 0.0f };
	bounds.min = min;
	bounds.max = max;
	return bounds;
}

/// @brief 2つのAABBを結合する
/// @param a AABB
/// @param b AABB
/// @return 2つのAABBを含むAABB
AABB MergeBounds(const AABB& a, const AABB& b) {
	const Vector3 aMin = a.GetMinWorld();
	const Vector3 aMax = a.GetMaxWorld();
	const Vector3 bMin = b.GetMinWorld();
	const Vector3 bMax = b.GetMaxWorld();

	return MakeBounds(
		{
			std::min(aMin.x, bMin.x),
			std::min(aMin.y, bMin.y),
			std::min(aMin.z, bMin.z)
		},
		{
			std::max(aMax.x, bMax.x),
			std::max(aMax.y, bMax.y),
			std::max(aMax.z, bMax.z)
		}
	);
}

/// @brief AABB同士が交差しているか判定する
/// @param a AABB
/// @param b AABB
/// @return 交差していればtrueを返す
bool IsIntersectAABB(const AABB& a, const AABB& b) {
	const Vector3 aMin = a.GetMinWorld();
	const Vector3 aMax = a.GetMaxWorld();
	const Vector3 bMin = b.GetMinWorld();
	const Vector3 bMax = b.GetMaxWorld();

	return aMin.x <= bMax.x && aMax.x >= bMin.x &&
		aMin.y <= bMax.y && aMax.y >= bMin.y &&
		aMin.z <= bMax.z && aMax.z >= bMin.z;
}

/// @brief AABBの中心座標を取得する
/// @param bounds 対象AABB
/// @return 中心座標
Vector3 GetBoundsCenter(const AABB& bounds) {
	const Vector3 min = bounds.GetMinWorld();
	const Vector3 max = bounds.GetMaxWorld();
	return (min + max) * 0.5f;
}

/// @brief 指定範囲のAABBを結合する
/// @param entries エントリ配列
/// @param first 開始インデックス
/// @param count エントリ数
/// @return 指定範囲を含むAABB
AABB CalculateRangeBounds(const std::vector<StaticBVH::Entry>& entries, uint32_t first, uint32_t count) {
	Vector3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (uint32_t i = first; i < first + count; ++i) {
		const Vector3 entryMin = entries[i].bounds.GetMinWorld();
		const Vector3 entryMax = entries[i].bounds.GetMaxWorld();
		min.x = std::min(min.x, entryMin.x);
		min.y = std::min(min.y, entryMin.y);
		min.z = std::min(min.z, entryMin.z);
		max.x = std::max(max.x, entryMax.x);
		max.y = std::max(max.y, entryMax.y);
		max.z = std::max(max.z, entryMax.z);
	}

	return MakeBounds(min, max);
}

/// @brief AABBの中心分布が最も広い軸を取得する
/// @param entries エントリ配列
/// @param first 開始インデックス
/// @param count エントリ数
/// @return 0:X, 1:Y, 2:Z
int FindLongestCentroidAxis(const std::vector<StaticBVH::Entry>& entries, uint32_t first, uint32_t count) {
	Vector3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (uint32_t i = first; i < first + count; ++i) {
		const Vector3 center = GetBoundsCenter(entries[i].bounds);
		min.x = std::min(min.x, center.x);
		min.y = std::min(min.y, center.y);
		min.z = std::min(min.z, center.z);
		max.x = std::max(max.x, center.x);
		max.y = std::max(max.y, center.y);
		max.z = std::max(max.z, center.z);
	}

	const Vector3 extent = max - min;
	if (extent.x >= extent.y && extent.x >= extent.z) {
		return 0;
	}
	if (extent.y >= extent.z) {
		return 1;
	}
	return 2;
}

}

void StaticBVH::Clear() {
	entries_.clear();
	nodes_.clear();
}

void StaticBVH::Build(const std::vector<Entry>& entries) {
	entries_ = entries;
	nodes_.clear();

	if (entries_.empty()) {
		return;
	}

	nodes_.reserve(entries_.size() * 2);
	BuildNode(0, static_cast<uint32_t>(entries_.size()));
}

void StaticBVH::Query(const AABB& bounds, std::vector<const std::string*>& outCandidates) const {
	outCandidates.clear();
	if (nodes_.empty()) {
		return;
	}

	std::vector<uint32_t> stack;
	stack.reserve(64);
	stack.push_back(0);

	while (!stack.empty()) {
		const uint32_t nodeIndex = stack.back();
		stack.pop_back();

		const Node& node = nodes_[nodeIndex];
		if (!IsIntersectAABB(bounds, node.bounds)) {
			continue;
		}

		if (node.count > 0) {
			for (uint32_t i = 0; i < node.count; ++i) {
				const Entry& entry = entries_[node.first + i];
				if (IsIntersectAABB(bounds, entry.bounds)) {
					outCandidates.push_back(&entry.colliderName);
				}
			}
			continue;
		}

		if (node.left >= 0) {
			stack.push_back(static_cast<uint32_t>(node.left));
		}
		if (node.right >= 0) {
			stack.push_back(static_cast<uint32_t>(node.right));
		}
	}
}

size_t StaticBVH::GetEntryCount() const {
	return entries_.size();
}

size_t StaticBVH::GetNodeCount() const {
	return nodes_.size();
}

bool StaticBVH::IsEmpty() const {
	return entries_.empty();
}

uint32_t StaticBVH::BuildNode(uint32_t first, uint32_t count) {
	const uint32_t nodeIndex = static_cast<uint32_t>(nodes_.size());
	nodes_.push_back({});

	Node& node = nodes_[nodeIndex];
	node.bounds = CalculateRangeBounds(entries_, first, count);

	if (count <= kLeafEntryCount) {
		node.first = first;
		node.count = count;
		return nodeIndex;
	}

	const int splitAxis = FindLongestCentroidAxis(entries_, first, count);
	const uint32_t mid = first + count / 2;
	std::nth_element(
		entries_.begin() + first,
		entries_.begin() + mid,
		entries_.begin() + first + count,
		[splitAxis](const Entry& a, const Entry& b) {
			return GetBoundsCenter(a.bounds)[splitAxis] < GetBoundsCenter(b.bounds)[splitAxis];
		}
	);

	node.left = static_cast<int>(BuildNode(first, mid - first));
	node.right = static_cast<int>(BuildNode(mid, first + count - mid));
	node.bounds = MergeBounds(nodes_[node.left].bounds, nodes_[node.right].bounds);
	return nodeIndex;
}
