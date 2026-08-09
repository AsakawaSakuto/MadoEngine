#include "Terminal.h"
#include <cassert>
#include <cstddef>

Terminal::Terminal(HINSTANCE hInstance) {
	execution_ = std::make_unique<MadoEngine::EngineExecution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->RegisterScene(SceneType::Test,   []() { return std::make_unique<Test>(); });
	sceneManager_->RegisterScene(SceneType::Title,  []() { return std::make_unique<Title>(); });
	sceneManager_->RegisterScene(SceneType::Game,   []() { return std::make_unique<Game>(); });
	sceneManager_->RegisterScene(SceneType::Result, []() { return std::make_unique<Result>(); });
	sceneManager_->Initialize(SceneType::Title);
}

void Terminal::ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage stage) {
	assert(MadoEngine::Render::IsValidLayerEffectStage(stage) && "LayerEffectStageが範囲外です");
	assert(stage != MadoEngine::Render::LayerEffectStage::Overlay &&
		"OverlayのLayerEffectは描画順を維持するバッチ処理から実行してください");

	const std::vector<MadoEngine::Render::PostEffectPassHandle>& layerEffectPassHandles =
		execution_->GetLayerEffectPassHandles();
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
			sceneManager_->DrawParticleLayerMask(chainLayerMask);
			break;
		default:
			break;
		}
		execution_->EndLayerEffectRender();

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

		execution_->CompositeLayerEffectChain();
	}
}

void Terminal::BeginRenderLayerBatch(MadoEngine::Render::RenderLayer layer) {
	assert(!isOverlayEffectBatchActive_ && "前のOverlayレイヤーバッチが終了していません");
	activeOverlayPassHandles_.clear();

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

		if (execution_->IsStopApplication()) {

		} else {
			sceneManager_->Update(execution_->GetDeltaTime());
		}

		execution_->PreDraw(
			sceneManager_->GetCurrentSceneType(),
			sceneManager_->GetShadowFocusPosition()
		);

		const MadoEngine::Render::RenderLayerMask sceneLayerEffectTargetMask =
			execution_->GetEnabledLayerEffectTargetMask(MadoEngine::Render::LayerEffectStage::Scene);
		if (sceneLayerEffectTargetMask != 0) {
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
			execution_->EndSceneColorRender();
			ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage::Scene);
		} else {
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
		}

		execution_->BeginTransparentRender();
		sceneManager_->DrawParticleLayerMask(MadoEngine::Render::kAllRenderLayers);
		execution_->EndTransparentRender();
		ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage::Transparent);

		execution_->BeginOverlayRender();
		sceneManager_->DrawOverlayInOrder(*this);
		assert(!isOverlayEffectBatchActive_ && "Overlayレイヤーバッチが終了していません");
		execution_->EndOverlayRender();

		// DockSpaceを先に生成してから、シーンのImGuiウィンドウを作成する
		execution_->BeginImGuiLayout();

		sceneManager_->DrawImGui();

		execution_->PostDraw();

		// シーン遷移の予約があれば、フレームの最後に遷移を実行する
		sceneManager_->ApplyPendingSceneChange();
	}

}
