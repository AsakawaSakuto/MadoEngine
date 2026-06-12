#include "LayerEffectPassSetup.h"
#include ".Execution/Execution.h"

namespace RenderPassSetup {

	/// @brief アプリケーションで使用するLayerEffectPassを初期化して登録する
	/// @param execution 登録先のExecution
	void RegisterLayerEffectPasses(MadoEngine::Execution& execution) {
		execution.ClearLayerEffectPasses();

		MadoEngine::Render::LayerEffectPass::Desc defaultLayerPass{};
		defaultLayerPass.name = "DefaultLayer";
		defaultLayerPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Default);
		defaultLayerPass.effectShaderKey = "PostEffect/CopyImage.PS";
		defaultLayerPass.enabled = true;
		execution.AddLayerEffectPass(defaultLayerPass);

		MadoEngine::Render::LayerEffectPass::Desc playerLayerPass{};
		playerLayerPass.name = "PlayerLayer";
		playerLayerPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerLayerPass.effectShaderKey = "PostEffect/GrayScale.PS";
		playerLayerPass.enabled = true;
		execution.AddLayerEffectPass(playerLayerPass);

		MadoEngine::Render::LayerEffectPass::Desc playerSepiaPass{};
		playerSepiaPass.name = "PlayerSepia";
		playerSepiaPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerSepiaPass.effectShaderKey = "PostEffect/Sepia.PS";
		playerSepiaPass.enabled = true;
		execution.AddLayerEffectPass(playerSepiaPass);
	}

} // namespace RenderPassSetup
