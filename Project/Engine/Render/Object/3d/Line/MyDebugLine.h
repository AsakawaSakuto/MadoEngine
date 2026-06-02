#pragma once
#include "DebugLineManager.h"
#include "Utility/Collider/Shape.h"
#include "Math/Vector4.h"
#include "Utility/Camera/Camera.h"

namespace MyDebugLine {
    /// @brief 形状を追加（統一API）
    /// @param shape 描画する形状（AABB, OBB, Sphere, OvalSphere, Plane, Segment, Line）
    /// @param color 描画色（デフォルト：白）
    inline void AddShape(const Shape& shape, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f }) {
        DebugLineManager::GetInstance().AddShape(shape, color);
    }

    /// @brief グリッド描画
    /// @param size グリッドサイズ
    /// @param divisions 分割数
    /// @param color 描画色（デフォルト：白）
    inline void AddGrid(float size, int divisions, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f }) {
        DebugLineManager::GetInstance().AddGrid(size, divisions, color);
    }

    /// @brief すべての線を描画
    /// @param camera 使用するカメラ
    inline void Draw(Camera& camera) {
        DebugLineManager::GetInstance().Draw(camera);
    }
}