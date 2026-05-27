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
void ColliderManager::RegisterCollider(const std::string& name, const std::string& tag, Shape* pShape, Vector3* pPos, CollisionCallback callback) {
    m_colliders[name] = { name, tag, pShape, pPos, callback };
    Logger::Output("コライダー登録 : " + name + " (タグ : " + tag + ")", Logger::Level::Application);
}

void ColliderManager::RemoveCollider(const std::string& name) {
    if (m_colliders.find(name) != m_colliders.end()) {
        m_colliders.erase(name);
        Logger::Output("コライダー削除 : " + name, Logger::Level::Application);
    } else {
        Logger::Output("コライダー削除失敗 : " + name + " が見つかりません", Logger::Level::Warning);
    }
}

void ColliderManager::RegisterCollisionPair(const std::string& tagA, const std::string& tagB, bool enableResolve) {
    m_matrix[tagA][tagB] = { true, enableResolve };
    m_matrix[tagB][tagA] = { true, enableResolve };
    Logger::Output("衝突ペア登録 : " + tagA + " <-> " + tagB + " (押し出し : " + (enableResolve ? "有効" : "無効") + ")", Logger::Level::Application);
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

    // ★ pShape をデリファレンスして判定
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

            *(a.pPosition) = { a.pPosition->x + pushDir.x * (penetration * 0.5f),
                               a.pPosition->y + pushDir.y * (penetration * 0.5f),
                               a.pPosition->z + pushDir.z * (penetration * 0.5f) };

            *(b.pPosition) = { b.pPosition->x - pushDir.x * (penetration * 0.5f),
                               b.pPosition->y - pushDir.y * (penetration * 0.5f),
                               b.pPosition->z - pushDir.z * (penetration * 0.5f) };
        }
    }
}

bool ColliderManager::IsHitName(const std::string& nameA, const std::string& nameB) {
    if (m_colliders.find(nameA) == m_colliders.end() || m_colliders.find(nameB) == m_colliders.end()) {
        return false;
    }
    auto shapeA = GetSyncedShape(m_colliders[nameA]);
    auto shapeB = GetSyncedShape(m_colliders[nameB]);
    return CheckVariantCollision(shapeA, shapeB);
}

bool ColliderManager::IsHitWithTag(const std::string& name, const std::string& targetTag) {
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

bool ColliderManager::IsHitTags(const std::string& tagA, const std::string& tagB) {
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

    for (auto itA = m_colliders.begin(); itA != m_colliders.end(); ++itA) {
        for (auto itB = std::next(itA); itB != m_colliders.end(); ++itB) {
            ColliderInfo& a = itA->second;
            ColliderInfo& b = itB->second;

            if (!m_matrix[a.tag][b.tag].isRegistered) continue;
            if (!a.pShape || !b.pShape) continue; // ★ Nullチェック

            // ★ デリファレンスして判定
            if (CheckVariantCollision(*(a.pShape), *(b.pShape))) {
                if (a.onHit) a.onHit(b.tag, b.actorName);
                if (b.onHit) b.onHit(a.tag, a.actorName);

                if (m_matrix[a.tag][b.tag].enableResolve) {
                    ResolveOverlap(a, b);
                }
            }
        }
    }
}