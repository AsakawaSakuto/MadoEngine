#pragma once
#include <memory>
#include "Line.h"
#include "Utility/Collider/Shape.h"
#include "Math/Vector4.h"
#include "Utility/Camera/Camera.h"

/// <summary>
/// Singleton パターンで Line3d を管理するクラス
/// デバッグ描画ラインの一元管理とAPI統一を提供
/// </summary>
class DebugLineManager {
public:
    /// @brief Singleton インスタンスの取得（ポインタ）
    /// @return DebugLineManager のシングルトンインスタンス
    static DebugLineManager* GetInstance();

    // コピー・ムーブ禁止
    DebugLineManager(const DebugLineManager&) = delete;
    DebugLineManager& operator=(const DebugLineManager&) = delete;
    DebugLineManager(DebugLineManager&&) = delete;
    DebugLineManager& operator=(DebugLineManager&&) = delete;

    /// @brief 明示的な初期化（オプション）
    /// ※初回の Add または Draw 時に自動で遅延初期化されるため、呼ばなくても可
    /// @param device D3D12デバイス
    /// @param commandList コマンドリスト
    /// @param maxVertices 最大頂点数
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, uint32_t maxVertices);

    /// @brief 形状を追加（統一API）
    /// @param shape 描画する形状
    /// @param color 描画色（デフォルト：白）
    void AddShape(const Shape& shape, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief グリッド描画
    /// @param size グリッドサイズ
    /// @param divisions 分割数
    /// @param color 描画色（デフォルト：白）
    void AddGrid(float size, int divisions, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief すべての線を描画
    /// @param camera 使用するカメラ
	void Draw(Camera& camera);

	/// @brief PSORegistryをセット
	void SetPSORegistry(MadoEngine::Render::PSORegistry* psoRegistry);
private:
    DebugLineManager() = default;
    ~DebugLineManager() = default;

private:
    bool initialized_ = false;
    bool isDrawing_ = true;
    std::unique_ptr<Line3d> line_;
    MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
};