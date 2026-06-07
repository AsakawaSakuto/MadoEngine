#pragma once
#include"Math/Vector3.h"

struct CameraForGPU {
    Vector3 worldPosition = { 0.0f, 0.0f, 0.0f };
    float padding; // 16バイト整列
};