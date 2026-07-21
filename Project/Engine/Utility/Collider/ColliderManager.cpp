#include "ColliderManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <type_traits>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

// pShape から実体を取得してコピーを作るように変更
static ColliderShape GetSyncedShape(const ColliderManager::ColliderInfo& info) {
    if (!info.pShape) return {}; // 万が一ポインタがnullの場合は空を返すなどの保護

    ColliderShape syncedShape = *(info.pShape);
    if (info.pPosition) {
        std::visit([&info](auto& s) {
            if constexpr (requires { s.center; }) {
                s.center = *(info.pPosition);
            }
            }, syncedShape);
    }
    return syncedShape;
}

/// @brief 指定したShapeのcenterを座標に同期する
/// @param shape 同期する形状
/// @param position 反映する座標
static void SyncShapeCenter(ColliderShape& shape, const Vector3& position) {
    std::visit([&position](auto& shapeActual) {
        if constexpr (requires { shapeActual.center; }) {
            shapeActual.center = position;
        }
        }, shape);
}

/// @brief コライダーの座標をShapeへ反映する
/// @param info 同期するコライダー情報
static void SyncColliderShapePosition(ColliderManager::ColliderInfo& info) {
    if (!info.pPosition || !info.pShape) {
        return;
    }

    SyncShapeCenter(*(info.pShape), *(info.pPosition));
}

/// @brief 2点間を線形補間する
/// @param start 開始座標
/// @param end 終了座標
/// @param t 補間率
/// @return 補間された座標
static Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t) {
    return start + (end - start) * t;
}

/// @brief 点がAABB内にあるか判定する
/// @param point 判定する点
/// @param min AABBの最小座標
/// @param max AABBの最大座標
/// @return 点がAABB内にあればtrue
static bool IsPointInsideAABB(const Vector3& point, const Vector3& min, const Vector3& max) {
    return point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

/// @brief SphereがAABBの上面上を移動しているか判定する
/// @param relativeStart AABB基準の開始座標
/// @param relativeEnd AABB基準の終了座標
/// @param aabb 対象AABB
/// @param radius Sphere半径
/// @return 上面移動として扱える場合はtrue
static bool IsSphereMovingOnAABBTop(const Vector3& relativeStart, const Vector3& relativeEnd, const AABB& aabb, float radius) {
    static constexpr float kTopTolerance = 0.08f;
    float topCenterY = aabb.max.y + radius;
    return relativeStart.y >= topCenterY - kTopTolerance &&
        relativeEnd.y >= topCenterY - kTopTolerance;
}

/// @brief SphereがAABB上面に乗っている状態か判定する
/// @param center Sphere中心
/// @param radius Sphere半径
/// @param aabbMin AABB最小座標
/// @param aabbMax AABB最大座標
/// @return 上面接触として扱える場合はtrue
static bool IsSphereOnAABBTop(const Vector3& center, float radius, const Vector3& aabbMin, const Vector3& aabbMax) {
    static constexpr float kTopTolerance = 0.08f;
    float closestX = std::clamp(center.x, aabbMin.x, aabbMax.x);
    float closestZ = std::clamp(center.z, aabbMin.z, aabbMax.z);
    float dx = center.x - closestX;
    float dz = center.z - closestZ;
    float bottomY = center.y - radius;

    return center.y >= aabbMax.y &&
        bottomY >= aabbMax.y - kTopTolerance &&
        bottomY <= aabbMax.y + kTopTolerance &&
        dx * dx + dz * dz <= radius * radius;
}

/// @brief AABB側面の接触を歩行用の継ぎ目として無視できるか判定する
/// @param center Sphere中心
/// @param radius Sphere半径
/// @param aabbMax AABB最大座標
/// @return 側面押し戻しを無視できる場合はtrue
static bool CanIgnoreAABBSideSeam(const Vector3& center, float radius, const Vector3& aabbMax) {
    static constexpr float kSeamTolerance = 0.12f;
    float bottomY = center.y - radius;
    return bottomY >= aabbMax.y - kSeamTolerance;
}

/// @brief AABB内部に入ったSphereを最も近い面から押し出す
/// @param center Sphere中心
/// @param radius Sphere半径
/// @param aabbMin AABB最小座標
/// @param aabbMax AABB最大座標
/// @param outPushDir 押し戻し方向の出力先
/// @param outPenetration 押し戻し量の出力先
static void CalcSphereInsideAABBPush(const Vector3& center, float radius, const Vector3& aabbMin, const Vector3& aabbMax, Vector3& outPushDir, float& outPenetration) {
    static constexpr float kSkinWidth = 0.001f;
    float nearest = center.x - aabbMin.x;
    outPushDir = { -1.0f, 0.0f, 0.0f };

    float distance = aabbMax.x - center.x;
    if (distance < nearest) {
        nearest = distance;
        outPushDir = { 1.0f, 0.0f, 0.0f };
    }

    distance = center.y - aabbMin.y;
    if (distance < nearest) {
        nearest = distance;
        outPushDir = { 0.0f, -1.0f, 0.0f };
    }

    distance = aabbMax.y - center.y;
    if (distance < nearest) {
        nearest = distance;
        outPushDir = { 0.0f, 1.0f, 0.0f };
    }

    distance = center.z - aabbMin.z;
    if (distance < nearest) {
        nearest = distance;
        outPushDir = { 0.0f, 0.0f, -1.0f };
    }

    distance = aabbMax.z - center.z;
    if (distance < nearest) {
        nearest = distance;
        outPushDir = { 0.0f, 0.0f, 1.0f };
    }

    outPenetration = nearest + radius + kSkinWidth;
}

struct SweptAABBHit {
    bool isHit = false;
    float entryTime = 1.0f;
    float exitTime = 1.0f;
};

/// @brief 移動する点とAABBの連続衝突判定を行う
/// @param start 点の開始座標
/// @param end 点の終了座標
/// @param min AABBの最小座標
/// @param max AABBの最大座標
/// @return 衝突時刻を含む判定結果
static SweptAABBHit SweepPointAABB(const Vector3& start, const Vector3& end, const Vector3& min, const Vector3& max) {
    SweptAABBHit result;
    if (IsPointInsideAABB(start, min, max)) {
        return result;
    }

    Vector3 move = end - start;
    float entryTime = 0.0f;
    float exitTime = 1.0f;
    static constexpr float kEpsilon = 0.0001f;

    for (int axis = 0; axis < 3; ++axis) {
        float origin = start[axis];
        float direction = move[axis];
        float axisMin = min[axis];
        float axisMax = max[axis];

        if (std::abs(direction) < kEpsilon) {
            if (origin < axisMin || origin > axisMax) {
                return result;
            }
            continue;
        }

        float t1 = (axisMin - origin) / direction;
        float t2 = (axisMax - origin) / direction;
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        entryTime = std::max(entryTime, t1);
        exitTime = std::min(exitTime, t2);
        if (entryTime > exitTime) {
            return result;
        }
    }

    if (entryTime < 0.0f || entryTime > 1.0f) {
        return result;
    }

    result.isHit = true;
    result.entryTime = std::clamp(entryTime, 0.0f, 1.0f);
    result.exitTime = std::clamp(exitTime, 0.0f, 1.0f);
    return result;
}

/// @brief SphereとAABBの連続衝突判定を行う
/// @param sphereInfo Sphereのコライダー情報
/// @param aabbInfo AABBのコライダー情報
/// @param hitTime 衝突時刻の出力先
/// @return 連続衝突していればtrue
static bool CheckSweptSphereAABB(ColliderManager::ColliderInfo& sphereInfo, ColliderManager::ColliderInfo& aabbInfo, float& hitTime) {
    if (!sphereInfo.hasPreviousPosition || !aabbInfo.hasPreviousPosition ||
        !sphereInfo.pPosition || !aabbInfo.pPosition ||
        !sphereInfo.pShape || !aabbInfo.pShape) {
        return false;
    }

    const Sphere& sphere = std::get<Sphere>(*(sphereInfo.pShape));
    const AABB& aabb = std::get<AABB>(*(aabbInfo.pShape));
    Vector3 radius = { sphere.radius, sphere.radius, sphere.radius };

    Vector3 relativeStart = sphereInfo.previousPosition - aabbInfo.previousPosition;
    Vector3 relativeEnd = *(sphereInfo.pPosition) - *(aabbInfo.pPosition);
    if (IsSphereMovingOnAABBTop(relativeStart, relativeEnd, aabb, sphere.radius)) {
        return false;
    }

    SweptAABBHit hit = SweepPointAABB(relativeStart, relativeEnd, aabb.min - radius, aabb.max + radius);
    if (!hit.isHit) {
        return false;
    }

    hitTime = hit.entryTime;
    return true;
}

/// @brief 指定時刻のSphereとSlopeが衝突しているか判定する
/// @param sphereInfo Sphereのコライダー情報
/// @param slopeInfo Slopeのコライダー情報
/// @param t 判定時刻
/// @return 衝突していればtrue
static bool IsSphereSlopeHitAtTime(ColliderManager::ColliderInfo& sphereInfo, ColliderManager::ColliderInfo& slopeInfo, float t) {
    Sphere sphere = std::get<Sphere>(*(sphereInfo.pShape));
    Slope slope = std::get<Slope>(*(slopeInfo.pShape));
    sphere.center = LerpVector3(sphereInfo.previousPosition, *(sphereInfo.pPosition), t);
    slope.center = LerpVector3(slopeInfo.previousPosition, *(slopeInfo.pPosition), t);
    return Collision::IsHit(sphere, slope);
}

/// @brief Sphere中心からSlope上面までの法線方向距離を計算する
/// @param center Sphere中心座標
/// @param slope 対象Slope
/// @param samplePoint 高さを取得するXZ座標
/// @return Slope上面の法線方向距離
static float CalcSphereSlopeTopSignedDistance(const Vector3& center, const Slope& slope, const Vector3& samplePoint) {
    Vector3 normal = Collision::Detail::GetSlopeTopNormal(slope);
    float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, samplePoint);
    Vector3 planePoint = { samplePoint.x, surfaceY, samplePoint.z };
    return Math::Dot(center - planePoint, normal);
}

/// @brief SphereがSlope上面に接地しているかを法線方向距離で判定する
/// @param sphere 対象Sphere
/// @param slope 対象Slope
/// @param center Sphere中心座標
/// @param xzMargin XZ範囲に追加する余白
/// @param belowTolerance Slope上面より下側に許容する距離
/// @param aboveTolerance Slope上面より上側に許容する距離
/// @return Slope上面に接地していればtrue
static bool IsSphereTouchingSlopeTop(const Sphere& sphere, const Slope& slope, const Vector3& center, float xzMargin, float belowTolerance = 0.08f, float aboveTolerance = 0.08f) {
    if (!Collision::Detail::IsInsideSlopeXZ(slope, center, xzMargin)) {
        return false;
    }

    float signedDistance = CalcSphereSlopeTopSignedDistance(center, slope, center);
    return signedDistance >= -belowTolerance && signedDistance <= sphere.radius + aboveTolerance;
}

/// @brief Slope上面にSphereを接地させる中心Y座標を取得する
/// @param sphere 対象Sphere
/// @param slope 対象Slope
/// @param samplePoint 高さを取得するXZ座標
/// @return Sphere中心のY座標
static float CalcSlopeTopSupportCenterY(const Sphere& sphere, const Slope& slope, const Vector3& samplePoint) {
    Vector3 normal = Collision::Detail::GetSlopeTopNormal(slope);
    float normalY = std::max(normal.y, 0.0001f);
    float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, samplePoint);
    return surfaceY + sphere.radius / normalY;
}

/// @brief Slope上面の法線方向にSphereを押し出すベクトルを取得する
/// @param center Sphere中心座標
/// @param radius Sphere半径
/// @param slope 対象Slope
/// @param outPushVec 押し出しベクトル
/// @return 押し出し可能であればtrue
static bool TryGetSlopeTopPushVector(const Vector3& center, float radius, const Slope& slope, Vector3& outPushVec) {
    static constexpr float kSkinWidth = 0.001f;
    Vector3 worldMin = slope.GetMinWorld();
    Vector3 worldMax = slope.GetMaxWorld();
    Vector3 samplePoint = {
        std::clamp(center.x, worldMin.x, worldMax.x),
        center.y,
        std::clamp(center.z, worldMin.z, worldMax.z)
    };

    if (!Collision::Detail::IsInsideSlopeXZ(slope, center, radius * 0.25f)) {
        return false;
    }

    Vector3 normal = Collision::Detail::GetSlopeTopNormal(slope);
    float signedDistance = CalcSphereSlopeTopSignedDistance(center, slope, samplePoint);
    if (signedDistance < -radius || signedDistance > radius) {
        return false;
    }

    float penetration = radius - signedDistance + kSkinWidth;
    if (penetration <= 0.0f) {
        return false;
    }

    outPushVec = normal * penetration;
    return true;
}

/// @brief 指定時刻のSphereがSlope上面に乗る接触か判定する
/// @param sphereInfo Sphereのコライダー情報
/// @param slopeInfo Slopeのコライダー情報
/// @param t 判定時刻
/// @return 上面接触として扱える場合はtrue
static bool IsSphereOnSlopeTopAtTime(ColliderManager::ColliderInfo& sphereInfo, ColliderManager::ColliderInfo& slopeInfo, float t) {
    Sphere sphere = std::get<Sphere>(*(sphereInfo.pShape));
    Slope slope = std::get<Slope>(*(slopeInfo.pShape));
    sphere.center = LerpVector3(sphereInfo.previousPosition, *(sphereInfo.pPosition), t);
    slope.center = LerpVector3(slopeInfo.previousPosition, *(slopeInfo.pPosition), t);

    return IsSphereTouchingSlopeTop(sphere, slope, sphere.center, sphere.radius);
}

/// @brief Slope側面の接触を歩行用の継ぎ目として無視できるか判定する
/// @param center Sphere中心
/// @param radius Sphere半径
/// @param slope 対象Slope
/// @param samplePoint 高さを取得する座標
/// @return 側面押し戻しを無視できる場合はtrue
static bool CanIgnoreSlopeSideSeam(const Vector3& center, float radius, const Slope& slope, const Vector3& samplePoint) {
    static constexpr float kSeamTolerance = 0.12f;
    Sphere sphere;
    sphere.radius = radius;
    float supportCenterY = CalcSlopeTopSupportCenterY(sphere, slope, samplePoint);
    return center.y >= supportCenterY - kSeamTolerance;
}

/// @brief SphereとSlopeの連続衝突判定を行う
/// @param sphereInfo Sphereのコライダー情報
/// @param slopeInfo Slopeのコライダー情報
/// @param hitTime 衝突時刻の出力先
/// @return 連続衝突していればtrue
static bool CheckSweptSphereSlope(ColliderManager::ColliderInfo& sphereInfo, ColliderManager::ColliderInfo& slopeInfo, float& hitTime) {
    if (!sphereInfo.hasPreviousPosition || !slopeInfo.hasPreviousPosition ||
        !sphereInfo.pPosition || !slopeInfo.pPosition ||
        !sphereInfo.pShape || !slopeInfo.pShape) {
        return false;
    }

    const Sphere& sphere = std::get<Sphere>(*(sphereInfo.pShape));
    const Slope& slope = std::get<Slope>(*(slopeInfo.pShape));
    Vector3 radius = { sphere.radius, sphere.radius, sphere.radius };
    Vector3 slopeMin = slope.min;
    slopeMin.y -= std::max(slope.bottomExtendY, 0.0f);

    Vector3 relativeStart = sphereInfo.previousPosition - slopeInfo.previousPosition;
    Vector3 relativeEnd = *(sphereInfo.pPosition) - *(slopeInfo.pPosition);
    Vector3 broadMin = slopeMin - radius;
    Vector3 broadMax = slope.max + radius;
    bool isStartInsideBroad = IsPointInsideAABB(relativeStart, broadMin, broadMax);
    SweptAABBHit broadHit = SweepPointAABB(relativeStart, relativeEnd, broadMin, broadMax);
    if (!broadHit.isHit && !isStartInsideBroad) {
        return false;
    }

    float searchStart = isStartInsideBroad ? 0.0f : broadHit.entryTime;
    float searchEnd = broadHit.isHit ? broadHit.exitTime : 1.0f;
    if (searchEnd < searchStart) {
        return false;
    }

    if (IsSphereSlopeHitAtTime(sphereInfo, slopeInfo, searchStart)) {
        hitTime = searchStart;
        return true;
    }

    static constexpr int kSearchSteps = 24;
    float lower = searchStart;
    float upper = searchStart;
    bool found = false;

    for (int i = 1; i <= kSearchSteps; ++i) {
        float t = searchStart + (searchEnd - searchStart) * (static_cast<float>(i) / static_cast<float>(kSearchSteps));
        if (IsSphereSlopeHitAtTime(sphereInfo, slopeInfo, t)) {
            lower = searchStart + (searchEnd - searchStart) * (static_cast<float>(i - 1) / static_cast<float>(kSearchSteps));
            upper = t;
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }

    static constexpr int kBinarySearchCount = 8;
    for (int i = 0; i < kBinarySearchCount; ++i) {
        float mid = (lower + upper) * 0.5f;
        if (IsSphereSlopeHitAtTime(sphereInfo, slopeInfo, mid)) {
            upper = mid;
        } else {
            lower = mid;
        }
    }

    hitTime = upper;
    if (IsSphereOnSlopeTopAtTime(sphereInfo, slopeInfo, hitTime)) {
        return false;
    }

    return true;
}

/// @brief Resolve overlap between a sphere and a slope.
/// @param sphereInfo Sphere collider information.
/// @param slopeInfo Slope collider information.
/// @param factorSphere Movement ratio applied to the sphere.
/// @param factorSlope Movement ratio applied to the slope.
static void ResolveSphereSlope(ColliderManager::ColliderInfo& sphereInfo, ColliderManager::ColliderInfo& slopeInfo, float factorSphere, float factorSlope) {
    auto& sphere = std::get<Sphere>(*(sphereInfo.pShape));
    auto& slope = std::get<Slope>(*(slopeInfo.pShape));

    Vector3 worldMin = slope.GetMinWorld();
    Vector3 worldMax = slope.GetMaxWorld();
    Vector3 center = *(sphereInfo.pPosition);
    float radius = sphere.radius;
    static constexpr float kSkinWidth = 0.001f;

    if (center.x < worldMin.x - radius || center.x > worldMax.x + radius ||
        center.y < worldMin.y - radius || center.y > worldMax.y + radius ||
        center.z < worldMin.z - radius || center.z > worldMax.z + radius) {
        return;
    }

    bool insideXZ = Collision::Detail::IsInsideSlopeXZ(slope, center);

    Vector3 topPushVec = { 0.0f, 0.0f, 0.0f };
    if (center.y + radius >= worldMin.y && TryGetSlopeTopPushVector(center, radius, slope, topPushVec)) {
        *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + topPushVec * factorSphere;
        *(slopeInfo.pPosition) = *(slopeInfo.pPosition) - topPushVec * factorSlope;
        return;
    }

    float closestX = std::clamp(center.x, worldMin.x, worldMax.x);
    float closestZ = std::clamp(center.z, worldMin.z, worldMax.z);
    Vector3 closestXZ = { closestX, center.y, closestZ };
    float sideTopY = Collision::Detail::GetSlopeSurfaceY(slope, closestXZ);
    Vector3 closestPoint = {
        closestX,
        std::clamp(center.y, worldMin.y, sideTopY),
        closestZ
    };

    if (!insideXZ && CanIgnoreSlopeSideSeam(center, radius, slope, closestXZ)) {
        return;
    }

    Vector3 diff = center - closestPoint;
    float distanceSq = diff.LengthSq();
    if (distanceSq > radius * radius) {
        return;
    }

    Vector3 pushDir = { 0.0f, 0.0f, 0.0f };
    float penetration = 0.0f;

    if (insideXZ) {
        float distMinX = center.x - worldMin.x;
        float distMaxX = worldMax.x - center.x;
        float distMinZ = center.z - worldMin.z;
        float distMaxZ = worldMax.z - center.z;
        float nearest = distMinX;
        pushDir = { -1.0f, 0.0f, 0.0f };

        if (distMaxX < nearest) {
            nearest = distMaxX;
            pushDir = { 1.0f, 0.0f, 0.0f };
        }
        if (distMinZ < nearest) {
            nearest = distMinZ;
            pushDir = { 0.0f, 0.0f, -1.0f };
        }
        if (distMaxZ < nearest) {
            nearest = distMaxZ;
            pushDir = { 0.0f, 0.0f, 1.0f };
        }

        penetration = nearest + radius + kSkinWidth;
    } else if (distanceSq > 0.0001f) {
        float distance = std::sqrt(distanceSq);
        penetration = radius - distance + kSkinWidth;
        Vector3 horizontalDiff = { diff.x, 0.0f, diff.z };
        float horizontalLength = horizontalDiff.Length();
        if (horizontalLength > 0.0001f) {
            pushDir = horizontalDiff / horizontalLength;
        }
    }

    if (penetration > 0.0f && pushDir.LengthSq() > 0.0001f) {
        Vector3 pushVec = pushDir * penetration;
        *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + pushVec * factorSphere;
        *(slopeInfo.pPosition) = *(slopeInfo.pPosition) - pushVec * factorSlope;
    }
}

/// @brief 指定したタグのコライダー名一覧を取得します。
/// @param tag 検索対象の衝突タグです。
/// @return コライダー名一覧へのポインタを返します。存在しない場合はnullptrを返します。
const std::vector<std::string>* ColliderManager::FindColliderNames(CollisionTag tag) const {
    auto it = m_colliderNamesByTag.find(tag);
    if (it == m_colliderNamesByTag.end()) {
        return nullptr;
    }

    return &it->second;
}

/// @brief 最小最大座標から検索用AABBを作成します。
/// @param min 最小座標です。
/// @param max 最大座標です。
/// @return 作成したAABBです。
static AABB MakeWorldBounds(const Vector3& min, const Vector3& max) {
    AABB bounds;
    bounds.center = { 0.0f, 0.0f, 0.0f };
    bounds.min = min;
    bounds.max = max;
    return bounds;
}

/// @brief AABBの交差判定を行います。
/// @param a AABBです。
/// @param b AABBです。
/// @return 交差していればtrueを返します。
static bool IsBoundsIntersect(const AABB& a, const AABB& b) {
    const Vector3 aMin = a.GetMinWorld();
    const Vector3 aMax = a.GetMaxWorld();
    const Vector3 bMin = b.GetMinWorld();
    const Vector3 bMax = b.GetMaxWorld();

    return aMin.x <= bMax.x && aMax.x >= bMin.x &&
        aMin.y <= bMax.y && aMax.y >= bMin.y &&
        aMin.z <= bMax.z && aMax.z >= bMin.z;
}

/// @brief OBBの外接AABBを作成します。
/// @param obb 対象OBBです。
/// @return 外接AABBです。
static AABB MakeOBBBounds(const OBB& obb) {
    Vector3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vector3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const Vector3 local = {
                    x == 0 ? obb.min.x : obb.max.x,
                    y == 0 ? obb.min.y : obb.max.y,
                    z == 0 ? obb.min.z : obb.max.z
                };
                const Vector3 world = {
                    obb.center.x + obb.orientation[0].x * local.x + obb.orientation[1].x * local.y + obb.orientation[2].x * local.z,
                    obb.center.y + obb.orientation[0].y * local.x + obb.orientation[1].y * local.y + obb.orientation[2].y * local.z,
                    obb.center.z + obb.orientation[0].z * local.x + obb.orientation[1].z * local.y + obb.orientation[2].z * local.z
                };
                min.x = std::min(min.x, world.x);
                min.y = std::min(min.y, world.y);
                min.z = std::min(min.z, world.z);
                max.x = std::max(max.x, world.x);
                max.y = std::max(max.y, world.y);
                max.z = std::max(max.z, world.z);
            }
        }
    }

    return MakeWorldBounds(min, max);
}

/// @brief タグ別索引へコライダー名を追加します。
/// @param tag 追加先の衝突タグです。
/// @param name 追加するコライダー名です。
void ColliderManager::AddColliderNameToTag(CollisionTag tag, const std::string& name) {
    auto& names = m_colliderNamesByTag[tag];
    if (std::find(names.begin(), names.end(), name) != names.end()) {
        return;
    }

    names.push_back(name);
}

/// @brief タグ別索引からコライダー名を削除します。
/// @param tag 削除対象の衝突タグです。
/// @param name 削除するコライダー名です。
void ColliderManager::RemoveColliderNameFromTag(CollisionTag tag, const std::string& name) {
    auto it = m_colliderNamesByTag.find(tag);
    if (it == m_colliderNamesByTag.end()) {
        return;
    }

    auto& names = it->second;
    names.erase(std::remove(names.begin(), names.end(), name), names.end());
    if (names.empty()) {
        m_colliderNamesByTag.erase(it);
    }
}

void ColliderManager::RegisterCollider(const std::string& name, CollisionTag tag, ColliderShape* pShape, Vector3* pPos, float weight, CollisionCallback callback    ) {
    auto existing = m_colliders.find(name);
    if (existing != m_colliders.end()) {
        RemoveColliderNameFromTag(existing->second.tag, name);
    }

    ColliderInfo info;
    info.actorName = name;
    info.tag = tag;
    info.pShape = pShape;
    info.pPosition = pPos;
    info.onHit = callback;
    info.weight = weight;
    if (pPos) {
        info.previousPosition = *pPos;
        info.hasPreviousPosition = true;
    }

    m_colliders[name] = info;
    AddColliderNameToTag(tag, name);
    MarkStaticTerrainBVHDirtyIfNeeded(tag);
#ifdef _DEBUG
    Logger::Output("コライダー登録 : " + name + " (タグ : " + CollisionTagToString(tag) + ")", Logger::Level::Application);
#endif
}

void ColliderManager::RemoveCollider(const std::string& name) {
    auto it = m_colliders.find(name);
    if (it != m_colliders.end()) {
        const CollisionTag removedTag = it->second.tag;
        RemoveColliderNameFromTag(it->second.tag, name);
        m_colliders.erase(it);
        MarkStaticTerrainBVHDirtyIfNeeded(removedTag);
        Logger::Output("コライダー削除 : " + name, Logger::Level::Application);
    } else {
        Logger::Output("コライダー削除失敗 : " + name + " が見つかりません", Logger::Level::Warning);
    }
}

void ColliderManager::RemoveColliderAll() {
    m_colliders.clear();
    m_colliderNamesByTag.clear();
    staticTerrainBVH_.Clear();
    staticTerrainEntries_.clear();
    bvhQueryResults_.clear();
    isStaticTerrainBVHDirty_ = true;
    Logger::Output("すべてのコライダーを削除しました", Logger::Level::Application);
}

/// @brief 衝突ルールを登録する
/// @param tagA グループA
/// @param tagB グループB
/// @param enableResolve 押し戻しを有効にするか
void ColliderManager::RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve) {
    RegisterCollisionPair(tagA, tagB, CollisionPairSetting{ enableResolve, false });
}

/// @brief 衝突ルールを詳細設定付きで登録する
/// @param tagA グループA
/// @param tagB グループB
/// @param setting 押し戻しやCCDの有効状態
void ColliderManager::RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, const CollisionPairSetting& setting) {
    m_matrix[tagA][tagB] = { true, setting.enableResolve, setting.enableCCD };
    m_matrix[tagB][tagA] = { true, setting.enableResolve, setting.enableCCD };
    Logger::Output(
        "衝突ペア登録 : " + CollisionTagToString(tagA) + " <-> " + CollisionTagToString(tagB) +
        " (押し出し : " + (setting.enableResolve ? "有効" : "無効") +
        ", CCD : " + (setting.enableCCD ? "有効" : "無効") + ")",
        Logger::Level::Application
    );
}

/// @brief 衝突ルールを押し戻しとCCD指定付きで登録する
/// @param tagA グループA
/// @param tagB グループB
/// @param enableResolve 押し戻しを有効にするか
/// @param enableCCD 連続衝突判定を有効にするか
void ColliderManager::RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve, bool enableCCD) {
    RegisterCollisionPair(tagA, tagB, CollisionPairSetting{ enableResolve, enableCCD });
}

void ColliderManager::SyncPositions() {
    for (auto& [name, info] : m_colliders) {
        SyncColliderShapePosition(info);
    }
}

bool ColliderManager::CheckVariantCollision(const ColliderShape& shapeA, const ColliderShape& shapeB) {
    ++profileStats_.narrowPhaseCount;
    const bool isSphereSlope =
        (std::holds_alternative<Sphere>(shapeA) && std::holds_alternative<Slope>(shapeB)) ||
        (std::holds_alternative<Slope>(shapeA) && std::holds_alternative<Sphere>(shapeB));
    if (isSphereSlope) {
        ++profileStats_.sphereSlopeNarrowPhaseCount;
    }

    return std::visit([](auto&& typeA, auto&& typeB) -> bool {
        if constexpr (requires { Collision::IsHit(typeA, typeB); }) return Collision::IsHit(typeA, typeB);
        else if constexpr (requires { Collision::IsHit(typeB, typeA); }) return Collision::IsHit(typeB, typeA);
        else return false;
        }, shapeA, shapeB);
}

bool ColliderManager::TryGetColliderBounds(const ColliderInfo& info, AABB& outBounds) const {
    if (!info.pShape) {
        return false;
    }

    ColliderShape syncedShape = GetSyncedShape(info);
    return std::visit([&outBounds](auto&& shape) -> bool {
        using ShapeType = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<ShapeType, AABB>) {
            outBounds = MakeWorldBounds(shape.GetMinWorld(), shape.GetMaxWorld());
            return true;
        } else if constexpr (std::is_same_v<ShapeType, Sphere>) {
            const Vector3 radius = { shape.radius, shape.radius, shape.radius };
            outBounds = MakeWorldBounds(shape.center - radius, shape.center + radius);
            return true;
        } else if constexpr (std::is_same_v<ShapeType, Slope>) {
            outBounds = MakeWorldBounds(shape.GetMinWorld(), shape.GetMaxWorld());
            return true;
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            outBounds = MakeOBBBounds(shape);
            return true;
        } else if constexpr (std::is_same_v<ShapeType, OvalSphere>) {
            const Vector3 radius = {
                std::abs(shape.radius.x),
                std::abs(shape.radius.y),
                std::abs(shape.radius.z)
            };
            outBounds = MakeWorldBounds(shape.center - radius, shape.center + radius);
            return true;
        } else if constexpr (std::is_same_v<ShapeType, Circle>) {
            const Vector3 radius = { shape.radius, shape.radius, shape.radius };
            outBounds = MakeWorldBounds(shape.center - radius, shape.center + radius);
            return true;
        } else if constexpr (std::is_same_v<ShapeType, Segment>) {
            const Vector3 end = shape.origin + shape.diff;
            outBounds = MakeWorldBounds(
                {
                    std::min(shape.origin.x, end.x),
                    std::min(shape.origin.y, end.y),
                    std::min(shape.origin.z, end.z)
                },
                {
                    std::max(shape.origin.x, end.x),
                    std::max(shape.origin.y, end.y),
                    std::max(shape.origin.z, end.z)
                }
            );
            return true;
        } else if constexpr (std::is_same_v<ShapeType, Line>) {
            outBounds = MakeWorldBounds(
                {
                    std::min(shape.start.x, shape.end.x),
                    std::min(shape.start.y, shape.end.y),
                    std::min(shape.start.z, shape.end.z)
                },
                {
                    std::max(shape.start.x, shape.end.x),
                    std::max(shape.start.y, shape.end.y),
                    std::max(shape.start.z, shape.end.z)
                }
            );
            return true;
        } else {
            return false;
        }
    }, syncedShape);
}

bool ColliderManager::IsStaticTerrainTag(CollisionTag tag) const {
    return tag == CollisionTag::MapBlock || tag == CollisionTag::MapSlope;
}

bool ColliderManager::ShouldUseStaticTerrainBVH(CollisionTag tagA, CollisionTag tagB) const {
    return IsStaticTerrainTag(tagA) != IsStaticTerrainTag(tagB);
}

void ColliderManager::MarkStaticTerrainBVHDirtyIfNeeded(CollisionTag tag) {
    if (IsStaticTerrainTag(tag)) {
        isStaticTerrainBVHDirty_ = true;
    }
}

void ColliderManager::RebuildStaticTerrainBVHIfNeeded() {
    if (!isStaticTerrainBVHDirty_) {
        return;
    }

    staticTerrainEntries_.clear();
    staticTerrainEntries_.reserve(m_colliders.size());

    for (const auto& [name, info] : m_colliders) {
        if (!IsStaticTerrainTag(info.tag)) {
            continue;
        }

        AABB bounds;
        if (!TryGetColliderBounds(info, bounds)) {
            continue;
        }

        StaticBVH::Entry entry;
        entry.colliderName = name;
        entry.bounds = bounds;
        staticTerrainEntries_.push_back(entry);
    }

    staticTerrainBVH_.Build(staticTerrainEntries_);
    profileStats_.staticBVHColliderCount = static_cast<uint32_t>(staticTerrainBVH_.GetEntryCount());
    profileStats_.staticBVHNodeCount = static_cast<uint32_t>(staticTerrainBVH_.GetNodeCount());
    isStaticTerrainBVHDirty_ = false;
}

void ColliderManager::QueryStaticTerrainBVH(const AABB& bounds, CollisionTag targetTag, std::vector<const std::string*>& outCandidates) {
    RebuildStaticTerrainBVHIfNeeded();
    staticTerrainBVH_.Query(bounds, outCandidates);
    ++profileStats_.bvhQueryCount;
    const std::vector<std::string>* targetNames = FindColliderNames(targetTag);
    if (targetNames) {
        profileStats_.nonBvhQueryCount += static_cast<uint32_t>(targetNames->size());
    }
    profileStats_.bvhCandidateCount += static_cast<uint32_t>(outCandidates.size());
}

/// @brief 前回座標から現在座標への移動で連続衝突しているか判定する
/// @param a コライダーA
/// @param b コライダーB
/// @param hitTime 衝突時刻の出力先
/// @return 連続衝突していればtrue
bool ColliderManager::CheckSweptCollision(ColliderInfo& a, ColliderInfo& b, float& hitTime) {
    if (!a.pShape || !b.pShape || !a.pPosition || !b.pPosition) {
        return false;
    }

    bool isASphere = std::holds_alternative<Sphere>(*(a.pShape));
    bool isBSphere = std::holds_alternative<Sphere>(*(b.pShape));
    bool isAAABB = std::holds_alternative<AABB>(*(a.pShape));
    bool isBAABB = std::holds_alternative<AABB>(*(b.pShape));
    bool isASlope = std::holds_alternative<Slope>(*(a.pShape));
    bool isBSlope = std::holds_alternative<Slope>(*(b.pShape));

    if ((isASphere && isBAABB) || (isAAABB && isBSphere)) {
        ColliderInfo& sphereInfo = isASphere ? a : b;
        ColliderInfo& aabbInfo = isASphere ? b : a;
        return CheckSweptSphereAABB(sphereInfo, aabbInfo, hitTime);
    }

    if ((isASphere && isBSlope) || (isASlope && isBSphere)) {
        ColliderInfo& sphereInfo = isASphere ? a : b;
        ColliderInfo& slopeInfo = isASphere ? b : a;
        return CheckSweptSphereSlope(sphereInfo, slopeInfo, hitTime);
    }

    return false;
}

/// @brief CCDで求めた衝突時刻へコライダー位置を戻す
/// @param a コライダーA
/// @param b コライダーB
/// @param hitTime 衝突時刻
void ColliderManager::ApplySweptCollision(ColliderInfo& a, ColliderInfo& b, float hitTime) {
    if (!a.pPosition || !b.pPosition || !a.hasPreviousPosition || !b.hasPreviousPosition) {
        return;
    }

    Vector3 moveA = *(a.pPosition) - a.previousPosition;
    Vector3 moveB = *(b.pPosition) - b.previousPosition;
    float relativeLength = (moveA - moveB).Length();
    float backstepTime = relativeLength > 0.0001f ? 0.001f / relativeLength : 0.0f;
    float resolveTime = std::clamp(hitTime - backstepTime, 0.0f, 1.0f);

    *(a.pPosition) = LerpVector3(a.previousPosition, *(a.pPosition), resolveTime);
    *(b.pPosition) = LerpVector3(b.previousPosition, *(b.pPosition), resolveTime);
    SyncColliderShapePosition(a);
    SyncColliderShapePosition(b);
}

/// @brief 現在座標を次回CCD用の前回座標として保存する
void ColliderManager::StorePreviousPositions() {
    for (auto& [name, info] : m_colliders) {
        if (!info.pPosition) {
            info.hasPreviousPosition = false;
            continue;
        }

        info.previousPosition = *(info.pPosition);
        info.hasPreviousPosition = true;
    }
}

void ColliderManager::ResolveOverlap(ColliderInfo& a, ColliderInfo& b) {
    if (!a.pShape || !b.pShape || !a.pPosition || !b.pPosition) return;

    // weight から mobility を計算し、押し戻し比率を決定する
    float mobilityA = 1.0f - std::clamp(a.weight, 0.0f, 1.0f);
    float mobilityB = 1.0f - std::clamp(b.weight, 0.0f, 1.0f);
    float totalMobility = mobilityA + mobilityB;

    // 双方 weight=1.0（完全固定）の場合は何もしない
    if (totalMobility <= 0.0f) return;

    float factorA = mobilityA / totalMobility;
    float factorB = mobilityB / totalMobility;

    // ==========================================
    // 1. Sphere vs Sphere
    // ==========================================
    if (std::holds_alternative<Sphere>(*(a.pShape)) && std::holds_alternative<Sphere>(*(b.pShape))) {
        auto& sphereA = std::get<Sphere>(*(a.pShape));
        auto& sphereB = std::get<Sphere>(*(b.pShape));

        Vector3 diff = *(a.pPosition) - *(b.pPosition);
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float radiusSum = sphereA.radius + sphereB.radius;

        if (distanceSq < radiusSum * radiusSum && distanceSq > 0.0001f) {
            float distance = std::sqrt(distanceSq);
            float penetration = radiusSum - distance;

            Vector3 pushDir = { diff.x / distance, diff.y / distance, diff.z / distance };

            // weight に応じた比率で押し戻す
            *(a.pPosition) = *(a.pPosition) + pushDir * (penetration * factorA);
            *(b.pPosition) = *(b.pPosition) - pushDir * (penetration * factorB);
        }
        return;
    }

    // ==========================================
    // 2. AABB vs AABB (互いに半分ずつ押し戻す)
    // ==========================================
    if (std::holds_alternative<AABB>(*(a.pShape)) && std::holds_alternative<AABB>(*(b.pShape))) {
        auto& aabbA = std::get<AABB>(*(a.pShape));
        auto& aabbB = std::get<AABB>(*(b.pShape));

        Vector3 aMin = aabbA.GetMinWorld();
        Vector3 aMax = aabbA.GetMaxWorld();
        Vector3 bMin = aabbB.GetMinWorld();
        Vector3 bMax = aabbB.GetMaxWorld();

        // 各軸のめり込み量を計算
        float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
        float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
        float overlapZ = std::min(aMax.z, bMax.z) - std::max(aMin.z, bMin.z);

        // すべての軸でめり込んでいる場合のみ押し戻す（CheckCollisionを通っているので基本true）
        if (overlapX > 0 && overlapY > 0 && overlapZ > 0) {
            // 境界ぴったりスナップによる再めり込みを防ぐための微小オフセット
            static constexpr float kSkinWidth = 0.001f;

            // 一番めり込みが浅い軸（最小移動ベクトル）を特定して押し戻す
            Vector3 pushVec = { 0,0,0 };
            if (overlapX < overlapY && overlapX < overlapZ) {
                pushVec.x = (a.pPosition->x < b.pPosition->x) ? -(overlapX + kSkinWidth) : (overlapX + kSkinWidth);
            } else if (overlapY < overlapX && overlapY < overlapZ) {
                pushVec.y = (a.pPosition->y < b.pPosition->y) ? -(overlapY + kSkinWidth) : (overlapY + kSkinWidth);
            } else {
                pushVec.z = (a.pPosition->z < b.pPosition->z) ? -(overlapZ + kSkinWidth) : (overlapZ + kSkinWidth);
            }

            *(a.pPosition) = *(a.pPosition) + pushVec * factorA;
            *(b.pPosition) = *(b.pPosition) - pushVec * factorB;
        }
        return;
    }

    // ==========================================
    // 3. Sphere vs AABB (SphereとAABBの押し戻し)
    // ==========================================
    bool isASphere = std::holds_alternative<Sphere>(*(a.pShape));
    bool isBAABB = std::holds_alternative<AABB>(*(b.pShape));
    bool isAAABB = std::holds_alternative<AABB>(*(a.pShape));
    bool isBSphere = std::holds_alternative<Sphere>(*(b.pShape));

    if ((isASphere && isBAABB) || (isAAABB && isBSphere)) {
        ColliderInfo& sphereInfo = isASphere ? a : b;
        ColliderInfo& aabbInfo = isASphere ? b : a;

        auto& sphere = std::get<Sphere>(*(sphereInfo.pShape));
        auto& aabb = std::get<AABB>(*(aabbInfo.pShape));

        Vector3 aabbMin = aabb.GetMinWorld();
        Vector3 aabbMax = aabb.GetMaxWorld();
        Vector3 center = *(sphereInfo.pPosition);
        static constexpr float kSkinWidth = 0.001f;

        if (IsSphereOnAABBTop(center, sphere.radius, aabbMin, aabbMax)) {
            float pushY = aabbMax.y + sphere.radius + kSkinWidth - center.y;
            if (pushY > 0.0f) {
                float factorSphere = isASphere ? factorA : factorB;
                float factorAabb = isASphere ? factorB : factorA;
                Vector3 pushVec = { 0.0f, pushY, 0.0f };
                *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + pushVec * factorSphere;
                *(aabbInfo.pPosition) = *(aabbInfo.pPosition) - pushVec * factorAabb;
            }
            return;
        }

        // AABB上の、Sphereの中心に一番近い点（ClosestPoint）を求める
        Vector3 closestPoint = {
            std::clamp(center.x, aabbMin.x, aabbMax.x),
            std::clamp(center.y, aabbMin.y, aabbMax.y),
            std::clamp(center.z, aabbMin.z, aabbMax.z)
        };

        Vector3 diff = center - closestPoint;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        // 中心がAABBの中に入り込んでしまっている（激しいめり込み）場合の特殊処理
        if (distSq <= 0.0001f) {
            Vector3 pushDir = { 0.0f, 0.0f, 0.0f };
            float penetration = 0.0f;
            CalcSphereInsideAABBPush(center, sphere.radius, aabbMin, aabbMax, pushDir, penetration);

            float factorSphere = isASphere ? factorA : factorB;
            float factorAabb = isASphere ? factorB : factorA;
            *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + pushDir * (penetration * factorSphere);
            *(aabbInfo.pPosition) = *(aabbInfo.pPosition) - pushDir * (penetration * factorAabb);
            return;
        }

        if (distSq < sphere.radius * sphere.radius) {
            float distance = std::sqrt(distSq);
            float penetration = sphere.radius - distance + kSkinWidth;
            Vector3 pushDir = { diff.x / distance, diff.y / distance, diff.z / distance };

            if (std::abs(pushDir.y) < 0.25f && CanIgnoreAABBSideSeam(center, sphere.radius, aabbMax)) {
                return;
            }

            // weight に応じた比率で押し戻す（sphereInfo/aabbInfo が a/b のどちらかに対応）
            float factorSphere = isASphere ? factorA : factorB;
            float factorAabb   = isASphere ? factorB : factorA;
            *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + pushDir * (penetration * factorSphere);
            *(aabbInfo.pPosition)   = *(aabbInfo.pPosition)   - pushDir * (penetration * factorAabb);
        }
        return;
    }

    // ==========================================
    // 4. Sphere vs Plane (Planeは静止物として扱い、Sphereを100%押し戻す)
    // ==========================================
    bool isASlope = std::holds_alternative<Slope>(*(a.pShape));
    bool isBSlope = std::holds_alternative<Slope>(*(b.pShape));

    if ((isASphere && isBSlope) || (isASlope && isBSphere)) {
        ColliderInfo& sphereInfo = isASphere ? a : b;
        ColliderInfo& slopeInfo = isASphere ? b : a;
        float factorSphere = isASphere ? factorA : factorB;
        float factorSlope = isASphere ? factorB : factorA;
        ResolveSphereSlope(sphereInfo, slopeInfo, factorSphere, factorSlope);
        return;
    }

    bool isBPlane = std::holds_alternative<Plane>(*(b.pShape));
    bool isAPlane = std::holds_alternative<Plane>(*(a.pShape));

    if ((isASphere && isBPlane) || (isAPlane && isBSphere)) {
        ColliderInfo& sphereInfo = isASphere ? a : b;
        ColliderInfo& planeInfo = isASphere ? b : a;

        auto& sphere = std::get<Sphere>(*(sphereInfo.pShape));
        auto& plane = std::get<Plane>(*(planeInfo.pShape));

        // 平面の法線を正規化
        Vector3 normal = { plane.normal.x, plane.normal.y, plane.normal.z }; // 正規化済みと仮定

        // 平面から球の中心までの距離
        Vector3 toSphere = *(sphereInfo.pPosition) - plane.center;
        float distToPlane = toSphere.x * normal.x + toSphere.y * normal.y + toSphere.z * normal.z;

        // 球の半径より近ければめり込んでいる
        if (std::abs(distToPlane) < sphere.radius) {
            float penetration = sphere.radius - std::abs(distToPlane);
            float sign = (distToPlane < 0.0f) ? -1.0f : 1.0f;

            // weight に応じた比率で押し戻す
            float factorSphere = isASphere ? factorA : factorB;
            float factorPlane  = isASphere ? factorB : factorA;
            *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + normal * (penetration * sign * factorSphere);
            *(planeInfo.pPosition)  = *(planeInfo.pPosition)  - normal * (penetration * sign * factorPlane);
        }
        return;
    }
}

bool ColliderManager::IsGroundContact(const std::string& name, CollisionTag targetTag) {
    auto it = m_colliders.find(name);
    if (it == m_colliders.end()) return false;

    const ColliderInfo& playerInfo = it->second;
    if (!playerInfo.pShape || !playerInfo.pPosition) return false;

    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) return false;

    // ==========================================
    // 1. プレイヤーが AABB の場合の接地判定（既存コード）
    // ==========================================
    if (std::holds_alternative<AABB>(*(playerInfo.pShape))) {
        auto& playerAABB = std::get<AABB>(*(playerInfo.pShape));
        Vector3 aMin = playerAABB.GetMinWorld();
        Vector3 aMax = playerAABB.GetMaxWorld();

        for (const auto& otherName : *targetNames) {
            if (otherName == name) continue;
            auto otherIt = m_colliders.find(otherName);
            if (otherIt == m_colliders.end()) continue;

            const ColliderInfo& otherInfo = otherIt->second;
            if (!otherInfo.pShape || !otherInfo.pPosition) continue;
            if (!std::holds_alternative<AABB>(*(otherInfo.pShape))) continue;

            auto& boxAABB = std::get<AABB>(*(otherInfo.pShape));
            Vector3 bMin = boxAABB.GetMinWorld();
            Vector3 bMax = boxAABB.GetMaxWorld();

            float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
            float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
            float overlapZ = std::min(aMax.z, bMax.z) - std::max(aMin.z, bMin.z);

            if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) continue;

            if (overlapY <= overlapX && overlapY <= overlapZ) {
                if (playerInfo.pPosition->y >= otherInfo.pPosition->y) {
                    return true;
                }
            }
        }
    }
    // ==========================================
    // 2. プレイヤーが Sphere の場合の接地判定（ここを追加）
    // ==========================================
    else if (std::holds_alternative<Sphere>(*(playerInfo.pShape))) {
        auto& sphere = std::get<Sphere>(*(playerInfo.pShape));
        auto checkSlopeGround = [&playerInfo, &sphere](const Slope& slope) -> bool {
            return IsSphereTouchingSlopeTop(sphere, slope, *(playerInfo.pPosition), sphere.radius * 0.25f, 0.05f, 0.08f);
        };

        if (IsStaticTerrainTag(targetTag)) {
            AABB queryBounds;
            if (TryGetColliderBounds(playerInfo, queryBounds)) {
                // 接地判定ではSphere下端がAABB上面より少し上にある状態も候補に含めます。
                queryBounds.min.y -= 0.08f;
                QueryStaticTerrainBVH(queryBounds, targetTag, bvhQueryResults_);
                for (const std::string* candidateName : bvhQueryResults_) {
                    if (!candidateName || *candidateName == name) continue;
                    auto otherIt = m_colliders.find(*candidateName);
                    if (otherIt == m_colliders.end()) continue;

                    const ColliderInfo& otherInfo = otherIt->second;
                    if (otherInfo.tag != targetTag) continue;
                    if (!otherInfo.pShape || !otherInfo.pPosition) continue;

                    if (std::holds_alternative<Slope>(*(otherInfo.pShape))) {
                        ++profileStats_.narrowPhaseCount;
                        ++profileStats_.sphereSlopeNarrowPhaseCount;
                        if (checkSlopeGround(std::get<Slope>(*(otherInfo.pShape)))) {
                            return true;
                        }
                        continue;
                    }
                    if (!std::holds_alternative<AABB>(*(otherInfo.pShape))) continue;

                    auto& boxAABB = std::get<AABB>(*(otherInfo.pShape));
                    Vector3 bMin = boxAABB.GetMinWorld();
                    Vector3 bMax = boxAABB.GetMaxWorld();

                    ++profileStats_.narrowPhaseCount;
                    float closestX = std::clamp(playerInfo.pPosition->x, bMin.x, bMax.x);
                    float closestZ = std::clamp(playerInfo.pPosition->z, bMin.z, bMax.z);

                    float dx = playerInfo.pPosition->x - closestX;
                    float dz = playerInfo.pPosition->z - closestZ;
                    float distSqXZ = dx * dx + dz * dz;

                    if (distSqXZ <= sphere.radius * sphere.radius) {
                        if (playerInfo.pPosition->y >= bMax.y - 0.05f) {
                            float bottomY = playerInfo.pPosition->y - sphere.radius;
                            if (bottomY <= bMax.y + 0.05f) {
                                return true;
                            }
                        }
                    }
                }

                return false;
            }
        }

        for (const auto& otherName : *targetNames) {
            if (otherName == name) continue;
            auto otherIt = m_colliders.find(otherName);
            if (otherIt == m_colliders.end()) continue;

            const ColliderInfo& otherInfo = otherIt->second;
            if (!otherInfo.pShape || !otherInfo.pPosition) continue;
            // 対象がAABB(MapBlock等)の場合のみ判定
            if (std::holds_alternative<Slope>(*(otherInfo.pShape))) {
                if (checkSlopeGround(std::get<Slope>(*(otherInfo.pShape)))) {
                    return true;
                }
                continue;
            }
            if (!std::holds_alternative<AABB>(*(otherInfo.pShape))) continue;

            auto& boxAABB = std::get<AABB>(*(otherInfo.pShape));
            Vector3 bMin = boxAABB.GetMinWorld();
            Vector3 bMax = boxAABB.GetMaxWorld();

            // Sphereの中心から、AABB上面へのXZ平面上での最短距離を求める
            float closestX = std::clamp(playerInfo.pPosition->x, bMin.x, bMax.x);
            float closestZ = std::clamp(playerInfo.pPosition->z, bMin.z, bMax.z);

            float dx = playerInfo.pPosition->x - closestX;
            float dz = playerInfo.pPosition->z - closestZ;
            float distSqXZ = dx * dx + dz * dz;

            // XZ平面上でAABBに乗っている（はみ出しの半径内含む）か
            if (distSqXZ <= sphere.radius * sphere.radius) {
                // 壁張り付きを防止するため、球の中心がAABBの上面(あるいは極めてその近く)より上にあること
                if (playerInfo.pPosition->y >= bMax.y - 0.05f) {

                    // 球の最下端座標を計算
                    float bottomY = playerInfo.pPosition->y - sphere.radius;

                    // 最下端がAABB上面付近に接触・めり込んでいる場合を接地とする
                    // （押し出し誤差等を吸収するために +0.05f の猶予を持たせる）
                    if (bottomY <= bMax.y + 0.05f) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/// @brief 指定したタグ同士で地面接触しているかを判定します。
/// @param selfTag 接地を判定する側のタグです。
/// @param targetTag 地面として扱う側のタグです。
/// @return 接地していればtrueを返します。
bool ColliderManager::IsGroundContact(CollisionTag selfTag, CollisionTag targetTag) {
    const auto* selfNames = FindColliderNames(selfTag);
    if (!selfNames) {
        return false;
    }

    for (const auto& name : *selfNames) {
        if (IsGroundContact(name, targetTag)) {
            return true;
        }
    }

    return false;
}

/// @brief 指定した個体が、対象タグのスロープ面に接触しているかを判定します。
/// @param name 個体の識別名です。
/// @param targetTag スロープとして扱うタグです。
/// @return スロープ面に接触していればtrueを返します。
bool ColliderManager::IsSlopeGroundContact(const std::string& name, CollisionTag targetTag) {
    auto it = m_colliders.find(name);
    if (it == m_colliders.end()) return false;

    const ColliderInfo& playerInfo = it->second;
    if (!playerInfo.pShape || !playerInfo.pPosition) return false;
    if (!std::holds_alternative<Sphere>(*(playerInfo.pShape))) return false;

    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) return false;

    const auto& sphere = std::get<Sphere>(*(playerInfo.pShape));

    if (IsStaticTerrainTag(targetTag)) {
        AABB queryBounds;
        if (TryGetColliderBounds(playerInfo, queryBounds)) {
            QueryStaticTerrainBVH(queryBounds, targetTag, bvhQueryResults_);
            for (const std::string* candidateName : bvhQueryResults_) {
                if (!candidateName || *candidateName == name) continue;
                auto otherIt = m_colliders.find(*candidateName);
                if (otherIt == m_colliders.end()) continue;

                const ColliderInfo& otherInfo = otherIt->second;
                if (otherInfo.tag != targetTag) continue;
                if (!otherInfo.pShape || !otherInfo.pPosition) continue;
                if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

                const auto& slope = std::get<Slope>(*(otherInfo.pShape));
                ++profileStats_.narrowPhaseCount;
                ++profileStats_.sphereSlopeNarrowPhaseCount;
                if (IsSphereTouchingSlopeTop(sphere, slope, *(playerInfo.pPosition), sphere.radius * 0.25f, 0.05f, 0.08f)) {
                    return true;
                }
            }

            return false;
        }
    }

    for (const auto& otherName : *targetNames) {
        if (otherName == name) continue;
        auto otherIt = m_colliders.find(otherName);
        if (otherIt == m_colliders.end()) continue;

        const ColliderInfo& otherInfo = otherIt->second;
        if (!otherInfo.pShape || !otherInfo.pPosition) continue;
        if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

        const auto& slope = std::get<Slope>(*(otherInfo.pShape));
        if (IsSphereTouchingSlopeTop(sphere, slope, *(playerInfo.pPosition), sphere.radius * 0.25f, 0.05f, 0.08f)) {
            return true;
        }
    }

    return false;
}

/// @brief 指定したタグ同士で坂地面に接触しているかを判定します。
/// @param selfTag 接地を判定する側のタグです。
/// @param targetTag 坂地面として扱う側のタグです。
/// @return 坂地面に接地していればtrueを返します。
bool ColliderManager::IsSlopeGroundContact(CollisionTag selfTag, CollisionTag targetTag) {
    const auto* selfNames = FindColliderNames(selfTag);
    if (!selfNames) {
        return false;
    }

    for (const auto& name : *selfNames) {
        if (IsSlopeGroundContact(name, targetTag)) {
            return true;
        }
    }

    return false;
}

/// @brief 指定座標の直下にある地表面のY座標を取得します。
/// @param origin 地表面を探す基準座標です。
/// @param targetTag 地面として扱う対象タグです。
/// @param outSurfaceY 見つかった地表面のY座標です。
/// @param maxDistance 下方向に探索する最大距離です。
/// @return 地表面が見つかった場合はtrueを返します。
bool ColliderManager::TryGetGroundSurfaceY(const Vector3& origin, CollisionTag targetTag, float& outSurfaceY, float maxDistance) {
    if (maxDistance < 0.0f) {
        return false;
    }

    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) {
        return false;
    }

    bool foundSurface = false;
    float bestSurfaceY = -FLT_MAX;

    const auto evaluateCollider = [&](const ColliderInfo& colliderInfo) {
        if (!colliderInfo.pShape || !colliderInfo.pPosition) {
            return;
        }

        float surfaceY = 0.0f;
        if (std::holds_alternative<AABB>(*(colliderInfo.pShape))) {
            AABB ground = std::get<AABB>(*(colliderInfo.pShape));
            ground.center = *(colliderInfo.pPosition);
            const Vector3 worldMin = ground.GetMinWorld();
            const Vector3 worldMax = ground.GetMaxWorld();
            if (origin.x < worldMin.x || origin.x > worldMax.x ||
                origin.z < worldMin.z || origin.z > worldMax.z) {
                return;
            }

            surfaceY = worldMax.y;
        } else if (std::holds_alternative<Slope>(*(colliderInfo.pShape))) {
            Slope slope = std::get<Slope>(*(colliderInfo.pShape));
            slope.center = *(colliderInfo.pPosition);
            if (!Collision::Detail::IsInsideSlopeXZ(slope, origin)) {
                return;
            }

            surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, origin);
        } else {
            return;
        }

        const float distance = origin.y - surfaceY;
        if (distance < 0.0f || distance > maxDistance) {
            return;
        }

        if (!foundSurface || surfaceY > bestSurfaceY) {
            foundSurface = true;
            bestSurfaceY = surfaceY;
        }
    };

    if (IsStaticTerrainTag(targetTag)) {
        RebuildStaticTerrainBVHIfNeeded();

        constexpr float kGroundQueryHalfWidth = 0.001f;
        AABB queryBounds;
        queryBounds.center = origin;
        queryBounds.min = { -kGroundQueryHalfWidth, -maxDistance, -kGroundQueryHalfWidth };
        queryBounds.max = { kGroundQueryHalfWidth, 0.0f, kGroundQueryHalfWidth };
        QueryStaticTerrainBVH(queryBounds, targetTag, bvhQueryResults_);

        for (const std::string* candidateName : bvhQueryResults_) {
            if (!candidateName) {
                continue;
            }

            auto colliderIt = m_colliders.find(*candidateName);
            if (colliderIt == m_colliders.end() || colliderIt->second.tag != targetTag) {
                continue;
            }

            evaluateCollider(colliderIt->second);
        }
    } else {
        for (const auto& targetName : *targetNames) {
            auto colliderIt = m_colliders.find(targetName);
            if (colliderIt == m_colliders.end()) {
                continue;
            }

            evaluateCollider(colliderIt->second);
        }
    }

    if (!foundSurface) {
        return false;
    }

    outSurfaceY = bestSurfaceY;
    return true;
}

/// @brief Sphereコライダーが追従できるSlope上面の中心Y座標を取得します。
/// @param name Sphereコライダーの識別名です。
/// @param targetTag Slopeとして扱うタグです。
/// @param outCenterY Sphere中心に設定するY座標の出力先です。
/// @param maxSnapDownDistance 下方向に追従できる最大距離です。
/// @return 追従できるSlopeが見つかればtrueを返します。
bool ColliderManager::TryGetSlopeGroundCenterY(const std::string& name, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance) {
    auto it = m_colliders.find(name);
    if (it == m_colliders.end()) return false;

    const ColliderInfo& playerInfo = it->second;
    if (!playerInfo.pShape || !playerInfo.pPosition) return false;
    if (!std::holds_alternative<Sphere>(*(playerInfo.pShape))) return false;

    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) return false;

    const auto& sphere = std::get<Sphere>(*(playerInfo.pShape));
    const Vector3& center = *(playerInfo.pPosition);
    bool foundSlope = false;
    float bestCenterY = 0.0f;
    float bestSurfaceY = -FLT_MAX;

    if (IsStaticTerrainTag(targetTag)) {
        AABB queryBounds;
        if (TryGetColliderBounds(playerInfo, queryBounds)) {
            QueryStaticTerrainBVH(queryBounds, targetTag, bvhQueryResults_);
            for (const std::string* candidateName : bvhQueryResults_) {
                if (!candidateName || *candidateName == name) continue;
                auto otherIt = m_colliders.find(*candidateName);
                if (otherIt == m_colliders.end()) continue;

                const ColliderInfo& otherInfo = otherIt->second;
                if (otherInfo.tag != targetTag) continue;
                if (!otherInfo.pShape || !otherInfo.pPosition) continue;
                if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

                Slope slope = std::get<Slope>(*(otherInfo.pShape));
                slope.center = *(otherInfo.pPosition);
                ++profileStats_.narrowPhaseCount;
                ++profileStats_.sphereSlopeNarrowPhaseCount;
                if (!Collision::Detail::IsInsideSlopeXZ(slope, center)) {
                    continue;
                }

                float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, center);
                float targetCenterY = CalcSlopeTopSupportCenterY(sphere, slope, center);
                float snapDownDistance = center.y - targetCenterY;
                if (snapDownDistance < 0.0f || snapDownDistance > maxSnapDownDistance) {
                    continue;
                }

                if (!foundSlope || surfaceY > bestSurfaceY) {
                    foundSlope = true;
                    bestSurfaceY = surfaceY;
                    bestCenterY = targetCenterY;
                }
            }

            if (!foundSlope) {
                return false;
            }

            outCenterY = bestCenterY;
            return true;
        }
    }

    for (const auto& otherName : *targetNames) {
        if (otherName == name) continue;
        auto otherIt = m_colliders.find(otherName);
        if (otherIt == m_colliders.end()) continue;

        const ColliderInfo& otherInfo = otherIt->second;
        if (!otherInfo.pShape || !otherInfo.pPosition) continue;
        if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

        Slope slope = std::get<Slope>(*(otherInfo.pShape));
        slope.center = *(otherInfo.pPosition);
        if (!Collision::Detail::IsInsideSlopeXZ(slope, center)) {
            continue;
        }

        float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, center);
        float targetCenterY = CalcSlopeTopSupportCenterY(sphere, slope, center);
        float snapDownDistance = center.y - targetCenterY;
        if (snapDownDistance < 0.0f || snapDownDistance > maxSnapDownDistance) {
            continue;
        }

        if (!foundSlope || surfaceY > bestSurfaceY) {
            foundSlope = true;
            bestSurfaceY = surfaceY;
            bestCenterY = targetCenterY;
        }
    }

    if (!foundSlope) {
        return false;
    }

    outCenterY = bestCenterY;
    return true;
}

/// @brief 指定したタグ同士で追従できるSlope上面の中心Y座標を取得します。
/// @param selfTag Sphereコライダーとして扱う側のタグです。
/// @param targetTag Slopeとして扱う側のタグです。
/// @param outCenterY Sphere中心に設定するY座標の出力先です。
/// @param maxSnapDownDistance 下方向に追従できる最大距離です。
/// @return 追従できるSlopeが見つかればtrueを返します。
bool ColliderManager::TryGetSlopeGroundCenterY(CollisionTag selfTag, CollisionTag targetTag, float& outCenterY, float maxSnapDownDistance) {
    const auto* selfNames = FindColliderNames(selfTag);
    if (!selfNames) {
        return false;
    }

    for (const auto& name : *selfNames) {
        if (TryGetSlopeGroundCenterY(name, targetTag, outCenterY, maxSnapDownDistance)) {
            return true;
        }
    }

    return false;
}

/// @brief Sphereコライダーが接地しているSlope上面の法線を取得します。
/// @param name Sphereコライダーの識別名です。
/// @param targetTag Slopeとして扱うタグです。
/// @param outNormal Slope上面の法線の出力先です。
/// @return 接地しているSlopeが見つかればtrueを返します。
bool ColliderManager::TryGetSlopeGroundNormal(const std::string& name, CollisionTag targetTag, Vector3& outNormal) {
    auto it = m_colliders.find(name);
    if (it == m_colliders.end()) return false;

    const ColliderInfo& playerInfo = it->second;
    if (!playerInfo.pShape || !playerInfo.pPosition) return false;
    if (!std::holds_alternative<Sphere>(*(playerInfo.pShape))) return false;

    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) return false;

    const auto& sphere = std::get<Sphere>(*(playerInfo.pShape));
    const Vector3& center = *(playerInfo.pPosition);
    bool foundSlope = false;
    float bestSurfaceY = -FLT_MAX;
    Vector3 bestNormal = { 0.0f, 1.0f, 0.0f };

    if (IsStaticTerrainTag(targetTag)) {
        AABB queryBounds;
        if (TryGetColliderBounds(playerInfo, queryBounds)) {
            QueryStaticTerrainBVH(queryBounds, targetTag, bvhQueryResults_);
            for (const std::string* candidateName : bvhQueryResults_) {
                if (!candidateName || *candidateName == name) continue;
                auto otherIt = m_colliders.find(*candidateName);
                if (otherIt == m_colliders.end()) continue;

                const ColliderInfo& otherInfo = otherIt->second;
                if (otherInfo.tag != targetTag) continue;
                if (!otherInfo.pShape || !otherInfo.pPosition) continue;
                if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

                Slope slope = std::get<Slope>(*(otherInfo.pShape));
                slope.center = *(otherInfo.pPosition);
                ++profileStats_.narrowPhaseCount;
                ++profileStats_.sphereSlopeNarrowPhaseCount;
                if (!IsSphereTouchingSlopeTop(sphere, slope, center, sphere.radius * 0.25f, 0.05f, 0.08f)) {
                    continue;
                }

                float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, center);
                if (!foundSlope || surfaceY > bestSurfaceY) {
                    foundSlope = true;
                    bestSurfaceY = surfaceY;
                    bestNormal = Collision::Detail::GetSlopeTopNormal(slope);
                }
            }

            if (!foundSlope) {
                return false;
            }

            outNormal = bestNormal;
            return true;
        }
    }

    for (const auto& otherName : *targetNames) {
        if (otherName == name) continue;
        auto otherIt = m_colliders.find(otherName);
        if (otherIt == m_colliders.end()) continue;

        const ColliderInfo& otherInfo = otherIt->second;
        if (!otherInfo.pShape || !otherInfo.pPosition) continue;
        if (!std::holds_alternative<Slope>(*(otherInfo.pShape))) continue;

        Slope slope = std::get<Slope>(*(otherInfo.pShape));
        slope.center = *(otherInfo.pPosition);
        if (!IsSphereTouchingSlopeTop(sphere, slope, center, sphere.radius * 0.25f, 0.05f, 0.08f)) {
            continue;
        }

        float surfaceY = Collision::Detail::GetSlopeSurfaceY(slope, center);
        if (!foundSlope || surfaceY > bestSurfaceY) {
            foundSlope = true;
            bestSurfaceY = surfaceY;
            bestNormal = Collision::Detail::GetSlopeTopNormal(slope);
        }
    }

    if (!foundSlope) {
        return false;
    }

    outNormal = bestNormal;
    return true;
}

/// @brief 指定したタグ同士で接地しているSlope上面の法線を取得します。
/// @param selfTag Sphereコライダーとして扱う側のタグです。
/// @param targetTag Slopeとして扱う側のタグです。
/// @param outNormal Slope上面の法線の出力先です。
/// @return 接地しているSlopeが見つかればtrueを返します。
bool ColliderManager::TryGetSlopeGroundNormal(CollisionTag selfTag, CollisionTag targetTag, Vector3& outNormal) {
    const auto* selfNames = FindColliderNames(selfTag);
    if (!selfNames) {
        return false;
    }

    for (const auto& name : *selfNames) {
        if (TryGetSlopeGroundNormal(name, targetTag, outNormal)) {
            return true;
        }
    }

    return false;
}

bool ColliderManager::IsHitName(const std::string& nameA, const std::string& nameB) {
    if (m_colliders.find(nameA) == m_colliders.end() || m_colliders.find(nameB) == m_colliders.end()) {
        return false;
    }
    auto shapeA = GetSyncedShape(m_colliders[nameA]);
    auto shapeB = GetSyncedShape(m_colliders[nameB]);
    return CheckVariantCollision(shapeA, shapeB);
}

bool ColliderManager::IsHitWithTag(const std::string& name, CollisionTag targetTag) {
    if (m_colliders.find(name) == m_colliders.end()) {
        return false;
    }
    const auto* targetNames = FindColliderNames(targetTag);
    if (!targetNames) {
        return false;
    }

    auto shapeA = GetSyncedShape(m_colliders[name]);
    for (const auto& otherName : *targetNames) {
        if (otherName == name) continue;
        auto itB = m_colliders.find(otherName);
        if (itB == m_colliders.end()) continue;

        const auto& infoB = itB->second;
        auto shapeB = GetSyncedShape(infoB);
        if (CheckVariantCollision(shapeA, shapeB)) {
            return true;
        }
    }
    return false;
}

bool ColliderManager::IsHitTags(CollisionTag tagA, CollisionTag tagB) {
    const auto* namesA = FindColliderNames(tagA);
    const auto* namesB = FindColliderNames(tagB);
    if (!namesA || !namesB) {
        return false;
    }

    if (tagA == tagB) {
        for (size_t i = 0; i < namesA->size(); ++i) {
            auto itA = m_colliders.find((*namesA)[i]);
            if (itA == m_colliders.end()) continue;
            auto shapeA = GetSyncedShape(itA->second);

            for (size_t j = i + 1; j < namesA->size(); ++j) {
                auto itB = m_colliders.find((*namesA)[j]);
                if (itB == m_colliders.end()) continue;
                auto shapeB = GetSyncedShape(itB->second);

                if (CheckVariantCollision(shapeA, shapeB)) {
                    return true;
                }
            }
        }
        return false;
    }

    for (const auto& nameA : *namesA) {
        auto itA = m_colliders.find(nameA);
        if (itA == m_colliders.end()) continue;
        const auto& infoA = itA->second;
        auto shapeA = GetSyncedShape(infoA);

        for (const auto& nameB : *namesB) {
            auto itB = m_colliders.find(nameB);
            if (itB == m_colliders.end()) continue;
            const auto& infoB = itB->second;
            auto shapeB = GetSyncedShape(infoB);

            if (CheckVariantCollision(shapeA, shapeB)) {
                return true;
            }
        }
    }
    return false;
}

void ColliderManager::ProcessCollisionPair(ColliderInfo& a, ColliderInfo& b, const CollisionRule& rule) {
    if (!a.pShape || !b.pShape) {
        return;
    }

    bool isHit = CheckVariantCollision(*(a.pShape), *(b.pShape));
    bool isSweptHit = false;
    float hitTime = 1.0f;
    if (!isHit && rule.enableCCD) {
        isSweptHit = CheckSweptCollision(a, b, hitTime);
        if (isSweptHit && rule.enableResolve) {
            ApplySweptCollision(a, b, hitTime);
        }
    }

    if (!isHit && !isSweptHit) {
        return;
    }

    if (a.onHit) a.onHit(b.tag, b.actorName);
    if (b.onHit) b.onHit(a.tag, a.actorName);
    if (rule.enableResolve) {
        ResolveOverlap(a, b);
        SyncColliderShapePosition(a);
        SyncColliderShapePosition(b);
    }
}

void ColliderManager::ProcessStaticTerrainPair(std::vector<ColliderInfo*>& dynamicList, CollisionTag staticTag, const CollisionRule& rule) {
    for (ColliderInfo* dynamicInfo : dynamicList) {
        if (!dynamicInfo || !dynamicInfo->pShape) {
            continue;
        }

        AABB queryBounds;
        if (!TryGetColliderBounds(*dynamicInfo, queryBounds)) {
            continue;
        }

        QueryStaticTerrainBVH(queryBounds, staticTag, bvhQueryResults_);
        for (const std::string* candidateName : bvhQueryResults_) {
            if (!candidateName || *candidateName == dynamicInfo->actorName) {
                continue;
            }

            auto candidateIt = m_colliders.find(*candidateName);
            if (candidateIt == m_colliders.end()) {
                continue;
            }

            ColliderInfo& staticInfo = candidateIt->second;
            if (staticInfo.tag != staticTag) {
                continue;
            }

            ProcessCollisionPair(*dynamicInfo, staticInfo, rule);
        }
    }
}

void ColliderManager::Update() {
    {
        const auto updateStartTime = std::chrono::high_resolution_clock::now();
        const uint32_t previousStaticColliderCount = profileStats_.staticBVHColliderCount;
        const uint32_t previousStaticNodeCount = profileStats_.staticBVHNodeCount;
        profileStats_ = {};
        profileStats_.staticBVHColliderCount = previousStaticColliderCount;
        profileStats_.staticBVHNodeCount = previousStaticNodeCount;

        SyncPositions();
        RebuildStaticTerrainBVHIfNeeded();

        std::unordered_map<CollisionTag, std::vector<ColliderInfo*>> groups;
        for (auto& [name, info] : m_colliders) {
            groups[info.tag].push_back(&info);
        }

        for (auto& [tagA, pairsMap] : m_matrix) {
            for (auto& [tagB, rule] : pairsMap) {
                if (!rule.isRegistered) {
                    continue;
                }
                if (static_cast<int>(tagA) > static_cast<int>(tagB)) {
                    continue;
                }

                auto itA = groups.find(tagA);
                auto itB = groups.find(tagB);
                if (itA == groups.end() || itB == groups.end()) {
                    continue;
                }

                auto& listA = itA->second;
                auto& listB = itB->second;

                if (tagA == tagB) {
                    for (size_t i = 0; i < listA.size(); ++i) {
                        for (size_t j = i + 1; j < listA.size(); ++j) {
                            ProcessCollisionPair(*listA[i], *listA[j], rule);
                        }
                    }
                    continue;
                }

                if (ShouldUseStaticTerrainBVH(tagA, tagB)) {
                    if (IsStaticTerrainTag(tagA)) {
                        ProcessStaticTerrainPair(listB, tagA, rule);
                    } else {
                        ProcessStaticTerrainPair(listA, tagB, rule);
                    }
                    continue;
                }

                for (ColliderInfo* pA : listA) {
                    for (ColliderInfo* pB : listB) {
                        ProcessCollisionPair(*pA, *pB, rule);
                    }
                }
            }
        }

        StorePreviousPositions();
        const auto updateEndTime = std::chrono::high_resolution_clock::now();
        profileStats_.updateTimeMs = std::chrono::duration<double, std::milli>(updateEndTime - updateStartTime).count();
    }

#if 0

    // タグ別にコライダーをグループ化
    std::unordered_map<CollisionTag, std::vector<ColliderInfo*>> groups;
    for (auto& [name, info] : m_colliders) {
        groups[info.tag].push_back(&info);
    }

    // 登録されたペアのみをチェック（O(N²)全探索を回避）
    for (auto& [tagA, pairsMap] : m_matrix) {
        for (auto& [tagB, rule] : pairsMap) {
            if (!rule.isRegistered) continue;
            // 重複チェックを避けるため tagA <= tagB のみ処理
            if (static_cast<int>(tagA) > static_cast<int>(tagB)) continue;

            auto itA = groups.find(tagA);
            auto itB = groups.find(tagB);
            if (itA == groups.end() || itB == groups.end()) continue;

            auto& listA = itA->second;
            auto& listB = itB->second;

            if (tagA == tagB) {
                // 同タグ間（例: Enemy vs Enemy）
                for (size_t i = 0; i < listA.size(); ++i) {
                    for (size_t j = i + 1; j < listA.size(); ++j) {
                        ProcessCollisionPair(*listA[i], *listA[j], rule);
                    }
                }
            } else {
                // 異タグ間（例: PlayerAABB vs MapBlock）
                for (auto* pA : listA) {
                    for (auto* pB : listB) {
                        ColliderInfo& a = *pA;
                        ColliderInfo& b = *pB;
                        if (!a.pShape || !b.pShape) continue;
                        bool isHit = CheckVariantCollision(*(a.pShape), *(b.pShape));
                        bool isSweptHit = false;
                        float hitTime = 1.0f;
                        if (!isHit && rule.enableCCD) {
                            isSweptHit = CheckSweptCollision(a, b, hitTime);
                            if (isSweptHit && rule.enableResolve) {
                                ApplySweptCollision(a, b, hitTime);
                            }
                        }

                        if (isHit || isSweptHit) {
                            if (a.onHit) a.onHit(b.tag, b.actorName);
                            if (b.onHit) b.onHit(a.tag, a.actorName);
                            if (rule.enableResolve) {
                                ResolveOverlap(a, b);
                                SyncColliderShapePosition(a);
                                SyncColliderShapePosition(b);
                            }
                        }
                    }
                }
            }
        }
    }

    StorePreviousPositions();
#endif
}

const ColliderManager::ProfileStats& ColliderManager::GetProfileStats() const {
    return profileStats_;
}

#ifdef USE_IMGUI
void ColliderManager::DrawImGui() {
    ImGui::Begin("Collider");
    ImGui::Text("Update Time : %.3f ms", profileStats_.updateTimeMs);
    ImGui::Text("Static BVH Colliders : %u", profileStats_.staticBVHColliderCount);
    ImGui::Text("Static BVH Nodes : %u", profileStats_.staticBVHNodeCount);
    ImGui::Separator();
    ImGui::Text("BVH Query Count : %u", profileStats_.bvhQueryCount);
    ImGui::Text("Non-BVH Query Count : %u", profileStats_.nonBvhQueryCount);
    ImGui::Text("BVH Candidate Count : %u", profileStats_.bvhCandidateCount);
    ImGui::Text("Narrow Phase Count : %u", profileStats_.narrowPhaseCount);
    ImGui::Text("Sphere vs Slope Count : %u", profileStats_.sphereSlopeNarrowPhaseCount);
    ImGui::End();
}
#endif // USE_IMGUI
