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
#include "Utility/Collider/Shape/Slope.h"

/// @brief 複数の当たり判定形状をまとめるための型
using ColliderShape = std::variant<
    AABB,
    OBB,
    Sphere,
    OvalSphere,
    Plane,
    Segment,
    Line,
    Circle,
    Slope
>;
