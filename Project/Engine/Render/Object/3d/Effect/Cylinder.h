#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <cstdint>

// 上面・下面を持たない、Effect 用の円柱（側面だけ）です。
// 頂点は TriangleList として展開するため、IndexBuffer は使用しません。
class Cylinder final {
public:
    struct Vertex {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT2 texcoord;
        DirectX::XMFLOAT3 normal;
    };

    struct Desc {
        std::uint32_t divide = 32;  // 円周の分割数。3 以上。
        float topRadius = 1.0f;
        float bottomRadius = 1.0f;
        float height = 3.0f;
    };

public:
    Cylinder() = default;
    ~Cylinder() = default;

    Cylinder(const Cylinder&) = delete;
    Cylinder& operator=(const Cylinder&) = delete;
    Cylinder(Cylinder&&) noexcept = default;
    Cylinder& operator=(Cylinder&&) noexcept = default;

    // 静的な Effect メッシュとしては十分小さいため、実装は簡単な Upload Heap を使用します。
    // 大量のメッシュを保持する場合は Default Heap へのアップロードに置き換えてください。
    void Initialize(ID3D12Device* device, const Desc& desc = {});

    // 呼び出す前に、RootSignature / PipelineState / CBV / Texture SRV を設定してください。
    void Draw(ID3D12GraphicsCommandList* commandList) const;

    [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const noexcept;
    [[nodiscard]] std::uint32_t GetVertexCount() const noexcept;
    [[nodiscard]] const Desc& GetDesc() const noexcept;

    // Cylinder を半透明 Effect として描画するための代表的な PSO 設定です。
    // 既存の PSO 作成処理に必要なものだけ取り込んでください。
    [[nodiscard]] static D3D12_RASTERIZER_DESC CreateEffectRasterizerDesc() noexcept;
    [[nodiscard]] static D3D12_DEPTH_STENCIL_DESC CreateEffectDepthStencilDesc() noexcept;
    [[nodiscard]] static D3D12_BLEND_DESC CreateAlphaBlendDesc() noexcept;

private:
    static void ValidateDesc(const Desc& desc);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Desc desc_{};
    std::uint32_t vertexCount_ = 0;
};
