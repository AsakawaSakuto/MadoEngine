#pragma once
#include "Render/PSO/PSODesc.h"
#include <d3d12.h>

namespace MadoEngine::Core {
	class DxDevice;
}

namespace MadoEngine::Render {

	/// @brief PSODescからD3D12_GRAPHICS_PIPELINE_STATE_DESCを組み立てるファクトリクラス
	class PSOFactory {
	public:
		/// @brief 初期化
		/// @param device DxDeviceポインタ
		void Initialize(Core::DxDevice* device);

		/// @brief PSODescをもとにD3D12_GRAPHICS_PIPELINE_STATE_DESCを構築する
		/// @param desc PSO記述子
		/// @return 組み立て済みのD3D12_GRAPHICS_PIPELINE_STATE_DESC
		D3D12_GRAPHICS_PIPELINE_STATE_DESC Build(const PSODesc& desc) const;

	private:
		Core::DxDevice* device_ = nullptr;

		/// @brief InputLayoutTypeに応じたD3D12_INPUT_ELEMENT_DESCを選択する
		/// @param type 入力レイアウトタイプ
		/// @param outElements 出力要素配列へのポインタ
		/// @param outCount 出力要素数
		static void SelectInputLayout(
			InputLayoutType type,
			const D3D12_INPUT_ELEMENT_DESC** outElements,
			UINT* outCount
		);

		/// @brief BlendModeに応じたD3D12_BLEND_DESCを組み立てる
		/// @param mode ブレンドモード
		/// @return 組み立て済みのD3D12_BLEND_DESC
		static D3D12_BLEND_DESC BuildBlendDesc(BlendMode mode);

		/// @brief DepthModeに応じたD3D12_DEPTH_STENCIL_DESCを組み立てる
		/// @param mode 深度モード
		/// @return 組み立て済みのD3D12_DEPTH_STENCIL_DESC
		static D3D12_DEPTH_STENCIL_DESC BuildDepthStencilDesc(DepthMode mode);

		/// @brief CullMode/FillModeに応じたD3D12_RASTERIZER_DESCを組み立てる
		/// @param cull カリングモード
		/// @param fill フィルモード
		/// @return 組み立て済みのD3D12_RASTERIZER_DESC
		static D3D12_RASTERIZER_DESC BuildRasterizerDesc(
			CullMode cull,
			FillMode fill,
			int depthBias,
			float depthBiasClamp,
			float slopeScaledDepthBias
		);
	};

} // namespace MadoEngine::Render
