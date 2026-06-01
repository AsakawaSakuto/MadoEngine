#include "ColliderManager.h"
#include "Utility/Logger/Logger.h"

// pShape から実体を取得してコピーを作るように変更
static Shape GetSyncedShape(const ColliderManager::ColliderInfo& info) {
    if (!info.pShape) return {}; // 万が一ポインタがnullの場合は空を返すなどの保護

    Shape syncedShape = *(info.pShape);
    if (info.pPosition) {
        std::visit([&info](auto& s) {
            if constexpr (requires { s.center; }) {
                s.center = *(info.pPosition);
            }
            }, syncedShape);
    }
    return syncedShape;
}

// ★ 引数を Shape* に変更
void ColliderManager::RegisterCollider(const std::string& name, CollisionTag tag, Shape* pShape, Vector3* pPos, float weight, CollisionCallback callback    ) {
    m_colliders[name] = { name, tag, pShape, pPos, callback, weight };
#ifdef _DEBUG
    Logger::Output("コライダー登録 : " + name + " (タグ : " + CollisionTagToString(tag) + ")", Logger::Level::Application);
#endif
}

void ColliderManager::RemoveCollider(const std::string& name) {
    if (m_colliders.find(name) != m_colliders.end()) {
        m_colliders.erase(name);
        Logger::Output("コライダー削除 : " + name, Logger::Level::Application);
    } else {
        Logger::Output("コライダー削除失敗 : " + name + " が見つかりません", Logger::Level::Warning);
    }
}

void ColliderManager::RegisterCollisionPair(CollisionTag tagA, CollisionTag tagB, bool enableResolve) {
    m_matrix[tagA][tagB] = { true, enableResolve };
    m_matrix[tagB][tagA] = { true, enableResolve };
    Logger::Output("衝突ペア登録 : " + CollisionTagToString(tagA) + " <-> " + CollisionTagToString(tagB) + " (押し出し : " + (enableResolve ? "有効" : "無効") + ")", Logger::Level::Application);
}

void ColliderManager::SyncPositions() {
    for (auto& [name, info] : m_colliders) {
        if (!info.pPosition || !info.pShape) continue; // ★ Nullチェック追加

        // ★ pShape の中身に対して visit を行う
        std::visit([&info](auto& shapeActual) {
            if constexpr (requires { shapeActual.center; }) {
                shapeActual.center = *(info.pPosition);
            }
            }, *(info.pShape));
    }
}

bool ColliderManager::CheckVariantCollision(const Shape& shapeA, const Shape& shapeB) {
    return std::visit([](auto&& typeA, auto&& typeB) -> bool {
        if constexpr (requires { Collision::IsHit(typeA, typeB); }) return Collision::IsHit(typeA, typeB);
        else if constexpr (requires { Collision::IsHit(typeB, typeA); }) return Collision::IsHit(typeB, typeA);
        else return false;
        }, shapeA, shapeB);
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

        // AABB上の、Sphereの中心に一番近い点（ClosestPoint）を求める
        Vector3 closestPoint = {
            std::clamp(sphereInfo.pPosition->x, aabbMin.x, aabbMax.x),
            std::clamp(sphereInfo.pPosition->y, aabbMin.y, aabbMax.y),
            std::clamp(sphereInfo.pPosition->z, aabbMin.z, aabbMax.z)
        };

        Vector3 diff = *(sphereInfo.pPosition) - closestPoint;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        // 中心がAABBの中に入り込んでしまっている（激しいめり込み）場合の特殊処理
        if (distSq == 0.0f) {
            // ここでは簡易的にY軸上へ押し上げる処理を入れるなど工夫が必要
            *(sphereInfo.pPosition) = *(sphereInfo.pPosition) + Vector3{ 0, sphere.radius, 0 };
            return;
        }

        if (distSq < sphere.radius * sphere.radius) {
            float distance = std::sqrt(distSq);
            float penetration = sphere.radius - distance;
            Vector3 pushDir = { diff.x / distance, diff.y / distance, diff.z / distance };

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

    // ==========================================
    // 1. プレイヤーが AABB の場合の接地判定（既存コード）
    // ==========================================
    if (std::holds_alternative<AABB>(*(playerInfo.pShape))) {
        auto& playerAABB = std::get<AABB>(*(playerInfo.pShape));
        Vector3 aMin = playerAABB.GetMinWorld();
        Vector3 aMax = playerAABB.GetMaxWorld();

        for (const auto& [otherName, otherInfo] : m_colliders) {
            if (otherName == name) continue;
            if (otherInfo.tag != targetTag) continue;
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

        for (const auto& [otherName, otherInfo] : m_colliders) {
            if (otherName == name) continue;
            if (otherInfo.tag != targetTag) continue;
            if (!otherInfo.pShape || !otherInfo.pPosition) continue;
            // 対象がAABB(MapBlock等)の場合のみ判定
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
    auto shapeA = GetSyncedShape(m_colliders[name]);
    for (const auto& [otherName, infoB] : m_colliders) {
        if (otherName == name) continue;
        if (infoB.tag != targetTag) continue;

        auto shapeB = GetSyncedShape(infoB);
        if (CheckVariantCollision(shapeA, shapeB)) {
            return true;
        }
    }
    return false;
}

bool ColliderManager::IsHitTags(CollisionTag tagA, CollisionTag tagB) {
    if (tagA == tagB) {
        for (auto itA = m_colliders.begin(); itA != m_colliders.end(); ++itA) {
            if (itA->second.tag != tagA) continue;
            auto shapeA = GetSyncedShape(itA->second);

            for (auto itB = std::next(itA); itB != m_colliders.end(); ++itB) {
                if (itB->second.tag != tagB) continue;
                auto shapeB = GetSyncedShape(itB->second);

                if (CheckVariantCollision(shapeA, shapeB)) {
                    return true;
                }
            }
        }
        return false;
    }

    for (const auto& [nameA, infoA] : m_colliders) {
        if (infoA.tag != tagA) continue;
        auto shapeA = GetSyncedShape(infoA);

        for (const auto& [nameB, infoB] : m_colliders) {
            if (infoB.tag != tagB) continue;
            auto shapeB = GetSyncedShape(infoB);

            if (CheckVariantCollision(shapeA, shapeB)) {
                return true;
            }
        }
    }
    return false;
}

void ColliderManager::Update() {
    SyncPositions();

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
                        ColliderInfo& a = *listA[i];
                        ColliderInfo& b = *listA[j];
                        if (!a.pShape || !b.pShape) continue;
                        if (CheckVariantCollision(*(a.pShape), *(b.pShape))) {
                            if (a.onHit) a.onHit(b.tag, b.actorName);
                            if (b.onHit) b.onHit(a.tag, a.actorName);
                            if (rule.enableResolve) ResolveOverlap(a, b);
                        }
                    }
                }
            } else {
                // 異タグ間（例: PlayerAABB vs MapBlock）
                for (auto* pA : listA) {
                    for (auto* pB : listB) {
                        ColliderInfo& a = *pA;
                        ColliderInfo& b = *pB;
                        if (!a.pShape || !b.pShape) continue;
                        if (CheckVariantCollision(*(a.pShape), *(b.pShape))) {
                            if (a.onHit) a.onHit(b.tag, b.actorName);
                            if (b.onHit) b.onHit(a.tag, a.actorName);
                            if (rule.enableResolve) ResolveOverlap(a, b);
                        }
                    }
                }
            }
        }
    }
}