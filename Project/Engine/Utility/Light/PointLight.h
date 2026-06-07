#pragma once
#include"Math/Vector3.h"
#include"Math/Vector4.h"

struct PointLight {
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };    // ライトの色
    Vector3 position = { 0.0f, 0.0f, 0.0f }; // ライトの位置
    float pad1;
    float intensity = 1.0f;  // 輝度
    float radius = 1.0f;     // 光の届く最大距離
    float decay = 1.0f;      // 減衰の指数
    uint32_t useLight = 0;
};