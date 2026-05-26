#pragma once
#include <variant>
#include "Utility/Collider/Shape/AABB.h"
#include "Utility/Collider/Shape/OBB.h"
#include "Utility/Collider/Shape/Sphere.h"
#include "Utility/Collider/Shape/OvalSphere.h"
#include "Utility/Collider/Shape/Plane.h"
#include "Utility/Collider/Shape/Segment.h"
#include "Utility/Collider/Shape/Line.h"
#include "Utility/Collider/Shape/Circle.h"

/// <summary>
/// デバッグ描画用の形状を統一的に扱うための variant型定義
/// </summary>
using Shape = std::variant<
    AABB,
    OBB,
    Sphere,
    OvalSphere,
    Plane,
    Segment,
    Line,
    Circle
>;