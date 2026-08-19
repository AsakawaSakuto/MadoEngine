#include "Terminal.h"
#include <algorithm>
#include <cassert>
#include <cstddef>

namespace {
	constexpr const char* kSceneTransitionPassName = "PixelArt";
	constexpr const char* kPixelSizeParameterKey = "PixelSize";
	constexpr float kMinimumPixelSize = 1.0f;
	constexpr float kMaximumPixelSize = 64.0f;
}

Terminal::Terminal(HINSTANCE hInstance) {
	execution_ = std::make_unique<MadoEngine::EngineExecution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();

	// 初期Scene生成前に利用可能なSceneTypeとFactoryを登録
	sceneManager_->RegisterScene(SceneType::Test,   [](CommonData&) { return std::make_unique<Test>(); });
	sceneManager_->RegisterScene(SceneType::Title,  [](CommonData& commonData) { return std::make_unique<Title>(commonData); });
	sceneManager_->RegisterScene(SceneType::Game,   [](CommonData& commonData) { return std::make_unique<Game>(commonData); });
	sceneManager_->RegisterScene(SceneType::Result, [](CommonData& commonData) { return std::make_unique<Result>(commonData); });
	sceneManager_->Initialize(SceneType::Title);
}

void Terminal::UpdateSceneTransitionPixelArt() {
	MadoEngine::Render::PostEffectManager& postEffectManager = execution_->GetPostEffectManager();
	const bool isTransitioning = sceneManager_->IsSceneTransitioning();
	const float progress = std::clamp(
		sceneManager_->GetSceneTransitionEffectProgress(),
		0.0f,
		1.0f
	);
	const float pixelSize =
		kMinimumPixelSize + (kMaximumPixelSize - kMinimumPixelSize) * progress;

	// Editorで内部キーが変更されても追従できるよう表示名とEffect種別の両方で対象を特定
	for (MadoEngine::Render::PostEffectPassHandle handle : postEffectManager.GetScreenPassHandles()) {
		MadoEngine::Render::PostEffectPass* pass = postEffectManager.TryGet(handle);
		if (!pass || pass->GetName() != kSceneTransitionPassName ||
			pass->GetPostEffectType() != MadoEngine::Render::PostEffectType::PixelArt) {
			continue;
		}

		// 遷移完了時は通常描画の負荷を避けるためPixelArt Passを無効化
		postEffectManager.SetFloatParameter(handle, kPixelSizeParameterKey, pixelSize);
		postEffectManager.SetEnabled(handle, isTransitioning);
		return;
	}
}

void Terminal::ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage stage) {
	assert(MadoEngine::Render::IsValidLayerEffectStage(stage) && "LayerEffectStageが範囲外です");
	assert(stage != MadoEngine::Render::LayerEffectStage::Overlay &&
		"OverlayのLayerEffectは描画順を維持するバッチ処理から実行してください");

	const std::vector<MadoEngine::Render::PostEffectPassHandle>& layerEffectPassHandles =
		execution_->GetLayerEffectPassHandles();

	// 指定Stageで有効なPassだけを登録順に処理
	for (std::size_t passIndex = 0; passIndex < layerEffectPassHandles.size(); ++passIndex) {
		const MadoEngine::Render::PostEffectPass* layerEffectPass =
			execution_->TryGetPostEffectPass(layerEffectPassHandles[passIndex]);
		if (!layerEffectPass || !layerEffectPass->IsEnabled() ||
			layerEffectPass->GetTargetLayerMask() == 0 ||
			layerEffectPass->GetLayerEffectStage() != stage) {
			continue;
		}

		const MadoEngine::Render::RenderLayerMask chainLayerMask = layerEffectPass->GetTargetLayerMask();
		execution_->BeginLayerEffectRender(*layerEffectPass);
		switch (stage) {
		case MadoEngine::Render::LayerEffectStage::Scene:
			sceneManager_->DrawSceneLayerMask(chainLayerMask);
			break;
		case MadoEngine::Render::LayerEffectStage::Transparent:
			sceneManager_->DrawTransparentLayerMask(chainLayerMask);
			break;
		default:
			break;
		}
		execution_->EndLayerEffectRender();

		// 同じLayerMaskへ連続適用するPassを中間合成せず一つのChainへ集約
		execution_->ApplyLayerEffectToChain(*layerEffectPass);
		while (passIndex + 1 < layerEffectPassHandles.size()) {
			const MadoEngine::Render::PostEffectPass* nextLayerEffectPass =
				execution_->TryGetPostEffectPass(layerEffectPassHandles[passIndex + 1]);
			if (!nextLayerEffectPass || !nextLayerEffectPass->IsEnabled() ||
				nextLayerEffectPass->GetTargetLayerMask() == 0) {
				++passIndex;
				continue;
			}

			if (nextLayerEffectPass->GetLayerEffectStage() != stage ||
				nextLayerEffectPass->GetTargetLayerMask() != chainLayerMask) {
				break;
			}

			++passIndex;
			execution_->ApplyLayerEffectToChain(*nextLayerEffectPass);
		}

		// Chain単位で元の描画先へ合成してPass間の不要な往復を回避
		execution_->CompositeLayerEffectChain();
	}
}

void Terminal::BeginRenderLayerBatch(MadoEngine::Render::RenderLayer layer) {
	assert(!isOverlayEffectBatchActive_ && "前のOverlayレイヤーバッチが終了していません");
	activeOverlayPassHandles_.clear();

	// 描画順を維持するため現在Layerを対象に含むOverlay Passだけを収集
	for (MadoEngine::Render::PostEffectPassHandle handle : execution_->GetLayerEffectPassHandles()) {
		const MadoEngine::Render::PostEffectPass* pass = execution_->TryGetPostEffectPass(handle);
		if (!pass || !pass->IsEnabled() ||
			pass->GetLayerEffectStage() != MadoEngine::Render::LayerEffectStage::Overlay ||
			!MadoEngine::Render::ContainsRenderLayer(pass->GetTargetLayerMask(), layer)) {
			continue;
		}

		activeOverlayPassHandles_.push_back(handle);
	}

	if (activeOverlayPassHandles_.empty()) {
		return;
	}

	// 通常Overlay描画を一時終了してLayer専用のEffect描画先へ切り替え
	execution_->EndOverlayRender();
	const MadoEngine::Render::PostEffectPass* firstPass =
		execution_->TryGetPostEffectPass(activeOverlayPassHandles_.front());
	assert(firstPass && "Overlayレイヤーバッチの先頭Passが無効です");
	if (!firstPass) {
		activeOverlayPassHandles_.clear();
		execution_->BeginOverlayRender();
		return;
	}

	execution_->BeginLayerEffectRender(*firstPass);
	activeOverlayLayer_ = layer;
	isOverlayEffectBatchActive_ = true;
}

void Terminal::EndRenderLayerBatch(MadoEngine::Render::RenderLayer layer) {
	if (!isOverlayEffectBatchActive_) {
		return;
	}

	assert(layer == activeOverlayLayer_ && "開始時と異なるOverlayレイヤーバッチを終了しようとしています");
	execution_->EndLayerEffectRender();

	// 収集済みPassを同一Chainへ適用してからOverlay描画先へ復帰
	for (MadoEngine::Render::PostEffectPassHandle handle : activeOverlayPassHandles_) {
		const MadoEngine::Render::PostEffectPass* pass = execution_->TryGetPostEffectPass(handle);
		assert(pass && "OverlayレイヤーバッチのPassが無効です");
		if (pass) {
			execution_->ApplyLayerEffectToChain(*pass);
		}
	}
	execution_->CompositeLayerEffectChain();
	execution_->BeginOverlayRender();

	activeOverlayPassHandles_.clear();
	activeOverlayLayer_ = MadoEngine::Render::RenderLayer::Default;
	isOverlayEffectBatchActive_ = false;
}

void Terminal::Run() {

	while (execution_->IsRunning()) {

		execution_->Update();

		// Application停止要求後は描画を維持しつつGame状態の更新だけを停止
		if (execution_->IsStopApplication()) {

		} else {
			sceneManager_->Update(execution_->GetDeltaTime());
		}
		execution_->PreDraw(
			sceneManager_->GetCurrentSceneType(),
			sceneManager_->GetShadowFocusPosition()
		);
		UpdateSceneTransitionPixelArt();

		const MadoEngine::Render::RenderLayerMask sceneLayerEffectTargetMask =
			execution_->GetEnabledLayerEffectTargetMask(MadoEngine::Render::LayerEffectStage::Scene);
		if (sceneLayerEffectTargetMask != 0) {

			// Scene Effect利用時だけ中間Color Targetを閉じてEffect Chainへ接続
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
			execution_->EndSceneColorRender();
			ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage::Scene);
		} else {
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
		}

		execution_->BeginTransparentRender();

		// 透明Modelと透明Effectを完成済みScene Colorへ合成して背景の透過を維持
		sceneManager_->DrawTransparentLayerMask(MadoEngine::Render::kAllRenderLayers);
		execution_->EndTransparentRender();
		ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage::Transparent);

		execution_->BeginOverlayRender();

		// SpriteとTextの描画順を保ったままLayer単位のOverlay Effectを適用
		sceneManager_->DrawOverlayInOrder(*this);
		assert(!isOverlayEffectBatchActive_ && "Overlayレイヤーバッチが終了していません");
		execution_->EndOverlayRender();

		// DockSpace生成後にScene固有のImGui Windowを構築
		execution_->BeginImGuiLayout();

		sceneManager_->DrawImGui();

		execution_->PostDraw();

		// 描画中のResource破棄を避けるため予約済みScene遷移をFrame末尾で適用
		sceneManager_->ApplyPendingSceneChange();
	}

}
