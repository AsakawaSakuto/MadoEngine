#include "Cylinder.h"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void ThrowIfFailed(const HRESULT hr, const char* message) {
    if (FAILED(hr)) {
        throw std::runtime_error(std::string(message) + " (HRESULT: " + std::to_string(static_cast<long>(hr)) + ")");
    }
}

} // namespace

void Cylinder::Initialize(ID3D12Device* device, const Desc& desc) {
    if (device == nullptr) {
        throw std::invalid_argument("Cylinder::Initialize: device must not be nullptr.");
    }

    ValidateDesc(desc);

    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<size_t>(desc.divide) * 6);

    const float radianPerDivide = DirectX::XM_2PI / static_cast<float>(desc.divide);

    // 円すい台にも対応した側面法線です。
    // 半径が上ほど大きいとき、法線はわずかに下向きへ傾きます。
    const float radialSlope = (desc.topRadius - desc.bottomRadius) / desc.height;

    for (std::uint32_t index = 0; index < desc.divide; ++index) {
        const float radian = static_cast<float>(index) * radianPerDivide;
        const float nextRadian = static_cast<float>(index + 1) * radianPerDivide;

        const float sinValue = std::sin(radian);
        const float cosValue = std::cos(radian);
        const float sinNext = std::sin(nextRadian);
        const float cosNext = std::cos(nextRadian);

        const float u = static_cast<float>(index) / static_cast<float>(desc.divide);
        const float uNext = static_cast<float>(index + 1) / static_cast<float>(desc.divide);

        // スライドと同じく、円周は -sin(x), cos(z) で生成します。
        // normalY を含めることで topRadius と bottomRadius が異なる場合にも正しい法線になります。
        const DirectX::XMFLOAT3 normal = {
            -sinValue,
            -radialSlope,
            cosValue,
        };
        const DirectX::XMFLOAT3 normalNext = {
            -sinNext,
            -radialSlope,
            cosNext,
        };

        // 1 分割あたり 2 三角形、合計 6 頂点。
        // v は上面側を 0、下面側を 1 としています。
        // Cylinder.PS.hlsl 側で v = 1 - v に反転します。
        vertices.push_back({
            {-sinValue * desc.topRadius, desc.height, cosValue * desc.topRadius, 1.0f},
            {u, 0.0f},
            normal,
        });
        vertices.push_back({
            {-sinNext * desc.topRadius, desc.height, cosNext * desc.topRadius, 1.0f},
            {uNext, 0.0f},
            normalNext,
        });
        vertices.push_back({
            {-sinValue * desc.bottomRadius, 0.0f, cosValue * desc.bottomRadius, 1.0f},
            {u, 1.0f},
            normal,
        });

        vertices.push_back({
            {-sinNext * desc.topRadius, desc.height, cosNext * desc.topRadius, 1.0f},
            {uNext, 0.0f},
            normalNext,
        });
        vertices.push_back({
            {-sinNext * desc.bottomRadius, 0.0f, cosNext * desc.bottomRadius, 1.0f},
            {uNext, 1.0f},
            normalNext,
        });
        vertices.push_back({
            {-sinValue * desc.bottomRadius, 0.0f, cosValue * desc.bottomRadius, 1.0f},
            {u, 1.0f},
            normal,
        });
    }

    const UINT64 bufferSize = static_cast<UINT64>(sizeof(Vertex)) * vertices.size();

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = bufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    vertexBuffer_.Reset();
    ThrowIfFailed(
        device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer_)),
        "Cylinder::Initialize: failed to create vertex buffer.");

    void* mappedData = nullptr;
    const D3D12_RANGE readRange{0, 0};
    ThrowIfFailed(
        vertexBuffer_->Map(0, &readRange, &mappedData),
        "Cylinder::Initialize: failed to map vertex buffer.");

    std::memcpy(mappedData, vertices.data(), static_cast<size_t>(bufferSize));
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView_.StrideInBytes = sizeof(Vertex);

    desc_ = desc;
    vertexCount_ = static_cast<std::uint32_t>(vertices.size());
}

void Cylinder::Draw(ID3D12GraphicsCommandList* commandList) const {
    if (commandList == nullptr) {
        throw std::invalid_argument("Cylinder::Draw: commandList must not be nullptr.");
    }
    if (vertexBuffer_ == nullptr || vertexCount_ == 0) {
        throw std::logic_error("Cylinder::Draw: Initialize must be called before Draw.");
    }

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->DrawInstanced(vertexCount_, 1, 0, 0);
}

const D3D12_VERTEX_BUFFER_VIEW& Cylinder::GetVertexBufferView() const noexcept {
    return vertexBufferView_;
}

std::uint32_t Cylinder::GetVertexCount() const noexcept {
    return vertexCount_;
}

const Cylinder::Desc& Cylinder::GetDesc() const noexcept {
    return desc_;
}

D3D12_RASTERIZER_DESC Cylinder::CreateEffectRasterizerDesc() noexcept {
    D3D12_RASTERIZER_DESC desc{};
    desc.FillMode = D3D12_FILL_MODE_SOLID;
    desc.CullMode = D3D12_CULL_MODE_NONE;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    desc.DepthClipEnable = TRUE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;
    desc.ForcedSampleCount = 0;
    desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return desc;
}

D3D12_DEPTH_STENCIL_DESC Cylinder::CreateEffectDepthStencilDesc() noexcept {
    D3D12_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.BackFace = desc.FrontFace;
    return desc;
}

D3D12_BLEND_DESC Cylinder::CreateAlphaBlendDesc() noexcept {
    D3D12_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC& renderTarget = desc.RenderTarget[0];
    renderTarget.BlendEnable = TRUE;
    renderTarget.LogicOpEnable = FALSE;
    renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
    renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
    renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
    renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    renderTarget.LogicOp = D3D12_LOGIC_OP_NOOP;
    renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return desc;
}

void Cylinder::ValidateDesc(const Desc& desc) {
    if (desc.divide < 3) {
        throw std::invalid_argument("Cylinder::Desc::divide must be 3 or greater.");
    }
    if (!std::isfinite(desc.topRadius) || !std::isfinite(desc.bottomRadius) || !std::isfinite(desc.height)) {
        throw std::invalid_argument("Cylinder::Desc contains a non-finite value.");
    }
    if (desc.topRadius < 0.0f || desc.bottomRadius < 0.0f) {
        throw std::invalid_argument("Cylinder radii must be non-negative.");
    }
    if (desc.topRadius == 0.0f && desc.bottomRadius == 0.0f) {
        throw std::invalid_argument("At least one cylinder radius must be greater than zero.");
    }
    if (desc.height <= 0.0f) {
        throw std::invalid_argument("Cylinder height must be greater than zero.");
    }
}
