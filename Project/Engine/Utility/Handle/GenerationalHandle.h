#pragma once

#include <cstdint>
#include <limits>

namespace MadoEngine {

inline constexpr uint32_t kInvalidGenerationalHandleIndex = (std::numeric_limits<uint32_t>::max)();

/// @brief 世代番号で再利用済みSlotを識別できる汎用Handle
/// @tparam Tag Handle型を区別するためのタグ
template<class Tag>
struct GenerationalHandle {
	uint32_t index = kInvalidGenerationalHandleIndex;
	uint32_t generation = 0;

	/// @brief Handleが有効値を保持しているか確認
	/// @return generationが0ではなく、indexが無効値ではない場合はtrue
	[[nodiscard]] constexpr bool IsValid() const {
		return generation != 0 && index != kInvalidGenerationalHandleIndex;
	}

	bool operator==(const GenerationalHandle&) const = default;
};

/// @brief 削除後に使用する次の世代番号を取得
/// @param generation 現在の世代番号
/// @return 0を除外した次の世代番号
[[nodiscard]] constexpr uint32_t NextGeneration(uint32_t generation) {
	++generation;
	return generation == 0 ? 1 : generation;
}

static_assert(NextGeneration((std::numeric_limits<uint32_t>::max)()) == 1);

} // namespace MadoEngine
