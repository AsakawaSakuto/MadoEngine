#include "Line.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Logger/Logger.h"
#include <cmath>
#include <numbers>
#include <cassert>
#include <cstring>

void Line3d::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, uint32_t maxVertices) {
    Logger::Output("Line3d初期化開始", Logger::Level::Engine);

    device_ = device;
    commandList_ = commandList;
    maxVertices_ = maxVertices;

    // 頂点バッファを予め確保
    vertices_.reserve(maxVertices_);

    // PSODescの設定
    psoDesc_.blendMode   = MadoEngine::Render::BlendMode::Normal;
    psoDesc_.depthMode   = MadoEngine::Render::DepthMode::ReadWrite;
    psoDesc_.cullMode    = MadoEngine::Render::CullMode::None;
    psoDesc_.fillMode    = MadoEngine::Render::FillMode::Solid;
    psoDesc_.topology    = MadoEngine::Render::TopologyType::Line;
    psoDesc_.inputLayout = MadoEngine::Render::InputLayoutType::Line;
    psoDesc_.vsKey       = "Object3d/Line/Line.VS";
    psoDesc_.psKey       = "Object3d/Line/Line.PS";
    psoDesc_.rootSigKey  = "Line3d.RootSig";

    // リソース作成
    CreateVertexBuffer();
    CreateTransformBuffer();

    Logger::Output("Line3d初期化完了", Logger::Level::Engine);
}

void Line3d::SetPSORegistry(MadoEngine::Render::PSORegistry* psoRegistry) {
    psoRegistry_ = psoRegistry;
}

Line3d::~Line3d() {
    if (vertexBuffer_) {
        vertexBuffer_->Unmap(0, nullptr);
    }
    if (transformBuffer_) {
        transformBuffer_->Unmap(0, nullptr);
    }
}

void Line3d::AddLine(const Vector3& start, const Vector3& end, const Vector4& color) {
    if (vertices_.size() + 2 > maxVertices_) {
        Logger::Output("警告: Line3d頂点バッファ容量超過", Logger::Level::Warning);
        return;
    }
    // 頂点を追加
    vertices_.push_back({ start, color });
    vertices_.push_back({ end, color });
}

void Line3d::AddLines(const std::vector<Vector3>& points, const Vector4& color) {
    if (points.size() < 2) return;

    for (size_t i = 0; i < points.size() - 1; ++i) {
        AddLine(points[i], points[i + 1], color);
    }
}

void Line3d::AddGrid(float size, int divisions, const Vector4& color) {
    float step = size / divisions;
    float halfSize = size * 0.5f;

    // X軸方向の線（Z軸に平行な線）
    for (int i = 0; i <= divisions; ++i) {
        float z = -halfSize + step * i;

        // 原点を通る線（X軸、Z=0）は赤色
        Vector4 lineColor = (std::abs(z) < 0.001f) ? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f } : color;

        AddLine(
            { -halfSize, 0.0f, z },
            { halfSize, 0.0f, z },
            lineColor
        );
    }

    // Z軸方向の線（X軸に平行な線）
    for (int i = 0; i <= divisions; ++i) {
        float x = -halfSize + step * i;

        // 原点を通る線（Z軸、X=0）は緑色
        Vector4 lineColor = (std::abs(x) < 0.001f) ? Vector4{ 0.0f, 1.0f, 0.0f, 1.0f } : color;

        AddLine(
            { x, 0.0f, -halfSize },
            { x, 0.0f, halfSize },
            lineColor
        );
    }
}

void Line3d::AddBox(const Vector3& center, const Vector3& size, const Vector4& color) {
    Vector3 halfSize = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };

    // 8つの頂点
    Vector3 vertices[8] = {
        { center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z }, // 0: 左下前
        { center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z }, // 1: 右下前
        { center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z }, // 2: 右上前
        { center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z }, // 3: 左上前
        { center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z }, // 4: 左下奥
        { center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z }, // 5: 右下奥
        { center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z }, // 6: 右上奥
        { center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z }  // 7: 左上奥
    };

    // 前面
    AddLine(vertices[0], vertices[1], color);
    AddLine(vertices[1], vertices[2], color);
    AddLine(vertices[2], vertices[3], color);
    AddLine(vertices[3], vertices[0], color);

    // 背面
    AddLine(vertices[4], vertices[5], color);
    AddLine(vertices[5], vertices[6], color);
    AddLine(vertices[6], vertices[7], color);
    AddLine(vertices[7], vertices[4], color);

    // 側面
    AddLine(vertices[0], vertices[4], color);
    AddLine(vertices[1], vertices[5], color);
    AddLine(vertices[2], vertices[6], color);
    AddLine(vertices[3], vertices[7], color);
}

void Line3d::AddBox(const AABB& aabb, const Vector4& color) {
    // AABBのワールド座標のmin/maxから8つの頂点を定義
    Vector3 worldMin = aabb.center + aabb.min;
    Vector3 worldMax = aabb.center + aabb.max;

    Vector3 vertices[8] = {
        { worldMin.x, worldMin.y, worldMin.z }, // 0: 左下前
        { worldMax.x, worldMin.y, worldMin.z }, // 1: 右下前
        { worldMax.x, worldMax.y, worldMin.z }, // 2: 右上前
        { worldMin.x, worldMax.y, worldMin.z }, // 3: 左上前
        { worldMin.x, worldMin.y, worldMax.z }, // 4: 左下奥
        { worldMax.x, worldMin.y, worldMax.z }, // 5: 右下奥
        { worldMax.x, worldMax.y, worldMax.z }, // 6: 右上奥
        { worldMin.x, worldMax.y, worldMax.z }  // 7: 左上奥
    };

    // 前面（z = min）
    AddLine(vertices[0], vertices[1], color);
    AddLine(vertices[1], vertices[2], color);
    AddLine(vertices[2], vertices[3], color);
    AddLine(vertices[3], vertices[0], color);

    // 背面（z = max）
    AddLine(vertices[4], vertices[5], color);
    AddLine(vertices[5], vertices[6], color);
    AddLine(vertices[6], vertices[7], color);
    AddLine(vertices[7], vertices[4], color);

    // 側面の辺（前面と背面を繋ぐ）
    AddLine(vertices[0], vertices[4], color);
    AddLine(vertices[1], vertices[5], color);
    AddLine(vertices[2], vertices[6], color);
    AddLine(vertices[3], vertices[7], color);
}

void Line3d::AddBox(const OBB& obb, const Vector4& color) {
    // OBBのローカル座標系での8つの頂点を計算
    Vector3 center = obb.center;

    // ローカル座標系での8つのコーナー（min/maxのローカルオフセットから直接生成）
    Vector3 localVertices[8] = {
        { obb.min.x, obb.min.y, obb.min.z }, // 0: 左下前
        { obb.max.x, obb.min.y, obb.min.z }, // 1: 右下前
        { obb.max.x, obb.max.y, obb.min.z }, // 2: 右上前
        { obb.min.x, obb.max.y, obb.min.z }, // 3: 左上前
        { obb.min.x, obb.min.y, obb.max.z }, // 4: 左下奥
        { obb.max.x, obb.min.y, obb.max.z }, // 5: 右下奥
        { obb.max.x, obb.max.y, obb.max.z }, // 6: 右上奥
        { obb.min.x, obb.max.y, obb.max.z }  // 7: 左上奥
    };

    // ワールド座標系へ変換（orientation行列を使用して回転し、centerで平行移動）
    Vector3 worldVertices[8];
    for (int i = 0; i < 8; ++i) {
        // orientation[0], orientation[1], orientation[2]はそれぞれX, Y, Z軸の基底ベクトル
        worldVertices[i] = {
            center.x + localVertices[i].x * obb.orientation[0].x + localVertices[i].y * obb.orientation[1].x + localVertices[i].z * obb.orientation[2].x,
            center.y + localVertices[i].x * obb.orientation[0].y + localVertices[i].y * obb.orientation[1].y + localVertices[i].z * obb.orientation[2].y,
            center.z + localVertices[i].x * obb.orientation[0].z + localVertices[i].y * obb.orientation[1].z + localVertices[i].z * obb.orientation[2].z
        };
    }

    // 前面（z = min）
    AddLine(worldVertices[0], worldVertices[1], color);
    AddLine(worldVertices[1], worldVertices[2], color);
    AddLine(worldVertices[2], worldVertices[3], color);
    AddLine(worldVertices[3], worldVertices[0], color);

    // 背面（z = max）
    AddLine(worldVertices[4], worldVertices[5], color);
    AddLine(worldVertices[5], worldVertices[6], color);
    AddLine(worldVertices[6], worldVertices[7], color);
    AddLine(worldVertices[7], worldVertices[4], color);

    // 側面の辺（前面と背面を繋ぐ）
    AddLine(worldVertices[0], worldVertices[4], color);
    AddLine(worldVertices[1], worldVertices[5], color);
    AddLine(worldVertices[2], worldVertices[6], color);
    AddLine(worldVertices[3], worldVertices[7], color);
}

void Line3d::AddSphere(const Sphere& sphere, const Vector4& color) {
    const float pi = std::numbers::pi_v<float>;

    // 経度線（縦の円）を描画
    for (int i = 0; i < longitudeDivisions_; ++i) {
        float longitude = (2.0f * pi * i) / longitudeDivisions_;

        // Y軸周りに回転した円を描画
        for (int j = 0; j < segments_; ++j) {
            float latitude1 = (-pi / 2.0f) + (pi * j) / segments_;
            float latitude2 = (-pi / 2.0f) + (pi * (j + 1)) / segments_;

            Vector3 p1 = {
                sphere.center.x + sphere.radius * std::cos(latitude1) * std::cos(longitude),
                sphere.center.y + sphere.radius * std::sin(latitude1),
                sphere.center.z + sphere.radius * std::cos(latitude1) * std::sin(longitude)
            };
            Vector3 p2 = {
                sphere.center.x + sphere.radius * std::cos(latitude2) * std::cos(longitude),
                sphere.center.y + sphere.radius * std::sin(latitude2),
                sphere.center.z + sphere.radius * std::cos(latitude2) * std::sin(longitude)
            };
            AddLine(p1, p2, color);
        }
    }

    // 緯度線（横の円）を描画
    for (int i = 1; i < latitudeDivisions_; ++i) {
        float latitude = (-pi / 2.0f) + (pi * i) / latitudeDivisions_;
        float y = sphere.radius * std::sin(latitude);
        float circleRadius = sphere.radius * std::cos(latitude);

        // 各緯度での円を描画
        for (int j = 0; j < segments_; ++j) {
            float angle1 = (2.0f * pi * j) / segments_;
            float angle2 = (2.0f * pi * (j + 1)) / segments_;

            Vector3 p1 = {
                sphere.center.x + circleRadius * std::cos(angle1),
                sphere.center.y + y,
                sphere.center.z + circleRadius * std::sin(angle1)
            };
            Vector3 p2 = {
                sphere.center.x + circleRadius * std::cos(angle2),
                sphere.center.y + y,
                sphere.center.z + circleRadius * std::sin(angle2)
            };
            AddLine(p1, p2, color);
        }
    }
}

void Line3d::AddOvalSphere(const OvalSphere& ovalSphere, const Vector4& color) {
    const float pi = std::numbers::pi_v<float>;

    // 経度線（縦の円）を描画 - 各軸の半径を考慮した楕円
    for (int i = 0; i < longitudeDivisions_; ++i) {
        float longitude = (2.0f * pi * i) / longitudeDivisions_;

        // Y軸周りに回転した楕円を描画
        for (int j = 0; j < segments_; ++j) {
            float latitude1 = (-pi / 2.0f) + (pi * j) / segments_;
            float latitude2 = (-pi / 2.0f) + (pi * (j + 1)) / segments_;

            // ローカル座標系での楕円の点を計算
            Vector3 localP1 = {
                ovalSphere.radius.x * std::cos(latitude1) * std::cos(longitude),
                ovalSphere.radius.y * std::sin(latitude1),
                ovalSphere.radius.z * std::cos(latitude1) * std::sin(longitude)
            };
            Vector3 localP2 = {
                ovalSphere.radius.x * std::cos(latitude2) * std::cos(longitude),
                ovalSphere.radius.y * std::sin(latitude2),
                ovalSphere.radius.z * std::cos(latitude2) * std::sin(longitude)
            };

            // ローカル座標からワールド座標へ変換（orientationで回転 + centerで平行移動）
            Vector3 p1 = {
                ovalSphere.center.x + localP1.x * ovalSphere.orientation[0].x + localP1.y * ovalSphere.orientation[1].x + localP1.z * ovalSphere.orientation[2].x,
                ovalSphere.center.y + localP1.x * ovalSphere.orientation[0].y + localP1.y * ovalSphere.orientation[1].y + localP1.z * ovalSphere.orientation[2].y,
                ovalSphere.center.z + localP1.x * ovalSphere.orientation[0].z + localP1.y * ovalSphere.orientation[1].z + localP1.z * ovalSphere.orientation[2].z
            };
            Vector3 p2 = {
                ovalSphere.center.x + localP2.x * ovalSphere.orientation[0].x + localP2.y * ovalSphere.orientation[1].x + localP2.z * ovalSphere.orientation[2].x,
                ovalSphere.center.y + localP2.x * ovalSphere.orientation[0].y + localP2.y * ovalSphere.orientation[1].y + localP2.z * ovalSphere.orientation[2].y,
                ovalSphere.center.z + localP2.x * ovalSphere.orientation[0].z + localP2.y * ovalSphere.orientation[1].z + localP2.z * ovalSphere.orientation[2].z
            };

            AddLine(p1, p2, color);
        }
    }

    // 緯度線（横の円）を描画 - 各軸の半径を考慮した楕円
    for (int i = 1; i < latitudeDivisions_; ++i) {
        float latitude = (-pi / 2.0f) + (pi * i) / latitudeDivisions_;
        float y = ovalSphere.radius.y * std::sin(latitude);

        // 楕円の断面での半径を計算
        float cosLatitude = std::cos(latitude);
        float circleRadiusX = ovalSphere.radius.x * cosLatitude;
        float circleRadiusZ = ovalSphere.radius.z * cosLatitude;

        // 各緯度での楕円を描画
        for (int j = 0; j < segments_; ++j) {
            float angle1 = (2.0f * pi * j) / segments_;
            float angle2 = (2.0f * pi * (j + 1)) / segments_;

            // ローカル座標系での楕円の点を計算
            Vector3 localP1 = {
                circleRadiusX * std::cos(angle1),
                y,
                circleRadiusZ * std::sin(angle1)
            };
            Vector3 localP2 = {
                circleRadiusX * std::cos(angle2),
                y,
                circleRadiusZ * std::sin(angle2)
            };

            // ローカル座標からワールド座標へ変換（orientationで回転 + centerで平行移動）
            Vector3 p1 = {
                ovalSphere.center.x + localP1.x * ovalSphere.orientation[0].x + localP1.y * ovalSphere.orientation[1].x + localP1.z * ovalSphere.orientation[2].x,
                ovalSphere.center.y + localP1.x * ovalSphere.orientation[0].y + localP1.y * ovalSphere.orientation[1].y + localP1.z * ovalSphere.orientation[2].y,
                ovalSphere.center.z + localP1.x * ovalSphere.orientation[0].z + localP1.y * ovalSphere.orientation[1].z + localP1.z * ovalSphere.orientation[2].z
            };
            Vector3 p2 = {
                ovalSphere.center.x + localP2.x * ovalSphere.orientation[0].x + localP2.y * ovalSphere.orientation[1].x + localP2.z * ovalSphere.orientation[2].x,
                ovalSphere.center.y + localP2.x * ovalSphere.orientation[0].y + localP2.y * ovalSphere.orientation[1].y + localP2.z * ovalSphere.orientation[2].y,
                ovalSphere.center.z + localP2.x * ovalSphere.orientation[0].z + localP2.y * ovalSphere.orientation[1].z + localP2.z * ovalSphere.orientation[2].z
            };

            AddLine(p1, p2, color);
        }
    }
}

void Line3d::AddCircle(const Circle& circle, const Vector4& color) {

    Vector3 center = circle.center;
    float radius = circle.radius;
    Vector3 normal = circle.normal;

    const float pi = std::numbers::pi_v<float>;

    // 法線から適当な接線ベクトルを作成
    Vector3 tangent;
    if (std::abs(normal.x) < 0.9f) {
        tangent = Math::Normalize(Math::Cross({ 1.0f, 0.0f, 0.0f }, normal));
    } else {
        tangent = Math::Normalize(Math::Cross({ 0.0f, 1.0f, 0.0f }, normal));
    }
    Vector3 bitangent = Math::Normalize(Math::Cross(normal, tangent));

    // 円を描画
    for (int i = 0; i < segments_; ++i) {
        float angle1 = (2.0f * pi * i) / segments_;
        float angle2 = (2.0f * pi * (i + 1)) / segments_;

        Vector3 offset1 = {
            tangent.x * std::cos(angle1) + bitangent.x * std::sin(angle1),
            tangent.y * std::cos(angle1) + bitangent.y * std::sin(angle1),
            tangent.z * std::cos(angle1) + bitangent.z * std::sin(angle1)
        };
        Vector3 offset2 = {
            tangent.x * std::cos(angle2) + bitangent.x * std::sin(angle2),
            tangent.y * std::cos(angle2) + bitangent.y * std::sin(angle2),
            tangent.z * std::cos(angle2) + bitangent.z * std::sin(angle2)
        };

        Vector3 p1 = {
            center.x + offset1.x * radius,
            center.y + offset1.y * radius,
            center.z + offset1.z * radius
        };
        Vector3 p2 = {
            center.x + offset2.x * radius,
            center.y + offset2.y * radius,
            center.z + offset2.z * radius
        };

        AddLine(p1, p2, color);
    }
}

void Line3d::AddPoint(const Vector3& position, const Vector4& color) {
    Sphere pointSphere = { position, 0.1f }; // 小さな球として点を表現
    AddSphere(pointSphere, color);
}

void Line3d::AddRay(const Vector3& origin, const Vector3& direction, float length, const Vector4& color) {
    Vector3 end = {
        origin.x + direction.x * length,
        origin.y + direction.y * length,
        origin.z + direction.z * length
    };
    AddLine(origin, end, color);
}

void Line3d::AddSegment(const Segment& segment, const Vector4& color) {
    // Segmentは origin（始点）と diff（終点への差分ベクトル）で定義される
    Vector3 start = segment.origin;
    Vector3 end = {
        segment.origin.x + segment.diff.x,
        segment.origin.y + segment.diff.y,
        segment.origin.z + segment.diff.z
    };
    AddLine(start, end, color);
}

void Line3d::AddLine(const Line& line, const Vector4& color) {
    // Line構造体は start（始点）と end（終点）で定義される
    AddLine(line.start, line.end, color);
}

void Line3d::AddPlane(const Plane& plane, int divisions, const Vector4& color) {
    // 平面の法線から、平面上の2つの接線ベクトルを計算
    Vector3 tangent;
    Vector3 bitangent;

    // 法線から適当な接線ベクトルを作成
    if (std::abs(plane.normal.x) < 0.9f) {
        tangent = Math::Normalize(Math::Cross({ 1.0f, 0.0f, 0.0f }, plane.normal));
    } else {
        tangent = Math::Normalize(Math::Cross({ 0.0f, 1.0f, 0.0f }, plane.normal));
    }
    bitangent = Math::Normalize(Math::Cross(plane.normal, tangent));

    // 平面の中心を使用
    Vector3 planeOrigin = plane.center;

    // グリッドを描画
    float step = plane.size / divisions;
    float halfSize = plane.size * 0.5f;

    // 接線方向（tangent）に平行な線を描画
    for (int i = 0; i <= divisions; ++i) {
        float offset = -halfSize + step * i;

        Vector3 start = {
            planeOrigin.x + tangent.x * (-halfSize) + bitangent.x * offset,
            planeOrigin.y + tangent.y * (-halfSize) + bitangent.y * offset,
            planeOrigin.z + tangent.z * (-halfSize) + bitangent.z * offset
        };
        Vector3 end = {
            planeOrigin.x + tangent.x * halfSize + bitangent.x * offset,
            planeOrigin.y + tangent.y * halfSize + bitangent.y * offset,
            planeOrigin.z + tangent.z * halfSize + bitangent.z * offset
        };

        AddLine(start, end, color);
    }

    // 従接線方向（bitangent）に平行な線を描画
    for (int i = 0; i <= divisions; ++i) {
        float offset = -halfSize + step * i;

        Vector3 start = {
            planeOrigin.x + tangent.x * offset + bitangent.x * (-halfSize),
            planeOrigin.y + tangent.y * offset + bitangent.y * (-halfSize),
            planeOrigin.z + tangent.z * offset + bitangent.z * (-halfSize)
        };
        Vector3 end = {
            planeOrigin.x + tangent.x * offset + bitangent.x * halfSize,
            planeOrigin.y + tangent.y * offset + bitangent.y * halfSize,
            planeOrigin.z + tangent.z * offset + bitangent.z * halfSize
        };

        AddLine(start, end, color);
    }

    // 法線ベクトルを表示（視覚化用、オプショナル）
    Vector3 normalEnd = {
        planeOrigin.x + plane.normal.x * 2.0f,
        planeOrigin.y + plane.normal.y * 2.0f,
        planeOrigin.z + plane.normal.z * 2.0f
    };
    AddLine(planeOrigin, normalEnd, { 1.0f, 1.0f, 0.0f, 1.0f }); // 法線は黄色で表示
}

void Line3d::AddSlope(const Slope& slope, const Vector4& color) {
    Vector3 worldMin = slope.GetMinWorld();
    Vector3 surfaceMin = slope.GetSurfaceMinWorld();
    Vector3 worldMax = slope.GetMaxWorld();

    Vector3 bottom[4] = {
        { worldMin.x, worldMin.y, worldMin.z },
        { worldMax.x, worldMin.y, worldMin.z },
        { worldMax.x, worldMin.y, worldMax.z },
        { worldMin.x, worldMin.y, worldMax.z }
    };

    Vector3 low[2] = {};
    Vector3 high[2] = {};
    int highBottomA = 0;
    int highBottomB = 0;
    int lowBottomA = 0;
    int lowBottomB = 0;

    switch (slope.direction) {
    case SlopeDirection::PulsX:
        low[0] = { worldMin.x, surfaceMin.y, worldMin.z };
        low[1] = { worldMin.x, surfaceMin.y, worldMax.z };
        high[0] = { worldMax.x, worldMax.y, worldMin.z };
        high[1] = { worldMax.x, worldMax.y, worldMax.z };
        highBottomA = 1;
        highBottomB = 2;
        lowBottomA = 0;
        lowBottomB = 3;
        break;
    case SlopeDirection::MinusX:
        low[0] = { worldMax.x, surfaceMin.y, worldMin.z };
        low[1] = { worldMax.x, surfaceMin.y, worldMax.z };
        high[0] = { worldMin.x, worldMax.y, worldMin.z };
        high[1] = { worldMin.x, worldMax.y, worldMax.z };
        highBottomA = 0;
        highBottomB = 3;
        lowBottomA = 1;
        lowBottomB = 2;
        break;
    case SlopeDirection::PulsZ:
        low[0] = { worldMin.x, surfaceMin.y, worldMin.z };
        low[1] = { worldMax.x, surfaceMin.y, worldMin.z };
        high[0] = { worldMin.x, worldMax.y, worldMax.z };
        high[1] = { worldMax.x, worldMax.y, worldMax.z };
        highBottomA = 3;
        highBottomB = 2;
        lowBottomA = 0;
        lowBottomB = 1;
        break;
    case SlopeDirection::MinusZ:
        low[0] = { worldMin.x, surfaceMin.y, worldMax.z };
        low[1] = { worldMax.x, surfaceMin.y, worldMax.z };
        high[0] = { worldMin.x, worldMax.y, worldMin.z };
        high[1] = { worldMax.x, worldMax.y, worldMin.z };
        highBottomA = 0;
        highBottomB = 1;
        lowBottomA = 3;
        lowBottomB = 2;
        break;
    }

    AddLine(bottom[0], bottom[1], color);
    AddLine(bottom[1], bottom[2], color);
    AddLine(bottom[2], bottom[3], color);
    AddLine(bottom[3], bottom[0], color);

    AddLine(bottom[highBottomA], high[0], color);
    AddLine(bottom[highBottomB], high[1], color);
    AddLine(bottom[lowBottomA], low[0], color);
    AddLine(bottom[lowBottomB], low[1], color);
    AddLine(low[0], low[1], color);
    AddLine(high[0], high[1], color);

    AddLine(low[0], high[0], color);
    AddLine(low[1], high[1], color);
}

void Line3d::Draw(Camera& camera) {

    if (vertices_.empty()) return;

    // 頂点バッファの更新が必要な場合は再作成
    if (vertices_.size() * sizeof(LineVertex) > vertexBuffer_->GetDesc().Width) {
        maxVertices_ = vertices_.size() * 2; // 余裕を持たせる
        CreateVertexBuffer();
    }

    // 頂点データを更新
    UpdateVertexBuffer();

    // 変換行列を更新
    Matrix4x4 viewProjectionMatrix = Matrix::Multiply(camera.GetViewMatrix(), camera.GetProjectionMatrix());
    transformData_->viewProjection = viewProjectionMatrix;

    // 描画コマンド
    commandList_->SetGraphicsRootSignature(
        MadoEngine::RootSignatureManager::GetInstance().Get(psoDesc_.rootSigKey));
    commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));

    // プリミティブトポロジーを設定（ラインリスト）
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    // 頂点バッファビューを設定
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // 変換行列を設定
    commandList_->SetGraphicsRootConstantBufferView(0, transformBuffer_->GetGPUVirtualAddress());

    // 描画
    commandList_->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);

    // 描画後に頂点データをクリア
    vertices_.clear();
}

void Line3d::CreateVertexBuffer() {
    Logger::Output("Line3d頂点バッファ作成開始", Logger::Level::Debug);

    // 既存のバッファがあればアンマップ
    if (vertexBuffer_ && vertexData_) {
        vertexBuffer_->Unmap(0, nullptr);
        vertexData_ = nullptr;
    }

    // 頂点バッファのサイズ
    size_t sizeInBytes = maxVertices_ * sizeof(LineVertex);

    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // リソース設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // リソース生成
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_)
    );

    if (FAILED(hr)) {
        Logger::Output("エラー: Line3d頂点バッファの作成に失敗", Logger::Level::Error);
    }
    assert(SUCCEEDED(hr));

    // マップ
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // 頂点バッファビューを作成
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeInBytes);
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    Logger::Output("Line3d頂点バッファ作成完了", Logger::Level::Debug);
}

void Line3d::CreateTransformBuffer() {
    Logger::Output("Line3d変換バッファ作成開始", Logger::Level::Debug);

    // 既存のバッファがあればアンマップ
    if (transformBuffer_ && transformData_) {
        transformBuffer_->Unmap(0, nullptr);
        transformData_ = nullptr;
    }

    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // リソース設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeof(TransformMatrix);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // リソース生成
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&transformBuffer_)
    );

    if (FAILED(hr)) {
        Logger::Output("エラー: Line3d変換バッファの作成に失敗", Logger::Level::Error);
    }
    assert(SUCCEEDED(hr));

    // マップ
    transformBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&transformData_));

    Logger::Output("Line3d変換バッファ作成完了", Logger::Level::Debug);
}

void Line3d::UpdateVertexBuffer() {
    // 頂点データをコピー
    std::memcpy(vertexData_, vertices_.data(), vertices_.size() * sizeof(LineVertex));
}
