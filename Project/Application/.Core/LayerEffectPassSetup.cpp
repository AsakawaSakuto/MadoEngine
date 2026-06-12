#include "LayerEffectPassSetup.h"
#include ".Execution/Execution.h"
#include <cstddef>

namespace {

	struct VignetteParams {
		float strength = 1.0f;
		float radius = 0.2f;
		float smoothness = 5.0f;
		float padding = 0.0f;
	};

} // namespace

namespace RenderPassSetup {

	/// @brief アプリケーションで使用するLayerEffectPassを初期化して登録する
	/// @param execution 登録先のExecution
	void RegisterLayerEffectPasses(MadoEngine::Execution& execution) {
		execution.ClearLayerEffectPasses();
		execution.ClearScreenEffectPasses();

		MadoEngine::Render::LayerEffectPass::Desc defaultLayerPass{};
		defaultLayerPass.name = "DefaultLayer";
		defaultLayerPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Default);
		defaultLayerPass.effectShaderKey = "PostEffect/CopyImage.PS";
		defaultLayerPass.enabled = false;
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

		MadoEngine::Render::LayerEffectPass::Desc vignettePass{};
		vignettePass.name = "Vignette";
		vignettePass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		vignettePass.effectShaderKey = "PostEffect/Vignette.PS";
		vignettePass.enabled = true;

		MadoEngine::Render::LayerEffectPass* registeredVignettePass = execution.AddScreenEffectPass(vignettePass);
		registeredVignettePass->SetParameterData(VignetteParams{});
		registeredVignettePass->AddFloatParameterControl("Strength", offsetof(VignetteParams, strength), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Radius", offsetof(VignetteParams, radius), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Smoothness", offsetof(VignetteParams, smoothness), 1.0f, 100.0f, 0.01f);
	}

} // namespace RenderPassSetup