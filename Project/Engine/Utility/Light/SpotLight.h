#pragma once
#include"Math/Vector3.h"
#include"Math/Vector4.h"

struct SpotLight {
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };     // ライトの色
    Vector3 position = { 0.0f, 0.0f, 0.0f }; // ライトの位置
    float intensity = 1.0f;   // 輝度
    Vector3 direction = { 0.0f, -1.0f, 0.0f }; // スポットライトの方向
    float distance = 1.0f;    // ライトの届く最大距離
    float decay = 1.0f;       // 減衰率
    float cosAngle = 0.0f;    // スポットライトの余弦
    float cosFalloffStart = 0.0f;
    uint32_t useLight = 0;
};