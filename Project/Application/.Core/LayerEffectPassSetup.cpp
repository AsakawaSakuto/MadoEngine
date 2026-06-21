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
		float pixelSize = 3.0f;
		float colorSteps = 8.0f;
		float contrast = 1.0f;
		float intensity = 1.0f;
	};

	struct ToonParams {
		float colorParams[4] = { 4.0f, 1.15f, 1.1f, 1.0f };
		float edgeParams[4] = { 1.25f, 80.0f, 0.005f, 1.0f };
		float outlineColor[4] = { 0.02f, 0.025f, 0.03f, 1.0f };
	};

	struct OutlineParams {
		float outlineColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
		float outlineParams[4] = { 1.5f, 80.0f, 0.005f, 1.0f };
	};

	struct DissolveParams {
		float dissolveParams[4] = { 0.35f, 0.06f, 1.0f, 2.0f };
		float edgeColor[4] = { 1.0f, 0.45f, 0.05f, 1.0f };
	};

	struct FogParams {
		float fogColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float fogDistanceParams[4] = { 35.0f, 200.0f, 0.95f, 0.0f };
		float fogCameraParams[4] = { 0.1f, 1000.0f, 0.0f, 0.0f };
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
			pass->AddFloatParameterControl("OutlineColorR", "輪郭色 R", offsetof(OutlineParams, outlineColor) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("OutlineColorG", "輪郭色 G", offsetof(OutlineParams, outlineColor) + sizeof(float) * 1, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("OutlineColorB", "輪郭色 B", offsetof(OutlineParams, outlineColor) + sizeof(float) * 2, 0.0f, 1.0f, 0.01f);
			pass->AddFloatParameterControl("Thickness", "輪郭の太さ", offsetof(OutlineParams, outlineParams) + sizeof(float) * 0, 0.25f, 8.0f, 0.05f);
			pass->AddFloatParameterControl("DepthSensitivity", "深度感度", offsetof(OutlineParams, outlineParams) + sizeof(float) * 1, 1.0f, 300.0f, 1.0f);
			pass->AddFloatParameterControl("Threshold", "輪郭しきい値", offsetof(OutlineParams, outlineParams) + sizeof(float) * 2, 0.0001f, 0.1f, 0.001f);
			pass->AddFloatParameterControl("Intensity", "輪郭の強さ", offsetof(OutlineParams, outlineParams) + sizeof(float) * 3, 0.0f, 3.0f, 0.01f);
		};

		MadoEngine::Render::LayerEffectPass::Desc defaultLayerPass{};
		defaultLayerPass.key = "DefaultLayer";
		defaultLayerPass.name = "デフォルトレイヤー";
		defaultLayerPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Default);
		defaultLayerPass.effectShaderKey = "PostEffect/CopyImage.PS";
		defaultLayerPass.enabled = false;
		execution.AddLayerEffectPass(defaultLayerPass);

		MadoEngine::Render::LayerEffectPass::Desc playerLayerPass{};
		playerLayerPass.key = "PlayerGrayScale";
		playerLayerPass.name = "プレイヤー白黒";
		playerLayerPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerLayerPass.effectShaderKey = "PostEffect/GrayScale.PS";
		playerLayerPass.enabled = false;
		execution.AddLayerEffectPass(playerLayerPass);

		MadoEngine::Render::LayerEffectPass::Desc playerSepiaPass{};
		playerSepiaPass.key = "PlayerSepia";
		playerSepiaPass.name = "プレイヤーセピア";
		playerSepiaPass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerSepiaPass.effectShaderKey = "PostEffect/Sepia.PS";
		playerSepiaPass.enabled = false;
		execution.AddLayerEffectPass(playerSepiaPass);

		MadoEngine::Render::LayerEffectPass::Desc playerOutlinePass{};
		playerOutlinePass.key = "PlayerOutline";
		playerOutlinePass.name = "プレイヤー輪郭";
		playerOutlinePass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerOutlinePass.effectShaderKey = "PostEffect/Outline.PS";
		playerOutlinePass.enabled = true;
		playerOutlinePass.ignoreDepthForMask = true;
		setupOutlineParameters(execution.AddLayerEffectPass(playerOutlinePass));

		MadoEngine::Render::LayerEffectPass::Desc mapEventObjOutlinePass{};
		mapEventObjOutlinePass.key = "MapEventObjectOutline";
		mapEventObjOutlinePass.name = "マップイベントオブジェクト輪郭";
		mapEventObjOutlinePass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::MapEventObject);
		mapEventObjOutlinePass.effectShaderKey = "PostEffect/Outline.PS";
		mapEventObjOutlinePass.enabled = true;
		mapEventObjOutlinePass.ignoreDepthForMask = false;
		setupOutlineParameters(execution.AddLayerEffectPass(mapEventObjOutlinePass));

		MadoEngine::Render::LayerEffectPass::Desc playerDissolvePass{};
		playerDissolvePass.key = "PlayerDissolve";
		playerDissolvePass.name = "プレイヤーDissolve";
		playerDissolvePass.targetLayerMask = MadoEngine::Render::ToRenderLayerMask(MadoEngine::Render::RenderLayer::Player);
		playerDissolvePass.effectShaderKey = "PostEffect/Dissolve.PS";
		playerDissolvePass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredDissolvePass = execution.AddLayerEffectPass(playerDissolvePass);
		registeredDissolvePass->SetParameterData(DissolveParams{});
		registeredDissolvePass->AddFloatParameterControl("Progress", "Dissolve進行度", offsetof(DissolveParams, dissolveParams) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
		registeredDissolvePass->AddFloatParameterControl("EdgeWidth", "Dissolve境界幅", offsetof(DissolveParams, dissolveParams) + sizeof(float) * 1, 0.001f, 0.3f, 0.001f);
		registeredDissolvePass->AddFloatParameterControl("EdgeIntensity", "Dissolve境界強度", offsetof(DissolveParams, dissolveParams) + sizeof(float) * 2, 0.0f, 5.0f, 0.01f);
		registeredDissolvePass->AddFloatParameterControl("NoiseScale", "Dissolveノイズ倍率", offsetof(DissolveParams, dissolveParams) + sizeof(float) * 3, 0.1f, 16.0f, 0.1f);
		registeredDissolvePass->AddFloatParameterControl("EdgeColorR", "Dissolve境界色 R", offsetof(DissolveParams, edgeColor) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
		registeredDissolvePass->AddFloatParameterControl("EdgeColorG", "Dissolve境界色 G", offsetof(DissolveParams, edgeColor) + sizeof(float) * 1, 0.0f, 1.0f, 0.01f);
		registeredDissolvePass->AddFloatParameterControl("EdgeColorB", "Dissolve境界色 B", offsetof(DissolveParams, edgeColor) + sizeof(float) * 2, 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc bloomPass{};
		bloomPass.key = "Bloom";
		bloomPass.name = "ブルーム";
		bloomPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		bloomPass.effectShaderKey = "PostEffect/Bloom.PS";
		bloomPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredBloomPass = execution.AddScreenEffectPass(bloomPass);
		registeredBloomPass->SetParameterData(BloomParams{});
		registeredBloomPass->AddFloatParameterControl("Intensity", "発光の強さ", offsetof(BloomParams, intensity), 0.0f, 5.0f, 0.01f);
		registeredBloomPass->AddFloatParameterControl("Threshold", "発光しきい値", offsetof(BloomParams, threshold), 0.0f, 1.0f, 0.01f);
		registeredBloomPass->AddFloatParameterControl("Radius", "ぼかし半径", offsetof(BloomParams, radius), 0.0f, 32.0f, 0.1f);
		registeredBloomPass->AddFloatParameterControl("SoftKnee", "境界の柔らかさ", offsetof(BloomParams, softKnee), 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc vignettePass{};
		vignettePass.key = "Vignette";
		vignettePass.name = "周辺減光";
		vignettePass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		vignettePass.effectShaderKey = "PostEffect/Vignette.PS";
		vignettePass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredVignettePass = execution.AddScreenEffectPass(vignettePass);
		registeredVignettePass->SetParameterData(VignetteParams{});
		registeredVignettePass->AddFloatParameterControl("Strength", "減光の強さ", offsetof(VignetteParams, strength), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Radius", "減光半径", offsetof(VignetteParams, radius), 0.0f, 100.0f, 0.01f);
		registeredVignettePass->AddFloatParameterControl("Smoothness", "滑らかさ", offsetof(VignetteParams, smoothness), 1.0f, 100.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc fogPass{};
		fogPass.key = "Fog";
		fogPass.name = "Fog";
		fogPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		fogPass.effectShaderKey = "PostEffect/Fog.PS";
		fogPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredFogPass = execution.AddScreenEffectPass(fogPass);
		registeredFogPass->SetParameterData(FogParams{});
		registeredFogPass->AddFloatParameterControl("ColorR", "Fog色 R", offsetof(FogParams, fogColor) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("ColorG", "Fog色 G", offsetof(FogParams, fogColor) + sizeof(float) * 1, 0.0f, 1.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("ColorB", "Fog色 B", offsetof(FogParams, fogColor) + sizeof(float) * 2, 0.0f, 1.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("StartDistance", "Fog開始距離", offsetof(FogParams, fogDistanceParams) + sizeof(float) * 0, 0.0f, 10000.0f, 1.0f);
		registeredFogPass->AddFloatParameterControl("EndDistance", "Fog終了距離", offsetof(FogParams, fogDistanceParams) + sizeof(float) * 1, 0.0f, 10000.0f, 1.0f);
		registeredFogPass->AddFloatParameterControl("Density", "Fog濃度", offsetof(FogParams, fogDistanceParams) + sizeof(float) * 2, 0.0f, 3.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("HeightIntensity", "Fog高さ強度", offsetof(FogParams, fogDistanceParams) + sizeof(float) * 3, 0.0f, 2.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("CameraNear", "Camera Near", offsetof(FogParams, fogCameraParams) + sizeof(float) * 0, 0.001f, 1000.0f, 0.01f);
		registeredFogPass->AddFloatParameterControl("CameraFar", "Camera Far", offsetof(FogParams, fogCameraParams) + sizeof(float) * 1, 1.0f, 10000.0f, 1.0f);

		MadoEngine::Render::LayerEffectPass::Desc pixelArtPass{};
		pixelArtPass.key = "PixelArt";
		pixelArtPass.name = "ドット絵風";
		pixelArtPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		pixelArtPass.effectShaderKey = "PostEffect/PixelArt.PS";
		pixelArtPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredPixelArtPass = execution.AddScreenEffectPass(pixelArtPass);
		registeredPixelArtPass->SetParameterData(PixelArtParams{});
		registeredPixelArtPass->AddFloatParameterControl("PixelSize", "ピクセルサイズ", offsetof(PixelArtParams, pixelSize), 1.0f, 128.0f, 1.0f);
		registeredPixelArtPass->AddFloatParameterControl("ColorSteps", "色数", offsetof(PixelArtParams, colorSteps), 2.0f, 128.0f, 1.0f);
		registeredPixelArtPass->AddFloatParameterControl("Contrast", "コントラスト", offsetof(PixelArtParams, contrast), 0.0f, 3.0f, 0.01f);
		registeredPixelArtPass->AddFloatParameterControl("Intensity", "効果の強さ", offsetof(PixelArtParams, intensity), 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc toonPass{};
		toonPass.key = "Toon";
		toonPass.name = "トゥーン";
		toonPass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		toonPass.effectShaderKey = "PostEffect/Toon.PS";
		toonPass.enabled = false;

		MadoEngine::Render::LayerEffectPass* registeredToonPass = execution.AddScreenEffectPass(toonPass);
		registeredToonPass->SetParameterData(ToonParams{});
		registeredToonPass->AddFloatParameterControl("ColorSteps", "色階調数", offsetof(ToonParams, colorParams) + sizeof(float) * 0, 2.0f, 12.0f, 1.0f);
		registeredToonPass->AddFloatParameterControl("Saturation", "彩度", offsetof(ToonParams, colorParams) + sizeof(float) * 1, 0.0f, 3.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("Contrast", "コントラスト", offsetof(ToonParams, colorParams) + sizeof(float) * 2, 0.0f, 4.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("Intensity", "適用率", offsetof(ToonParams, colorParams) + sizeof(float) * 3, 0.0f, 1.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("Thickness", "輪郭太さ", offsetof(ToonParams, edgeParams) + sizeof(float) * 0, 0.25f, 12.0f, 0.05f);
		registeredToonPass->AddFloatParameterControl("DepthSensitivity", "深度感度", offsetof(ToonParams, edgeParams) + sizeof(float) * 1, 1.0f, 300.0f, 1.0f);
		registeredToonPass->AddFloatParameterControl("EdgeThreshold", "輪郭しきい値", offsetof(ToonParams, edgeParams) + sizeof(float) * 2, 0.0001f, 0.1f, 0.0001f);
		registeredToonPass->AddFloatParameterControl("EdgeIntensity", "輪郭強度", offsetof(ToonParams, edgeParams) + sizeof(float) * 3, 0.0f, 4.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("OutlineColorR", "輪郭色 R", offsetof(ToonParams, outlineColor) + sizeof(float) * 0, 0.0f, 1.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("OutlineColorG", "輪郭色 G", offsetof(ToonParams, outlineColor) + sizeof(float) * 1, 0.0f, 1.0f, 0.01f);
		registeredToonPass->AddFloatParameterControl("OutlineColorB", "輪郭色 B", offsetof(ToonParams, outlineColor) + sizeof(float) * 2, 0.0f, 1.0f, 0.01f);

		MadoEngine::Render::LayerEffectPass::Desc screenOutlinePass{};
		screenOutlinePass.key = "ScreenOutline";
		screenOutlinePass.name = "画面全体輪郭";
		screenOutlinePass.targetLayerMask = MadoEngine::Render::kAllRenderLayers;
		screenOutlinePass.effectShaderKey = "PostEffect/Outline.PS";
		screenOutlinePass.enabled = false;
		setupOutlineParameters(execution.AddScreenEffectPass(screenOutlinePass));
	}

} // namespace RenderPassSetup
