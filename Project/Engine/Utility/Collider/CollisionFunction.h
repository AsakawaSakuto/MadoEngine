#pragma once
#include "Math/Vector3.h"
#include "Shape.h"
#include <algorithm>
#include <iostream>
#include <cfloat>

#undef min
#undef max

/// <summary>
/// 当たり判定ユーティリティ
/// </summary>
namespace Collision
{
    /// <summary>
    /// Sphere × Sphere 判定
    /// </summary>
    inline bool IsHit(const Sphere& a, const Sphere& b)
    {
        Vector3 diff = { a.center.x - b.center.x, a.center.y - b.center.y, a.center.z - b.center.z };
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float radiusSum = a.radius + b.radius;
        return distanceSq <= (radiusSum * radiusSum);
    }

    /// <summary>
    /// AABB × AABB 判定
    /// </summary>
    inline bool IsHit(const AABB& a, const AABB& b)
    {
        Vector3 aMin = a.center + a.min;
        Vector3 aMax = a.center + a.max;
        Vector3 bMin = b.center + b.min;
        Vector3 bMax = b.center + b.max;

        // 各軸で分離していなければ衝突
        return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
            (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
            (aMin.z <= bMax.z && aMax.z >= bMin.z);
    }

    /// <summary>
    /// Sphere × AABB 判定（最近点との距離で判定）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const AABB& box)
    {
        Vector3 boxMin = box.center + box.min;
        Vector3 boxMax = box.center + box.max;

        // AABB内でSphere.centerに最も近い点を求める
        Vector3 closestPoint;
        closestPoint.x = std::clamp(sphere.center.x, boxMin.x, boxMax.x);
        closestPoint.y = std::clamp(sphere.center.y, boxMin.y, boxMax.y);
        closestPoint.z = std::clamp(sphere.center.z, boxMin.z, boxMax.z);

        // 最近点との距離の2乗
        Vector3 diff = {
            sphere.center.x - closestPoint.x,
            sphere.center.y - closestPoint.y,
            sphere.center.z - closestPoint.z
        };
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// AABB × Sphere 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const AABB& a, const Sphere& b)
    {
        return IsHit(b, a);
    }

    /// @brief Vector3 座標がAABBの最上面に乗っているかを判定する
    /// @param point 判定対象の座標
    /// @param box   対象のAABB
    /// @param tolerance Y方向の許容誤差（デフォルト: 0.01f）
    /// @return 最上面に乗っていれば true
    inline bool IsOnTopSurface(const Vector3& point, const AABB& box, float tolerance = 0.01f)
    {
        Vector3 worldMin = box.GetMinWorld();
        Vector3 worldMax = box.GetMaxWorld();

        // XZ方向がAABBの範囲内かチェック
        bool inRangeXZ =
            point.x >= worldMin.x && point.x <= worldMax.x &&
            point.z >= worldMin.z && point.z <= worldMax.z;

        // Y座標がAABBの最上面に一致しているかチェック（許容誤差込み）
        bool onTopY = std::abs(point.y - worldMax.y) <= tolerance;

        return inRangeXZ && onTopY;
    }

    /// <summary>
    /// 分離軸定理(SAT)のためのヘルパー関数
    /// </summary>
    namespace Detail
    {
        /// @brief Get the slope top surface Y at the specified world position.
        /// @param slope Target slope.
        /// @param point World position used for XZ sampling.
        /// @return Y coordinate of the slope top surface.
        inline float GetSlopeSurfaceY(const Slope& slope, const Vector3& point)
        {
            Vector3 worldMin = slope.GetSurfaceMinWorld();
            Vector3 worldMax = slope.GetMaxWorld();
            float width = std::max(worldMax.x - worldMin.x, 0.0001f);
            float depth = std::max(worldMax.z - worldMin.z, 0.0001f);
            float height = worldMax.y - worldMin.y;
            float rate = 0.0f;

            switch (slope.direction) {
            case SlopeDirection::PulsX:
                rate = (point.x - worldMin.x) / width;
                break;
            case SlopeDirection::MinusX:
                rate = (worldMax.x - point.x) / width;
                break;
            case SlopeDirection::PulsZ:
                rate = (point.z - worldMin.z) / depth;
                break;
            case SlopeDirection::MinusZ:
                rate = (worldMax.z - point.z) / depth;
                break;
            }

            rate = std::clamp(rate, 0.0f, 1.0f);
            return worldMin.y + height * rate;
        }

        /// @brief Get the upward normal of the slope top surface.
        /// @param slope Target slope.
        /// @return Normalized upward normal.
        inline Vector3 GetSlopeTopNormal(const Slope& slope)
        {
            Vector3 worldMin = slope.GetSurfaceMinWorld();
            Vector3 worldMax = slope.GetMaxWorld();
            float width = std::max(worldMax.x - worldMin.x, 0.0001f);
            float depth = std::max(worldMax.z - worldMin.z, 0.0001f);
            float height = worldMax.y - worldMin.y;

            switch (slope.direction) {
            case SlopeDirection::PulsX:
                return Vector3{ -height / width, 1.0f, 0.0f }.Normalized();
            case SlopeDirection::MinusX:
                return Vector3{ height / width, 1.0f, 0.0f }.Normalized();
            case SlopeDirection::PulsZ:
                return Vector3{ 0.0f, 1.0f, -height / depth }.Normalized();
            case SlopeDirection::MinusZ:
                return Vector3{ 0.0f, 1.0f, height / depth }.Normalized();
            }

            return Vector3{ 0.0f, 1.0f, 0.0f };
        }

        /// @brief Check whether a point is inside the slope XZ footprint.
        /// @param slope Target slope.
        /// @param point World position.
        /// @param margin Additional accepted margin.
        /// @return True when the point is inside the slope XZ footprint.
        inline bool IsInsideSlopeXZ(const Slope& slope, const Vector3& point, float margin = 0.0f)
        {
            Vector3 worldMin = slope.GetMinWorld();
            Vector3 worldMax = slope.GetMaxWorld();
            return point.x >= worldMin.x - margin && point.x <= worldMax.x + margin &&
                point.z >= worldMin.z - margin && point.z <= worldMax.z + margin;
        }
        /// <summary>
        /// OBBを指定された軸に投影した際の範囲を計算
        /// OBBの8頂点をワールド空間で計算し、軸への投影範囲を求める
        /// </summary>
        /// <param name="obb">対象のOBB</param>
        /// <param name="axis">投影する軸</param>
        /// <param name="outMin">投影範囲の最小値（出力）</param>
        /// <param name="outMax">投影範囲の最大値（出力）</param>
        inline void GetProjectionRange(const OBB& obb, const Vector3& axis, float& outMin, float& outMax)
        {
            // OBBの8頂点をローカル空間で生成
            Vector3 localVertices[8] = {
                { obb.min.x, obb.min.y, obb.min.z },
                { obb.max.x, obb.min.y, obb.min.z },
                { obb.max.x, obb.max.y, obb.min.z },
                { obb.min.x, obb.max.y, obb.min.z },
                { obb.min.x, obb.min.y, obb.max.z },
                { obb.max.x, obb.min.y, obb.max.z },
                { obb.max.x, obb.max.y, obb.max.z },
                { obb.min.x, obb.max.y, obb.max.z }
            };

            // ワールド空間に変換して軸に投影
            outMin = FLT_MAX;
            outMax = -FLT_MAX;

            for (int i = 0; i < 8; ++i) {
                // ローカル座標をワールド座標に変換
                Vector3 worldVertex = {
                    obb.center.x + obb.orientation[0].x * localVertices[i].x + obb.orientation[1].x * localVertices[i].y + obb.orientation[2].x * localVertices[i].z,
                    obb.center.y + obb.orientation[0].y * localVertices[i].x + obb.orientation[1].y * localVertices[i].y + obb.orientation[2].y * localVertices[i].z,
                    obb.center.z + obb.orientation[0].z * localVertices[i].x + obb.orientation[1].z * localVertices[i].y + obb.orientation[2].z * localVertices[i].z
                };

                // 軸に投影
                float projection = Math::Dot(worldVertex, axis);
                outMin = std::min(outMin, projection);
                outMax = std::max(outMax, projection);
            }
        }

        /// <summary>
        /// 分離軸上での2つのOBBの投影が分離しているかチェック
        /// </summary>
        inline bool IsSeparatedOnAxis(const OBB& a, const OBB& b, const Vector3& axis)
        {
            // 軸がゼロベクトルの場合はスキップ（外積が0になる場合など）
            float axisLengthSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
            if (axisLengthSq < 0.0001f) return false;

            Vector3 normalizedAxis = axis.Normalized();

            // 各OBBの投影範囲を計算
            float aMin, aMax, bMin, bMax;
            GetProjectionRange(a, normalizedAxis, aMin, aMax);
            GetProjectionRange(b, normalizedAxis, bMin, bMax);

            // 分離している場合はtrue（範囲が重なっていない）
            return (aMax < bMin) || (bMax < aMin);
        }

        /// <summary>
        /// 線分上の最近点のパラメータtを計算（0 <= t <= 1）
        /// </summary>
        inline float GetClosestPointOnSegment(const Vector3& segStart, const Vector3& segEnd, const Vector3& point)
        {
            Vector3 segVec = { segEnd.x - segStart.x, segEnd.y - segStart.y, segEnd.z - segStart.z };
            Vector3 pointVec = { point.x - segStart.x, point.y - segStart.y, point.z - segStart.z };

            float segLengthSq = segVec.x * segVec.x + segVec.y * segVec.y + segVec.z * segVec.z;
            if (segLengthSq < 0.0001f) return 0.0f;

            float t = Math::Dot(pointVec, segVec) / segLengthSq;
            return std::clamp(t, 0.0f, 1.0f);
        }
    }

    /// <summary>
    /// OBB × OBB 判定（分離軸定理）
    /// </summary>
    /// @brief Check collision between a sphere and a slope.
    /// @param sphere Target sphere.
    /// @param slope Target slope.
    /// @return True when the sphere intersects the slope volume.
    inline bool IsHit(const Sphere& sphere, const Slope& slope)
    {
        Vector3 worldMin = slope.GetMinWorld();
        Vector3 worldMax = slope.GetMaxWorld();

        if (sphere.center.x < worldMin.x - sphere.radius || sphere.center.x > worldMax.x + sphere.radius ||
            sphere.center.y < worldMin.y - sphere.radius || sphere.center.y > worldMax.y + sphere.radius ||
            sphere.center.z < worldMin.z - sphere.radius || sphere.center.z > worldMax.z + sphere.radius) {
            return false;
        }

        float closestX = std::clamp(sphere.center.x, worldMin.x, worldMax.x);
        float closestZ = std::clamp(sphere.center.z, worldMin.z, worldMax.z);
        Vector3 closestXZ = { closestX, sphere.center.y, closestZ };
        float surfaceY = Detail::GetSlopeSurfaceY(slope, closestXZ);

        if (Detail::IsInsideSlopeXZ(slope, sphere.center)) {
            float sphereTopY = sphere.center.y + sphere.radius;
            Vector3 normal = Detail::GetSlopeTopNormal(slope);
            Vector3 planePoint = { sphere.center.x, surfaceY, sphere.center.z };
            float signedDistance = Math::Dot(sphere.center - planePoint, normal);
            return signedDistance >= -sphere.radius &&
                signedDistance <= sphere.radius &&
                sphereTopY >= worldMin.y;
        }

        Vector3 closestPoint = {
            closestX,
            std::clamp(sphere.center.y, worldMin.y, surfaceY),
            closestZ
        };

        Vector3 diff = sphere.center - closestPoint;
        return diff.LengthSq() <= sphere.radius * sphere.radius;
    }

    /// @brief Check collision between a slope and a sphere.
    /// @param slope Target slope.
    /// @param sphere Target sphere.
    /// @return True when the sphere intersects the slope volume.
    inline bool IsHit(const Slope& slope, const Sphere& sphere)
    {
        return IsHit(sphere, slope);
    }

    inline bool IsHit(const OBB& a, const OBB& b)
    {
        // 分離軸定理: 15個の軸でテスト
        // 1. Aの3軸
        // 2. Bの3軸
        // 3. 上記の外積9個

        // Aの3軸でテスト
        for (int i = 0; i < 3; ++i)
        {
            if (Detail::IsSeparatedOnAxis(a, b, a.orientation[i]))
                return false;
        }

        // Bの3軸でテスト
        for (int i = 0; i < 3; ++i)
        {
            if (Detail::IsSeparatedOnAxis(a, b, b.orientation[i]))
                return false;
        }

        // 外積の9軸でテスト
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                Vector3 axis = Math::Cross(a.orientation[i], b.orientation[j]);
                if (Detail::IsSeparatedOnAxis(a, b, axis))
                    return false;
            }
        }

        // すべての軸で分離していない = 衝突している
        return true;
    }

    /// <summary>
    /// AABB × OBB 判定
    /// AABBの3軸 + OBBの3軸 + 外積9軸 = 15軸でテスト
    /// </summary>
    inline bool IsHit(const AABB& aabb, const OBB& obb)
    {
        // AABBの3軸（X, Y, Z）でテスト
        Vector3 aabbAxes[3] = {
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f }
        };

        // AABBのワールド座標での範囲
        Vector3 aabbMin = aabb.center + aabb.min;
        Vector3 aabbMax = aabb.center + aabb.max;

        // AABBの各軸での投影テスト
        for (int i = 0; i < 3; ++i) {
            // AABBの投影範囲
            float aabbProjMin = (i == 0) ? aabbMin.x : (i == 1) ? aabbMin.y : aabbMin.z;
            float aabbProjMax = (i == 0) ? aabbMax.x : (i == 1) ? aabbMax.y : aabbMax.z;

            // OBBの投影範囲
            float obbProjMin, obbProjMax;
            Detail::GetProjectionRange(obb, aabbAxes[i], obbProjMin, obbProjMax);

            // 分離軸判定（範囲が重なっていない場合は衝突していない）
            if (aabbProjMax < obbProjMin || obbProjMax < aabbProjMin) {
                return false;
            }
        }

        // OBBの各軸での投影テスト
        for (int i = 0; i < 3; ++i) {
            // OBBの投影範囲
            float obbProjMin, obbProjMax;
            Detail::GetProjectionRange(obb, obb.orientation[i], obbProjMin, obbProjMax);

            // AABBの8頂点を生成してOBBの軸に投影
            Vector3 aabbVertices[8] = {
                { aabbMin.x, aabbMin.y, aabbMin.z },
                { aabbMax.x, aabbMin.y, aabbMin.z },
                { aabbMax.x, aabbMax.y, aabbMin.z },
                { aabbMin.x, aabbMax.y, aabbMin.z },
                { aabbMin.x, aabbMin.y, aabbMax.z },
                { aabbMax.x, aabbMin.y, aabbMax.z },
                { aabbMax.x, aabbMax.y, aabbMax.z },
                { aabbMin.x, aabbMax.y, aabbMax.z }
            };

            float aabbProjMin = FLT_MAX;
            float aabbProjMax = -FLT_MAX;
            for (int j = 0; j < 8; ++j) {
                float proj = Math::Dot(aabbVertices[j], obb.orientation[i]);
                aabbProjMin = std::min(aabbProjMin, proj);
                aabbProjMax = std::max(aabbProjMax, proj);
            }

            // 分離軸判定
            if (aabbProjMax < obbProjMin || obbProjMax < aabbProjMin) {
                return false;
            }
        }

        // 外積の9軸でテスト（AABBの3軸 × OBBの3軸）
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                Vector3 axis = Math::Cross(aabbAxes[i], obb.orientation[j]);

                float axisLengthSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
                if (axisLengthSq < 0.0001f) continue; // 平行な軸はスキップ

                Vector3 normalizedAxis = axis.Normalized();

                // AABBの投影範囲
                Vector3 aabbVertices[8] = {
                    { aabbMin.x, aabbMin.y, aabbMin.z },
                    { aabbMax.x, aabbMin.y, aabbMin.z },
                    { aabbMax.x, aabbMax.y, aabbMin.z },
                    { aabbMin.x, aabbMax.y, aabbMin.z },
                    { aabbMin.x, aabbMin.y, aabbMax.z },
                    { aabbMax.x, aabbMin.y, aabbMax.z },
                    { aabbMax.x, aabbMax.y, aabbMax.z },
                    { aabbMin.x, aabbMax.y, aabbMax.z }
                };

                float aabbProjMin = FLT_MAX;
                float aabbProjMax = -FLT_MAX;
                for (int k = 0; k < 8; ++k) {
                    float proj = Math::Dot(aabbVertices[k], normalizedAxis);
                    aabbProjMin = std::min(aabbProjMin, proj);
                    aabbProjMax = std::max(aabbProjMax, proj);
                }

                // OBBの投影範囲
                float obbProjMin, obbProjMax;
                Detail::GetProjectionRange(obb, normalizedAxis, obbProjMin, obbProjMax);

                // 分離軸判定
                if (aabbProjMax < obbProjMin || obbProjMax < aabbProjMin) {
                    return false;
                }
            }
        }

        // すべての軸で分離していない = 衝突している
        return true;
    }

    /// <summary>
    /// OBB × Sphere 判定
    /// OBBのローカル座標系での最近点を求める
    /// </summary>
    inline bool IsHit(const OBB& obb, const Sphere& sphere)
    {
        Vector3 center = obb.center;

        // 球の中心をOBBのローカル座標系に変換
        Vector3 centerDiff = {
            sphere.center.x - center.x,
            sphere.center.y - center.y,
            sphere.center.z - center.z
        };

        // OBBの各軸に投影してローカル座標を求める
        Vector3 localCenter = {
            Math::Dot(centerDiff, obb.orientation[0]),
            Math::Dot(centerDiff, obb.orientation[1]),
            Math::Dot(centerDiff, obb.orientation[2])
        };

        // ローカル空間での最近点を求める（min/maxの範囲でクランプ）
        // min/maxはcenterからのオフセットなので、そのまま範囲として使用
        Vector3 closestLocal = {
            std::clamp(localCenter.x, obb.min.x, obb.max.x),
            std::clamp(localCenter.y, obb.min.y, obb.max.y),
            std::clamp(localCenter.z, obb.min.z, obb.max.z)
        };

        // 最近点をワールド座標に戻す
        Vector3 closestWorld = {
            center.x + obb.orientation[0].x * closestLocal.x + obb.orientation[1].x * closestLocal.y + obb.orientation[2].x * closestLocal.z,
            center.y + obb.orientation[0].y * closestLocal.x + obb.orientation[1].y * closestLocal.y + obb.orientation[2].y * closestLocal.z,
            center.z + obb.orientation[0].z * closestLocal.x + obb.orientation[1].z * closestLocal.y + obb.orientation[2].z * closestLocal.z
        };

        // 球の中心と最近点の距離の2乗を計算
        Vector3 diff = {
            sphere.center.x - closestWorld.x,
            sphere.center.y - closestWorld.y,
            sphere.center.z - closestWorld.z
        };
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// Sphere × OBB 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const OBB& obb)
    {
        return IsHit(obb, sphere);
    }

    /// <summary>
    /// OvalSphere × Sphere 判定
    /// 球体の中心から楕円球体表面への最短距離で判定（回転対応）
    /// </summary>
    inline bool IsHit(const OvalSphere& ovalSphere, const Sphere& sphere)
    {
        // 球の中心を楕円球体のローカル座標系に変換
        Vector3 centerDiff = {
            sphere.center.x - ovalSphere.center.x,
            sphere.center.y - ovalSphere.center.y,
            sphere.center.z - ovalSphere.center.z
        };

        // 楕円球体の各軸に投影してローカル座標を求める
        Vector3 localCenter = {
            Math::Dot(centerDiff, ovalSphere.orientation[0]),
            Math::Dot(centerDiff, ovalSphere.orientation[1]),
            Math::Dot(centerDiff, ovalSphere.orientation[2])
        };

        // ローカル空間での最近点を求める（min/maxの範囲でクランプ）
        Vector3 closestLocal = {
            std::clamp(localCenter.x, -ovalSphere.radius.x, ovalSphere.radius.x),
            std::clamp(localCenter.y, -ovalSphere.radius.y, ovalSphere.radius.y),
            std::clamp(localCenter.z, -ovalSphere.radius.z, ovalSphere.radius.z)
        };

        // 最近点をワールド座標に戻す
        Vector3 closestWorld = {
            ovalSphere.center.x + ovalSphere.orientation[0].x * closestLocal.x + ovalSphere.orientation[1].x * closestLocal.y + ovalSphere.orientation[2].x * closestLocal.z,
            ovalSphere.center.y + ovalSphere.orientation[0].y * closestLocal.x + ovalSphere.orientation[1].y * closestLocal.y + ovalSphere.orientation[2].y * closestLocal.z,
            ovalSphere.center.z + ovalSphere.orientation[0].z * closestLocal.x + ovalSphere.orientation[1].z * closestLocal.y + ovalSphere.orientation[2].z * closestLocal.z
        };

        // 球の中心から楕円表面の最近点までの距離
        Vector3 surfaceDiff = {
            sphere.center.x - closestWorld.x,
            sphere.center.y - closestWorld.y,
            sphere.center.z - closestWorld.z
        };

        float surfaceDistanceSq = surfaceDiff.x * surfaceDiff.x +
            surfaceDiff.y * surfaceDiff.y +
            surfaceDiff.z * surfaceDiff.z;

        return surfaceDistanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// Sphere × OvalSphere 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const OvalSphere& ovalSphere)
    {
        return IsHit(ovalSphere, sphere);
    }

    /// <summary>
    /// OvalSphere × AABB 判定
    /// 楕円球体の各軸での最近点を求めて判定（回転対応）
    /// </summary>
    inline bool IsHit(const OvalSphere& ovalSphere, const AABB& aabb)
    {
        Vector3 aabbMin = aabb.center + aabb.min;
        Vector3 aabbMax = aabb.center + aabb.max;

        // AABBに対する楕円球体の中心の最近点を求める
        Vector3 closestPoint = {
            std::clamp(ovalSphere.center.x, aabbMin.x, aabbMax.x),
            std::clamp(ovalSphere.center.y, aabbMin.y, aabbMax.y),
            std::clamp(ovalSphere.center.z, aabbMin.z, aabbMax.z)
        };

        // 楕円球体の中心から最近点への差分
        Vector3 diff = {
            closestPoint.x - ovalSphere.center.x,
            closestPoint.y - ovalSphere.center.y,
            closestPoint.z - ovalSphere.center.z
        };

        // 差分を楕円球体のローカル座標系に変換
        Vector3 localDiff = {
            Math::Dot(diff, ovalSphere.orientation[0]),
            Math::Dot(diff, ovalSphere.orientation[1]),
            Math::Dot(diff, ovalSphere.orientation[2])
        };

        // 楕円の方程式で判定: (x/a)² + (y/b)² + (z/c)² <= 1
        float ellipseValue = (localDiff.x * localDiff.x) / (ovalSphere.radius.x * ovalSphere.radius.x) +
            (localDiff.y * localDiff.y) / (ovalSphere.radius.y * ovalSphere.radius.y) +
            (localDiff.z * localDiff.z) / (ovalSphere.radius.z * ovalSphere.radius.z);

        return ellipseValue <= 1.0f;
    }

    /// <summary>
    /// AABB × OvalSphere 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const AABB& aabb, const OvalSphere& ovalSphere)
    {
        return IsHit(ovalSphere, aabb);
    }

    /// <summary>
    /// OvalSphere × OBB 判定
    /// 楕円球体をOBBのローカル座標系に変換して判定（回転対応）
    /// </summary>
    inline bool IsHit(const OvalSphere& ovalSphere, const OBB& obb)
    {
        Vector3 center = obb.center;

        // 楕円球体の中心をOBBのローカル座標系に変換
        Vector3 centerDiff = {
            ovalSphere.center.x - center.x,
            ovalSphere.center.y - center.y,
            ovalSphere.center.z - center.z
        };

        // OBBの各軸に投影してローカル座標を求める
        Vector3 localOvalCenter = {
            Math::Dot(centerDiff, obb.orientation[0]),
            Math::Dot(centerDiff, obb.orientation[1]),
            Math::Dot(centerDiff, obb.orientation[2])
        };

        // ローカル空間での最近点を求める（min/maxの範囲でクランプ）
        // min/maxはcenterからのオフセットなので、そのまま範囲として使用
        Vector3 closestLocal = {
            std::clamp(localOvalCenter.x, obb.min.x, obb.max.x),
            std::clamp(localOvalCenter.y, obb.min.y, obb.max.y),
            std::clamp(localOvalCenter.z, obb.min.z, obb.max.z)
        };

        // 最近点をワールド座標に戻す
        Vector3 closestWorld = {
            center.x + obb.orientation[0].x * closestLocal.x + obb.orientation[1].x * closestLocal.y + obb.orientation[2].x * closestLocal.z,
            center.y + obb.orientation[0].y * closestLocal.x + obb.orientation[1].y * closestLocal.y + obb.orientation[2].y * closestLocal.z,
            center.z + obb.orientation[0].z * closestLocal.x + obb.orientation[1].z * closestLocal.y + obb.orientation[2].z * closestLocal.z
        };

        // 楕円球体の中心から最近点への差分
        Vector3 diff = {
            closestWorld.x - ovalSphere.center.x,
            closestWorld.y - ovalSphere.center.y,
            closestWorld.z - ovalSphere.center.z
        };

        // 差分を楕円球体のローカル座標系に変換
        Vector3 localDiff = {
            Math::Dot(diff, ovalSphere.orientation[0]),
            Math::Dot(diff, ovalSphere.orientation[1]),
            Math::Dot(diff, ovalSphere.orientation[2])
        };

        // 楕円の方程式で判定: (x/a)² + (y/b)² + (z/c)² <= 1
        float ellipseValue = (localDiff.x * localDiff.x) / (ovalSphere.radius.x * ovalSphere.radius.x) +
            (localDiff.y * localDiff.y) / (ovalSphere.radius.y * ovalSphere.radius.y) +
            (localDiff.z * localDiff.z) / (ovalSphere.radius.z * ovalSphere.radius.z);

        return ellipseValue <= 1.0f;
    }

    /// <summary>
    /// OBB × OvalSphere 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const OBB& obb, const OvalSphere& ovalSphere)
    {
        return IsHit(ovalSphere, obb);
    }

    /// <summary>
    /// OvalSphere × OvalSphere 判定
    /// 2つの楕円球体同士の衝突判定（回転対応）
    /// </summary>
    inline bool IsHit(const OvalSphere& a, const OvalSphere& b)
    {
        // 中心間の距離ベクトル
        Vector3 diff = {
            b.center.x - a.center.x,
            b.center.y - a.center.y,
            b.center.z - a.center.z
        };

        // 楕円球体Aのローカル座標系での距離を計算
        Vector3 localDiffA = {
            Math::Dot(diff, a.orientation[0]),
            Math::Dot(diff, a.orientation[1]),
            Math::Dot(diff, a.orientation[2])
        };

        // 楕円球体Bのローカル座標系での距離を計算（逆方向）
        Vector3 diffInverse = {
            a.center.x - b.center.x,
            a.center.y - b.center.y,
            a.center.z - b.center.z
        };

        Vector3 localDiffB = {
            Math::Dot(diffInverse, b.orientation[0]),
            Math::Dot(diffInverse, b.orientation[1]),
            Math::Dot(diffInverse, b.orientation[2])
        };

        // 各楕円体のローカル空間での正規化距離
        float normalizedDistanceA =
            (localDiffA.x * localDiffA.x) / (a.radius.x * a.radius.x) +
            (localDiffA.y * localDiffA.y) / (a.radius.y * a.radius.y) +
            (localDiffA.z * localDiffA.z) / (a.radius.z * a.radius.z);

        float normalizedDistanceB =
            (localDiffB.x * localDiffB.x) / (b.radius.x * b.radius.x) +
            (localDiffB.y * localDiffB.y) / (b.radius.y * b.radius.y) +
            (localDiffB.z * localDiffB.z) / (b.radius.z * b.radius.z);

        // どちらか一方の楕円内に相手の中心がある場合は衝突
        // または距離が十分近い場合（保守的な判定）
        return (normalizedDistanceA <= 1.0f) || (normalizedDistanceB <= 1.0f) ||
            ((normalizedDistanceA + normalizedDistanceB) * 0.5f <= 1.0f);
    }

    /// <summary>
    /// Plane × Sphere 判定
    /// 球の中心から平面までの距離で判定（有限平面）
    /// </summary>
    inline bool IsHit(const Plane& plane, const Sphere& sphere)
    {
        // 正規化された法線を使用
        Vector3 normal = plane.normal.Normalized();

        // 球の中心から平面中心への差分ベクトル
        Vector3 centerToPlane = {
            sphere.center.x - plane.center.x,
            sphere.center.y - plane.center.y,
            sphere.center.z - plane.center.z
        };

        // 平面への垂直距離
        float normalDistance = Math::Dot(centerToPlane, normal);

        // 法線方向の距離が球の半径より大きい場合は衝突しない
        if (std::abs(normalDistance) > sphere.radius) {
            return false;
        }

        // 平面上での最近点を計算
        Vector3 projectedPoint = {
            sphere.center.x - normal.x * normalDistance,
            sphere.center.y - normal.y * normalDistance,
            sphere.center.z - normal.z * normalDistance
        };

        // 平面の接線ベクトルを計算
        Vector3 tangent;
        if (std::abs(normal.x) < 0.9f) {
            tangent = Math::Cross(normal, Vector3{ 1.0f, 0.0f, 0.0f });
        } else {
            tangent = Math::Cross(normal, Vector3{ 0.0f, 1.0f, 0.0f });
        }
        tangent = tangent.Normalized();
        Vector3 bitangent = Math::Cross(normal, tangent).Normalized();

        // 投影点から平面中心への差分を平面の座標系で表現
        Vector3 diff = {
            projectedPoint.x - plane.center.x,
            projectedPoint.y - plane.center.y,
            projectedPoint.z - plane.center.z
        };

        float u = Math::Dot(diff, tangent);
        float v = Math::Dot(diff, bitangent);

        // 平面の範囲内にあるかチェック（正方形の範囲）
        float halfSize = plane.size * 0.5f;

        // 範囲外の場合は、平面の端からの距離を計算
        float uClamped = std::clamp(u, -halfSize, halfSize);
        float vClamped = std::clamp(v, -halfSize, halfSize);

        // 最近点を計算
        Vector3 closestPoint = {
            plane.center.x + tangent.x * uClamped + bitangent.x * vClamped,
            plane.center.y + tangent.y * uClamped + bitangent.y * vClamped,
            plane.center.z + tangent.z * uClamped + bitangent.z * vClamped
        };

        // 球の中心から最近点までの距離
        Vector3 toSphere = {
            sphere.center.x - closestPoint.x,
            sphere.center.y - closestPoint.y,
            sphere.center.z - closestPoint.z
        };

        float distanceSq = toSphere.x * toSphere.x + toSphere.y * toSphere.y + toSphere.z * toSphere.z;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// Sphere × Plane 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const Plane& plane)
    {
        return IsHit(plane, sphere);
    }

    /// <summary>
    /// Plane × AABB 判定
    /// AABBの8頂点が平面の範囲内で平面の両側にあるかチェック
    /// </summary>
    inline bool IsHit(const Plane& plane, const AABB& aabb)
    {
        Vector3 normal = plane.normal.Normalized();
        Vector3 aabbMin = aabb.center + aabb.min;
        Vector3 aabbMax = aabb.center + aabb.max;

        // AABBの8頂点を生成
        Vector3 vertices[8] = {
            { aabbMin.x, aabbMin.y, aabbMin.z },
            { aabbMax.x, aabbMin.y, aabbMin.z },
            { aabbMax.x, aabbMax.y, aabbMin.z },
            { aabbMin.x, aabbMax.y, aabbMin.z },
            { aabbMin.x, aabbMin.y, aabbMax.z },
            { aabbMax.x, aabbMin.y, aabbMax.z },
            { aabbMax.x, aabbMax.y, aabbMax.z },
            { aabbMin.x, aabbMax.y, aabbMax.z }
        };

        // 平面の接線ベクトルを計算
        Vector3 tangent;
        if (std::abs(normal.x) < 0.9f) {
            tangent = Math::Cross(normal, Vector3{ 1.0f, 0.0f, 0.0f });
        } else {
            tangent = Math::Cross(normal, Vector3{ 0.0f, 1.0f, 0.0f });
        }
        tangent = tangent.Normalized();
        Vector3 bitangent = Math::Cross(normal, tangent).Normalized();

        float planeHalfSize = plane.size * 0.5f;

        // 各頂点が平面の範囲内にあり、かつ平面の両側にあるかチェック
        bool hasPositive = false;
        bool hasNegative = false;
        bool hasInRange = false;

        for (int i = 0; i < 8; ++i) {
            Vector3 toVertex = {
                vertices[i].x - plane.center.x,
                vertices[i].y - plane.center.y,
                vertices[i].z - plane.center.z
            };

            float normalDist = Math::Dot(toVertex, normal);
            float u = Math::Dot(toVertex, tangent);
            float v = Math::Dot(toVertex, bitangent);

            // 平面の範囲内にあるかチェック
            if (std::abs(u) <= planeHalfSize && std::abs(v) <= planeHalfSize) {
                hasInRange = true;
                if (normalDist > 0.0f) hasPositive = true;
                if (normalDist < 0.0f) hasNegative = true;
            }
        }

        // 範囲内に頂点があり、かつ平面の両側に頂点がある場合は衝突
        return hasInRange && (hasPositive && hasNegative);
    }

    /// <summary>
    /// AABB × Plane 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const AABB& aabb, const Plane& plane)
    {
        return IsHit(plane, aabb);
    }

    /// <summary>
    /// Plane × OBB 判定
    /// OBBの8頂点が平面の範囲内で平面の両側にあるかチェック
    /// </summary>
    inline bool IsHit(const Plane& plane, const OBB& obb)
    {
        Vector3 normal = plane.normal.Normalized();
        Vector3 center = obb.center;

        // OBBの8頂点を生成（min/maxのローカルオフセットから）
        // min/maxはcenterからのオフセットなので、そのまま使用
        Vector3 localVertices[8] = {
            { obb.min.x, obb.min.y, obb.min.z },
            { obb.max.x, obb.min.y, obb.min.z },
            { obb.max.x, obb.max.y, obb.min.z },
            { obb.min.x, obb.max.y, obb.min.z },
            { obb.min.x, obb.min.y, obb.max.z },
            { obb.max.x, obb.min.y, obb.max.z },
            { obb.max.x, obb.max.y, obb.max.z },
            { obb.min.x, obb.max.y, obb.max.z }
        };

        Vector3 worldVertices[8];
        for (int i = 0; i < 8; ++i) {
            worldVertices[i] = {
                center.x + obb.orientation[0].x * localVertices[i].x + obb.orientation[1].x * localVertices[i].y + obb.orientation[2].x * localVertices[i].z,
                center.y + obb.orientation[0].y * localVertices[i].x + obb.orientation[1].y * localVertices[i].y + obb.orientation[2].y * localVertices[i].z,
                center.z + obb.orientation[0].z * localVertices[i].x + obb.orientation[1].z * localVertices[i].y + obb.orientation[2].z * localVertices[i].z
            };
        }

        // 平面の接線ベクトルを計算
        Vector3 tangent;
        if (std::abs(normal.x) < 0.9f) {
            tangent = Math::Cross(normal, Vector3{ 1.0f, 0.0f, 0.0f });
        } else {
            tangent = Math::Cross(normal, Vector3{ 0.0f, 1.0f, 0.0f });
        }
        tangent = tangent.Normalized();
        Vector3 bitangent = Math::Cross(normal, tangent).Normalized();

        float planeHalfSize = plane.size * 0.5f;

        // 各頂点が平面の範囲内にあり、かつ平面の両側にあるかチェック
        bool hasPositive = false;
        bool hasNegative = false;
        bool hasInRange = false;

        for (int i = 0; i < 8; ++i) {
            Vector3 toVertex = {
                worldVertices[i].x - plane.center.x,
                worldVertices[i].y - plane.center.y,
                worldVertices[i].z - plane.center.z
            };

            float normalDist = Math::Dot(toVertex, normal);
            float u = Math::Dot(toVertex, tangent);
            float v = Math::Dot(toVertex, bitangent);

            // 平面の範囲内にあるかチェック
            if (std::abs(u) <= planeHalfSize && std::abs(v) <= planeHalfSize) {
                hasInRange = true;
                if (normalDist > 0.0f) hasPositive = true;
                if (normalDist < 0.0f) hasNegative = true;
            }
        }

        // 範囲内に頂Vertexがあり、かつ平面の両側に頂Vertexがある場合は衝突
        return hasInRange && (hasPositive && hasNegative);
    }

    /// <summary>
    /// OBB × Plane 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const OBB& obb, const Plane& plane)
    {
        return IsHit(plane, obb);
    }

    /// <summary>
    /// Segment × Sphere 判定
    /// 線分上の最近点から球までの距離で判定
    /// </summary>
    inline bool IsHit(const Segment& segment, const Sphere& sphere)
    {
        Vector3 segEnd = {
            segment.origin.x + segment.diff.x,
            segment.origin.y + segment.diff.y,
            segment.origin.z + segment.diff.z
        };

        // 線分上の最近点を求める
        float t = Detail::GetClosestPointOnSegment(segment.origin, segEnd, sphere.center);
        Vector3 closestPoint = {
            segment.origin.x + segment.diff.x * t,
            segment.origin.y + segment.diff.y * t,
            segment.origin.z + segment.diff.z * t
        };

        // 最近点から球の中心までの距離
        Vector3 diff = {
            sphere.center.x - closestPoint.x,
            sphere.center.y - closestPoint.y,
            sphere.center.z - closestPoint.z
        };
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// Sphere × Segment 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const Segment& segment)
    {
        return IsHit(segment, sphere);
    }

    /// <summary>
    /// Segment × AABB 判定
    /// 線分とAABBの交差判定（スラブ法）
    /// </summary>
    inline bool IsHit(const Segment& segment, const AABB& aabb)
    {
        Vector3 aabbMin = aabb.center + aabb.min;
        Vector3 aabbMax = aabb.center + aabb.max;

        Vector3 segEnd = {
            segment.origin.x + segment.diff.x,
            segment.origin.y + segment.diff.y,
            segment.origin.z + segment.diff.z
        };

        float tmin = 0.0f;
        float tmax = 1.0f;

        // 各軸でスラブテスト
        for (int i = 0; i < 3; ++i) {
            float origin = (i == 0) ? segment.origin.x : (i == 1) ? segment.origin.y : segment.origin.z;
            float dir = (i == 0) ? segment.diff.x : (i == 1) ? segment.diff.y : segment.diff.z;
            float boxMin = (i == 0) ? aabbMin.x : (i == 1) ? aabbMin.y : aabbMin.z;
            float boxMax = (i == 0) ? aabbMax.x : (i == 1) ? aabbMax.y : aabbMax.z;

            if (std::abs(dir) < 0.0001f) {
                // 線分がこの軸に平行
                if (origin < boxMin || origin > boxMax) {
                    return false;
                }
            } else {
                float t1 = (boxMin - origin) / dir;
                float t2 = (boxMax - origin) / dir;

                if (t1 > t2) {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                tmin = (std::max)(tmin, t1);
                tmax = (std::min)(tmax, t2);

                if (tmin > tmax) {
                    return false;
                }
            }
        }

        return true;
    }

    /// <summary>
    /// AABB × Segment 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const AABB& aabb, const Segment& segment)
    {
        return IsHit(segment, aabb);
    }

    /// <summary>
    /// Segment × Plane 判定
    /// 線分が平面と交差するかチェック
    /// </summary>
    inline bool IsHit(const Segment& segment, const Plane& plane)
    {
        Vector3 normal = plane.normal.Normalized();

        Vector3 segEnd = {
            segment.origin.x + segment.diff.x,
            segment.origin.y + segment.diff.y,
            segment.origin.z + segment.diff.z
        };

        // 始点と終点から平面までの距離
        Vector3 startToPlane = {
            segment.origin.x - plane.center.x,
            segment.origin.y - plane.center.y,
            segment.origin.z - plane.center.z
        };
        Vector3 endToPlane = {
            segEnd.x - plane.center.x,
            segEnd.y - plane.center.y,
            segEnd.z - plane.center.z
        };

        float startDist = Math::Dot(startToPlane, normal);
        float endDist = Math::Dot(endToPlane, normal);

        // 始点と終点が平面の反対側にあえば交差
        return (startDist * endDist) <= 0.0f;
    }

    /// <summary>
    /// Plane × Segment 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Plane& plane, const Segment& segment)
    {
        return IsHit(segment, plane);
    }

    /// <summary>
    /// Line × Sphere 判定
    /// 直線上の最近点から球までの距離で判定
    /// </summary>
    inline bool IsHit(const Line& line, const Sphere& sphere)
    {
        // 直線のベクトル
        Vector3 lineVec = {
            line.end.x - line.start.x,
            line.end.y - line.start.y,
            line.end.z - line.start.z
        };

        Vector3 toSphere = {
            sphere.center.x - line.start.x,
            sphere.center.y - line.start.y,
            sphere.center.z - line.start.z
        };

        float lineLengthSq = lineVec.x * lineVec.x + lineVec.y * lineVec.y + lineVec.z * lineVec.z;
        if (lineLengthSq < 0.0001f) {
            // 直線が点の場合
            float distSq = toSphere.x * toSphere.x + toSphere.y * toSphere.y + toSphere.z * toSphere.z;
            return distSq <= (sphere.radius * sphere.radius);
        }

        // 直線上の最近点（制限なし）
        float t = Math::Dot(toSphere, lineVec) / lineLengthSq;
        Vector3 closestPoint = {
            line.start.x + lineVec.x * t,
            line.start.y + lineVec.y * t,
            line.start.z + lineVec.z * t
        };

        Vector3 diff = {
            sphere.center.x - closestPoint.x,
            sphere.center.y - closestPoint.y,
            sphere.center.z - closestPoint.z
        };
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    /// <summary>
    /// Sphere × Line 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Sphere& sphere, const Line& line)
    {
        return IsHit(line, sphere);
    }

    /// <summary>
    /// Line × Plane 判定
    /// 直線が平面と交差するかチェック
    /// </summary>
    inline bool IsHit(const Line& line, const Plane& plane)
    {
        Vector3 normal = plane.normal.Normalized();

        Vector3 lineVec = {
            line.end.x - line.start.x,
            line.end.y - line.start.y,
            line.end.z - line.start.z
        };

        float dotProduct = Math::Dot(lineVec, normal);

        // 直線が平面に平行な場合
        if (std::abs(dotProduct) < 0.0001f) {
            // 直線上の点が平面上にあるかチェック
            Vector3 startToPlane = {
                line.start.x - plane.center.x,
                line.start.y - plane.center.y,
                line.start.z - plane.center.z
            };
            return std::abs(Math::Dot(startToPlane, normal)) < 0.0001f;
        }

        // 直線は必ず平面と交差する（平行でない限り）
        return true;
    }

    /// <summary>
    /// Plane × Line 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const Plane& plane, const Line& line)
    {
        return IsHit(line, plane);
    }

    /// <summary>
    /// OBB × AABB 判定（呼び出し順反転のラッパ）
    /// </summary>
    inline bool IsHit(const OBB& obb, const AABB& aabb)
    {
        return IsHit(aabb, obb);
    }
}
