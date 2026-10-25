#pragma once
#include "Math/Vector4.h"

/// @brief RGBA形式の色定数、各成分は0.0fから1.0fの範囲
namespace Color {
    /// @brief 赤
    inline constexpr Vector4 Red{ 1.0f, 0.0f, 0.0f, 1.0f };
    /// @brief 緑
    inline constexpr Vector4 Green{ 0.0f, 1.0f, 0.0f, 1.0f };
    /// @brief 青
    inline constexpr Vector4 Blue{ 0.0f, 0.0f, 1.0f, 1.0f };
    /// @brief 黄
    inline constexpr Vector4 Yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
    /// @brief シアン
    inline constexpr Vector4 Cyan{ 0.0f, 1.0f, 1.0f, 1.0f };
    /// @brief マゼンタ
    inline constexpr Vector4 Magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
    /// @brief 白
    inline constexpr Vector4 White{ 1.0f, 1.0f, 1.0f, 1.0f };
    /// @brief 黒
    inline constexpr Vector4 Black{ 0.0f, 0.0f, 0.0f, 1.0f };
    /// @brief 灰色
    inline constexpr Vector4 Gray{ 0.5f, 0.5f, 0.5f, 1.0f };
    /// @brief オレンジ
    inline constexpr Vector4 Orange{ 1.0f, 0.5f, 0.0f, 1.0f };
    /// @brief 紫
    inline constexpr Vector4 Purple{ 0.5f, 0.0f, 0.5f, 1.0f };
    /// @brief ピンク
    inline constexpr Vector4 Pink{ 1.0f, 0.75f, 0.8f, 1.0f };
    /// @brief 完全な透明色
    inline constexpr Vector4 Transparent{ 0.0f, 0.0f, 0.0f, 0.0f };
}
