#pragma once
#include <string>
#include <dxgi1_6.h>

namespace MadoEngine::Render {

	/// @brief ブレンドモード
	enum class BlendMode {
		Normal,
		Add,
		Subtract,
		Multiply,
		None,
	};

	/// @brief 深度ステンシルモード
	enum class DepthMode {
		ReadWrite,
		ReadOnly,
		Disable,
	};

	/// @brief カリングモード
	enum class CullMode {
		None,
		Front,
		Back,
	};

	/// @brief フィルモード
	enum class FillMode {
		Solid,
		Wireframe,
	};

	/// @brief トポロジータイプ
	enum class TopologyType {
		Triangle,
		Line,
		Point,
	};

	/// @brief 入力レイアウトタイプ
	enum class InputLayoutType {
		None,
		Triangle,
		Sprite,
		StaticModel,
		SkiningModel,
		Line,
	};

	/// @brief PSOを一意に識別するための記述子
	struct PSODesc {
		BlendMode       blendMode   = BlendMode::Normal;
		DepthMode       depthMode   = DepthMode::ReadWrite;
		CullMode        cullMode    = CullMode::Back;
		FillMode        fillMode    = FillMode::Solid;
		TopologyType    topology    = TopologyType::Triangle;
		InputLayoutType inputLayout = InputLayoutType::StaticModel;
		DXGI_FORMAT     rtvFormat   = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		DXGI_FORMAT     dsvFormat   = DXGI_FORMAT_D24_UNORM_S8_UINT;
		std::string     vsKey;
		std::string     psKey;
		std::string     rootSigKey;
	};

} // namespace MadoEngine::Render
