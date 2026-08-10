#pragma once

#include "Utility/Handle/GenerationalHandle.h"
#include <type_traits>

namespace MadoEngine {

inline constexpr uint32_t kInvalidObjectHandleIndex = kInvalidGenerationalHandleIndex;

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

/// @brief 削除後の次世代番号を取得
/// @param generation 現在の世代番号
/// @return 0を除外した次の世代番号
[[nodiscard]] constexpr uint32_t NextObjectGeneration(uint32_t generation) {
	return NextGeneration(generation);
}

static_assert(NextObjectGeneration((std::numeric_limits<uint32_t>::max)()) == 1);

} // namespace MadoEngine
