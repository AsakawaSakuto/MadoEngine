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

	struct BloomParams {
		float intensity = 0.6f;
		float threshold = 0.7f;
		float radius = 4.0f;
		float softKnee = 0.5f;
	};

	struct PixelArtParams {
		float pixelSize = 6.0f;
		float colorSteps = 8.0f;
		float contrast = 1.15f;
		float intensity = 1.0f;
	};

	struct OutlineParams {
		float outlineColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float outlineParams[4] = { 1.0f, 80.0f, 0.005f, 1.0f };
	};

} // namespace

namespace RenderPassSetup {

	/// @brief アプリケーションで使用するLayerEffectPassを初期化して登録する
	/// @param execution 登録先のExecution
	void RegisterLayerEffectPasses(MadoEngine::Execution& execution) {
		execution.ClearLayerEffectPasses();
		execution.ClearScreenEffectPasses();

		auto setupOutlineParameters = [](MadoEngine::Render::LayerEffectPass* pass) {
			pass->SetParameterData(OutlineParams{});
			pass->AddFloatParameterControl("Color R", offsetof(OutlineParams, outlineColor) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("Color G", offsetof(OutlineParams, outlineColor) + sizeof(float) * 1, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("Color B", offsetof(OutlineParams, outlineColor) + sizeof(float) * 2, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("Thickness", offsetof(OutlineParams, outlineParams) + sizeof(float) * 0, 0.25f, 8.0f, 0.05f);
			pass->AddFloatParameterControl("DepthSensitivity", offsetof(OutlineParams, outlineParams) + sizeof(float) * 1, 1.0f, 300.0f, 1.0f);
			pass->AddFloatParameterControl("Threshold", offsetof(OutlineParams, outlineParams) + sizeof(float) * 2, 0.0001f, 0.1f, 0.001f);
			pass->AddFloatParameterControl("Intensity", offsetof(OutlineParams, outlineParams) + sizeof(float) * 3, 0.0f, 3.0f, 0.01f);
		};

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

		MadoEngine::Render::LayerEffectPass::Desc playerOutlinePass{};
		playerOutlinePass.name = "PlayerOutline";
		playerOutlinePass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerOutlinePass.effectShaderKey = "PostEffect/Outline.PS";
		playerOutlinePass.enabled = false;
		playerOutlinePass.ignoreDepthForMask = true;
		setupOutlineParameters(execution.AddLayerEffectPass(playerOutlinePass));

		MadoEngine::Render::LayerEffectPass::Desc bloomPass{};
		bloomPass.name = "Bloom";
		bloomPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		bloomPass.effectShaderKey = "PostEffect/Bloom.PS";
		bloomPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredBloomPass = execution.AddScreenEffectPass(bloomPass);
		registeredBloomPass->SetParameterData(BloomParams{});
		registeredBloomPass->AddFloatParameterControl("Intensity", offsetof(BloomParams, intensity), 0.0f, 5.0f, 0.01f);
		registeredBloomPass->AddFloatParameterControl("Threshold", offsetof(BloomParams, threshold), 0.0f, 1.0f, 0.01f);
		registeredBloomPass->AddFloatParameterControl("Radius", offsetof(BloomParams, radius), 0.0f, 32.0f, 0.1f);
		registeredBloomPass->AddFloatParameterControl("SoftKnee", offsetof(BloomParams, softKnee), 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc vignettePass{};
		vignettePass.name = "Vignette";
		vignettePass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		vignettePass.effectShaderKey = "PostEffect/Vignette.PS";
		vignettePass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredVignettePass = execution.AddScreenEffectPass(vignettePass);
		registeredVignettePass->SetParameterData(VignetteParams{});
		registeredVignettePass->AddFloatParameterControl("Strength", offsetof(VignetteParams, strength), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Radius", offsetof(VignetteParams, radius), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Smoothness", offsetof(VignetteParams, smoothness), 1.0f, 100.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc pixelArtPass{};
		pixelArtPass.name = "PixelArt";
		pixelArtPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		pixelArtPass.effectShaderKey = "PostEffect/PixelArt.PS";
		pixelArtPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredPixelArtPass = execution.AddScreenEffectPass(pixelArtPass);
		registeredPixelArtPass->SetParameterData(PixelArtParams{});
		registeredPixelArtPass->AddFloatParameterControl("PixelSize", offsetof(PixelArtParams, pixelSize), 1.0f, 128.0f, 1.0f);
		registeredPixelArtPass->AddFloatParameterControl("ColorSteps", offsetof(PixelArtParams, colorSteps), 2.0f, 128.0f, 1.0f);
		registeredPixelArtPass->AddFloatParameterControl("Contrast", offsetof(PixelArtParams, contrast), 0.0f, 3.0f, 0.01f);
		registeredPixelArtPass->AddFloatParameterControl("Intensity", offsetof(PixelArtParams, intensity), 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc screenOutlinePass{};
		screenOutlinePass.name = "ScreenOutline";
		screenOutlinePass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		screenOutlinePass.effectShaderKey = "PostEffect/Outline.PS";
		screenOutlinePass.enabled = false;
		setupOutlineParameters(execution.AddScreenEffectPass(screenOutlinePass));
	}

} // namespace RenderPassSetup
