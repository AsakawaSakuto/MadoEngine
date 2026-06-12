#include "LayerEffectPassSetup.h"
#include ".Execution/Execution.h"

namespace RenderPassSetup {

	/// @brief アプリケーションで使用するLayerEffectPassを初期化して登録する
	/// @param execution 登録先のExecution
	void RegisterLayerEffectPasses(MadoEngine::Execution& execution) {
		execution.ClearLayerEffectPasses();

		MadoEngine::Render::LayerEffectPass::Desc playerGrayScalePass{};
		playerGrayScalePass.name = "PlayerGrayScale";
		playerGrayScalePass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerGrayScalePass.effectShaderKey = "PostEffect/GrayScale.PS";
		playerGrayScalePass.enabled = true;
		execution.AddLayerEffectPass(playerGrayScalePass);
	}

} // namespace RenderPassSetup