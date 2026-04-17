#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>

#include "EaseType.h"

#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include "../../Math/Vector4.h"

/// <summary>
/// 汎用イージング関数群 float, Vector2, Vector3, Vector4 対応
/// </summary>
namespace MyEasing {

    /// @brief イージング関数を適用
    /// @param t 進行状況 0.0～1.0
    /// @param type イージングタイプ
    /// @return イージング適用済み値 0.0～1.0
    float Apply(float t, EaseType type);

    /// @brief 任意型の補間 Start -> End
    /// @param start      開始ベクトル/値
    /// @param end       終了ベクトル/値
    /// @param t            進行状況 0.0～1.0
    /// @param goType イージングタイプ
    template<typename T>
    T Lerp(const T& start, const T& end, float t, EaseType type = EaseType::Linear) {
        float easedT = Apply(t, type);
        return start + (end - start) * easedT;
    }  

    /// @brief 球面線形補間 Vector3
    /// @param start      開始ベクトル
    /// @param end       終了ベクトル
    /// @param t            進行状況 0.0～1.0
    /// @param type     イージングタイプ
    /// @return 補間されたベクトル
    inline Vector3 Slerp(const Vector3& start, const Vector3& end, float t, EaseType type = EaseType::Linear) {
        float easedT = Apply(t, type);

        Vector3 startNorm = start.Normalized();
        Vector3 endNorm = end.Normalized();

        float dot = startNorm.Dot(startNorm, endNorm);
        dot = std::clamp(dot, -1.0f, 1.0f);

        float theta = std::acos(dot) * easedT;

        Vector3 relative = (endNorm - startNorm * dot).Normalized();

        return startNorm * std::cos(theta) + relative * std::sin(theta);
    }

    /// @brief 球面線形補間 Vector4
    /// @param start      開始ベクトル
    /// @param end       終了ベクトル
    /// @param t            進行状況 0.0～1.0
    /// @param type     イージングタイプ
    /// @return 補間されたベクトル
    inline Vector4 Slerp(const Vector4& start, const Vector4& end, float t, EaseType type = EaseType::Linear) {
        float easedT = Apply(t, type);

        Vector4 startNorm = start.Normalized();
        Vector4 endNorm = end.Normalized();

        float dot = startNorm.Dot(startNorm, endNorm);
        dot = std::clamp(dot, -1.0f, 1.0f);

        float theta = std::acos(dot) * easedT;

        Vector4 relative = (endNorm - startNorm * dot).Normalized();

        return startNorm * std::cos(theta) + relative * std::sin(theta);
    }

    /// @brief 任意型の補間、行って帰ってくる
    /// @param start         開始ベクトル/値
    /// @param end          中間ベクトル/値
    /// @param t               進行状況 0.0～1.0
    /// @param goType    行きのイージング
    /// @param backType 帰りのイージング
    template<typename T>
    T LerpBack(const T& start, const T& end, float t,
        EaseType goType = EaseType::Linear, EaseType backType = EaseType::Linear) {

        // 0〜1 に Clamp
        t = std::clamp(t, 0.0f, 1.0f);

        if (t < 0.5f) {
            // 行き：start → end
            float normalizedT = t * 2.0f;           // 0〜0.5 → 0〜1
            return Lerp<T>(start, end, normalizedT, goType);
        } else {
            // 帰り：end → start
            float normalizedT = (t - 0.5f) * 2.0f;  // 0.5〜1 → 0〜1
            return Lerp<T>(end, start, normalizedT, backType);
        }
    }
}
