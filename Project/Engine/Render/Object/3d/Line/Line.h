#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "Math/Vector3.h"
#include "Math/Function/MatrixFunction.h"
#include "Math/Transform.h"
#include "Utility/Camera/Camera.h"
#include "Utility/Collider/Shape/AABB.h"
#include "Utility/Collider/Shape/OBB.h"
#include "Utility/Collider/Shape/Sphere.h"
#include "Utility/Collider/Shape/OvalSphere.h"
#include "Utility/Collider/Shape/Plane.h"
#include "Utility/Collider/Shape/Segment.h"
#include "Utility/Collider/Shape/Line.h"
#include "Utility/Collider/Shape/Circle.h"
#include "Utility/Collider/Shape/Slope.h"
#include "Render/PSO/PSORegistry.h"
#include "Render/PSO/PSODesc.h"

/// <summary>
/// 3D空間に線を描画するクラス（デバッグ表示、UI、可視化用）
/// </summary>
class Line3d {
public:
    /// <summary>
    /// 線の頂点データ
    /// </summary>
    struct LineVertex {
        Vector3 position; // 位置
        Vector4 color;    // 色
    };

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param commandList コマンドリスト
    /// @param maxVertices 最大頂点数
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, uint32_t maxVertices);

    /// @brief PSORegistryをセットする
    /// @param psoRegistry PSORegistryポインタ
    void SetPSORegistry(MadoEngine::Render::PSORegistry* psoRegistry);

    /// @brief デストラクタ
    ~Line3d();

    /// @brief 線を追加（2点指定）
    /// @param start 開始点
    /// @param end 終了点
    /// @param color 色（デフォルト：白）
    void AddLine(const Vector3& start, const Vector3& end, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 複数の線を一度に追加
    /// @param points 頂点配列
    /// @param color 色（デフォルト：白）
    void AddLines(const std::vector<Vector3>& points, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief グリッド描画
    /// @param size グリッドサイズ
    /// @param divisions 分割数
    /// @param color 色（デフォルト：白）
    void AddGrid(float size = 10.0f, int divisions = 10, const Vector4& color = { 0.25f, 0.25f, 0.25f, 1.0f });

    /// @brief ボックス（中心とサイズで指定）描画
    /// @param center 中心座標
    /// @param size サイズ
    /// @param color 色（デフォルト：白）
    void AddBox(const Vector3& center, const Vector3& size, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief ボックス（AABB）描画
    /// @param aabb AABB構造体
    /// @param color 色（デフォルト：白）
    void AddBox(const AABB& aabb, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief ボックス（OBB）描画
    /// @param obb OBB構造体
    /// @param color 色（デフォルト：白）
    void AddBox(const OBB& obb, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 球体（ワイヤーフレーム）描画
    /// @param sphere 球体構造体
    /// @param color 色（デフォルト：白）
    void AddSphere(const Sphere& sphere, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 楕円球体（ワイヤーフレーム）描画
    /// @param ovalSphere 楕円球体構造体
    /// @param color 色（デフォルト：白）
    void AddOvalSphere(const OvalSphere& ovalSphere, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 点を追加
    /// @param position 位置
    /// @param color 色（デフォルト：白）
    void AddPoint(const Vector3& position, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 円描画
    /// @param circle 円構造体
    /// @param color 色（デフォルト：白）
    void AddCircle(const Circle& circle, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief レイ（半直線）描画
    /// @param origin 始点
    /// @param direction 方向
    /// @param length 長さ
    /// @param color 色（デフォルト：白）
    void AddRay(const Vector3& origin, const Vector3& direction, float length, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 線分（Segment）描画
    /// @param segment 線分構造体
    /// @param color 色（デフォルト：白）
    void AddSegment(const Segment& segment, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 直線（Line）描画
    /// @param line 直線構造体
    /// @param color 色（デフォルト：白）
    void AddLine(const Line& line, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 平面（Plane）描画（グリッド表示）
    /// @param plane 平面構造体
    /// @param divisions グリッド分割数（デフォルト：10）
    /// @param color 色（デフォルト：白）
    void AddPlane(const Plane& plane, int divisions = 10, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 坂道をワイヤーフレームで描画する
    /// @param slope 坂道形状
    /// @param color 描画色
    void AddSlope(const Slope& slope, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief 描画
    /// @param camera 使用するカメラ
    void Draw(Camera& camera);

private:

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

    MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
    MadoEngine::Render::PSODesc psoDesc_{};

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // GPU転送用データ
    LineVertex* vertexData_ = nullptr;
    struct TransformMatrix {
        Matrix4x4 viewProjection;
    };
    TransformMatrix* transformData_ = nullptr;

    // 線データ
    std::vector<LineVertex> vertices_;
    size_t maxVertices_ = 100;

    // リソース作成関数
    void CreateVertexBuffer();
    void CreateTransformBuffer();

    // リソース更新関数
    void UpdateVertexBuffer();

    // 円の分割数
    int segments_ = 16;

    // 球体描画用の分割数
    const int latitudeDivisions_ = 8;   // 緯度の分割数（水平方向の円の数）
    const int longitudeDivisions_ = 16; // 経度の分割数（垂直方向の線の数）
};
