#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace MadoEngine {

inline constexpr uint32_t kInvalidObjectHandleIndex = std::numeric_limits<uint32_t>::max();

/// @brief 描画オブジェクトを世代付きで参照するHandle
/// @tparam Tag Handle型を区別するためのタグ
template <class Tag>
struct GenerationalHandle {
	uint32_t index = kInvalidObjectHandleIndex;
	uint32_t generation = 0;

	/// @brief Handleが有効値を保持しているか確認する
	/// @return generationが0ではなく、indexが無効値ではない場合はtrue
	[[nodiscard]] constexpr bool IsValid() const {
		return generation != 0 && index != kInvalidObjectHandleIndex;
	}

	bool operator==(const GenerationalHandle&) const = default;
};

struct SpriteHandleTag;
struct TextHandleTag;
struct ModelHandleTag;
struct InstancedModelHandleTag;

using SpriteHandle = GenerationalHandle<SpriteHandleTag>;
using TextHandle = GenerationalHandle<TextHandleTag>;
using ModelHandle = GenerationalHandle<ModelHandleTag>;
using InstancedModelHandle = GenerationalHandle<InstancedModelHandleTag>;

static_assert(!SpriteHandle{}.IsValid());
static_assert(!std::is_same_v<SpriteHandle, TextHandle>);
static_assert(!std::is_same_v<ModelHandle, InstancedModelHandle>);

/// @brief 削除後の次世代番号を取得する
/// @param generation 現在の世代番号
/// @return 0を除外した次の世代番号
[[nodiscard]] constexpr uint32_t NextObjectGeneration(uint32_t generation) {
	++generation;
	return generation == 0 ? 1 : generation;
}

static_assert(NextObjectGeneration(std::numeric_limits<uint32_t>::max()) == 1);

} // namespace MadoEngine
