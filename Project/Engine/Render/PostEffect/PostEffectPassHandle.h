#pragma once

#include "Utility/Handle/GenerationalHandle.h"
#include <cstdint>

namespace MadoEngine::Render {

struct PostEffectPassHandleTag;
using PostEffectPassHandle = GenerationalHandle<PostEffectPassHandleTag>;

/// @brief ポストエフェクトPassの適用先
enum class PostEffectPassScope : uint32_t {
	Layer,
	Screen,
	Count,
};

static_assert(!PostEffectPassHandle{}.IsValid());

} // namespace MadoEngine::Render
