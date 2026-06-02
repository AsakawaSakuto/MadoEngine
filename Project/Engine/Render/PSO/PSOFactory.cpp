#include "Render/PSO/PSOFactory.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"
#include "Shader/ShaderManager.h"
#include "Shader/RootSignatureManager.h"
#include <cassert>

namespace MadoEngine::Render {

	// ---------------------------------------------------------------
	// 静的な入力レイアウト定義
	// ---------------------------------------------------------------
	static const D3D12_INPUT_ELEMENT_DESC kTriangleLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D12_INPUT_ELEMENT_DESC kSpriteLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D12_INPUT_ELEMENT_DESC kStaticModelLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D12_INPUT_ELEMENT_DESC kSkinModelLayout[] = {
		{ "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_INDEX",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D12_INPUT_ELEMENT_DESC kLineLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// ---------------------------------------------------------------

	void PSOFactory::Initialize(Core::DxDevice* device) {
		assert(device);
		device_ = device;
		Logger::Output("[PSOFactory] 初期化完了", Logger::Level::Engine);
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSOFactory::Build(const PSODesc& desc) const {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

		// 入力レイアウト
		const D3D12_INPUT_ELEMENT_DESC* elements = nullptr;
		UINT elementCount = 0;
		SelectInputLayout(desc.inputLayout, &elements, &elementCount);
		psoDesc.InputLayout = { elements, elementCount };

		// ブレンド
		psoDesc.BlendState = BuildBlendDesc(desc.blendMode);

		// 深度ステンシル
		psoDesc.DepthStencilState = BuildDepthStencilDesc(desc.depthMode);

		// ラスタライザー
		psoDesc.RasterizerState = BuildRasterizerDesc(desc.cullMode, desc.fillMode);

		// RTV / DSV フォーマット
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0]    = desc.rtvFormat;
		psoDesc.DSVFormat        = desc.dsvFormat;

		// トポロジー
		switch (desc.topology) {
			case TopologyType::Triangle:
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				break;
			case TopologyType::Line:
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				break;
			case TopologyType::Point:
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				break;
		}

		psoDesc.SampleMask       = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.SampleDesc.Count = 1;

		// TODO: ShaderManagerから取得する
		psoDesc.VS             = MadoEngine::ShaderManager::GetInstance().Get(desc.vsKey);
		psoDesc.PS             = MadoEngine::ShaderManager::GetInstance().Get(desc.psKey);
		psoDesc.pRootSignature = MadoEngine::RootSignatureManager::GetInstance().Get(desc.rootSigKey);

		return psoDesc;
	}

	void PSOFactory::SelectInputLayout(
		InputLayoutType type,
		const D3D12_INPUT_ELEMENT_DESC** outElements,
		UINT* outCount)
	{
		switch (type) {
		    case InputLayoutType::Triangle:
			    *outElements = kTriangleLayout;
			    *outCount = _countof(kTriangleLayout);
			    break;
			case InputLayoutType::Sprite:
				*outElements = kSpriteLayout;
				*outCount    = _countof(kSpriteLayout);
				break;
			case InputLayoutType::StaticModel:
				*outElements = kStaticModelLayout;
				*outCount    = _countof(kStaticModelLayout);
				break;
			case InputLayoutType::SkiningModel:
				*outElements = kSkinModelLayout;
				*outCount    = _countof(kSkinModelLayout);
				break;
			case InputLayoutType::Line:
				*outElements = kLineLayout;
				*outCount    = _countof(kLineLayout);
				break;
			default:
				*outElements = kStaticModelLayout;
				*outCount    = _countof(kStaticModelLayout);
				break;
		}
	}

	D3D12_BLEND_DESC PSOFactory::BuildBlendDesc(BlendMode mode) {
		D3D12_BLEND_DESC desc{};
		desc.AlphaToCoverageEnable  = FALSE;
		desc.IndependentBlendEnable = FALSE;

		auto& rt = desc.RenderTarget[0];
		rt.BlendEnable           = TRUE;
		rt.LogicOpEnable         = FALSE;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
		rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
		rt.LogicOp               = D3D12_LOGIC_OP_NOOP;

		switch (mode) {
			case BlendMode::Normal:
				rt.SrcBlend  = D3D12_BLEND_SRC_ALPHA;
				rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				rt.BlendOp   = D3D12_BLEND_OP_ADD;
				break;
			case BlendMode::Add:
				rt.SrcBlend  = D3D12_BLEND_SRC_ALPHA;
				rt.DestBlend = D3D12_BLEND_ONE;
				rt.BlendOp   = D3D12_BLEND_OP_ADD;
				break;
			case BlendMode::Subtract:
				rt.SrcBlend  = D3D12_BLEND_SRC_ALPHA;
				rt.DestBlend = D3D12_BLEND_ONE;
				rt.BlendOp   = D3D12_BLEND_OP_REV_SUBTRACT;
				break;
			case BlendMode::Multiply:
				rt.SrcBlend  = D3D12_BLEND_ZERO;
				rt.DestBlend = D3D12_BLEND_SRC_COLOR;
				rt.BlendOp   = D3D12_BLEND_OP_ADD;
				break;
			case BlendMode::None:
				rt.BlendEnable = FALSE;
				rt.SrcBlend    = D3D12_BLEND_ONE;
				rt.DestBlend   = D3D12_BLEND_ZERO;
				rt.BlendOp     = D3D12_BLEND_OP_ADD;
				break;
		}

		return desc;
	}

	D3D12_DEPTH_STENCIL_DESC PSOFactory::BuildDepthStencilDesc(DepthMode mode) {
		D3D12_DEPTH_STENCIL_DESC desc{};
		desc.StencilEnable    = FALSE;
		desc.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		const D3D12_DEPTH_STENCILOP_DESC defaultOp = {
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_COMPARISON_FUNC_ALWAYS
		};
		desc.FrontFace = defaultOp;
		desc.BackFace  = defaultOp;

		switch (mode) {
			case DepthMode::ReadWrite:
				desc.DepthEnable    = TRUE;
				desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				desc.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
				break;
			case DepthMode::ReadOnly:
				desc.DepthEnable    = TRUE;
				desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
				break;
			case DepthMode::Disable:
				desc.DepthEnable    = FALSE;
				desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc      = D3D12_COMPARISON_FUNC_ALWAYS;
				break;
		}

		return desc;
	}

	D3D12_RASTERIZER_DESC PSOFactory::BuildRasterizerDesc(CullMode cull, FillMode fill) {
		D3D12_RASTERIZER_DESC desc{};
		desc.DepthClipEnable          = TRUE;
		desc.MultisampleEnable        = FALSE;
		desc.AntialiasedLineEnable    = FALSE;
		desc.ForcedSampleCount        = 0;
		desc.ConservativeRaster       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.FrontCounterClockwise    = FALSE;

		switch (cull) {
			case CullMode::None:  desc.CullMode = D3D12_CULL_MODE_NONE;  break;
			case CullMode::Front: desc.CullMode = D3D12_CULL_MODE_FRONT; break;
			case CullMode::Back:  desc.CullMode = D3D12_CULL_MODE_BACK;  break;
		}

		switch (fill) {
			case FillMode::Solid:     desc.FillMode = D3D12_FILL_MODE_SOLID;     break;
			case FillMode::Wireframe: desc.FillMode = D3D12_FILL_MODE_WIREFRAME; break;
		}

		return desc;
	}

} // namespace MadoEngine::Render
