#include "Terminal.h"
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

		const MadoEngine::Render::RenderLayerMask layerEffectTargetMask = execution_->GetEnabledLayerEffectTargetMask();
		if (layerEffectTargetMask != 0) {
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
			execution_->EndSceneColorRender();

			const std::vector<MadoEngine::Render::PostEffectPassHandle>& layerEffectPassHandles =
				execution_->GetLayerEffectPassHandles();
			for (std::size_t passIndex = 0; passIndex < layerEffectPassHandles.size(); ++passIndex) {
				const MadoEngine::Render::PostEffectPass* layerEffectPass =
					execution_->TryGetPostEffectPass(layerEffectPassHandles[passIndex]);
				if (!layerEffectPass || !layerEffectPass->IsEnabled() || layerEffectPass->GetTargetLayerMask() == 0) {
					continue;
				}

				const MadoEngine::Render::RenderLayerMask chainLayerMask = layerEffectPass->GetTargetLayerMask();
				execution_->BeginLayerEffectRender(*layerEffectPass);
				sceneManager_->DrawSceneLayerMask(chainLayerMask);
				execution_->EndLayerEffectRender();

				execution_->ApplyLayerEffectToChain(*layerEffectPass);

				while (passIndex + 1 < layerEffectPassHandles.size()) {
					const MadoEngine::Render::PostEffectPass* nextLayerEffectPass =
						execution_->TryGetPostEffectPass(layerEffectPassHandles[passIndex + 1]);
					if (!nextLayerEffectPass || !nextLayerEffectPass->IsEnabled() || nextLayerEffectPass->GetTargetLayerMask() == 0) {
						++passIndex;
						continue;
					}

					if (nextLayerEffectPass->GetTargetLayerMask() != chainLayerMask) {
						break;
					}

					++passIndex;
					execution_->ApplyLayerEffectToChain(*nextLayerEffectPass);
				}

				execution_->CompositeLayerEffectChain();
			}
		} else {
			sceneManager_->DrawSceneLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
		}

		execution_->BeginTransparentRender();
		sceneManager_->DrawParticleLayerMask(MadoEngine::Render::kAllRenderLayers);
		execution_->EndTransparentRender();

		execution_->BeginOverlayRender();
		sceneManager_->DrawOverlayLayerMask(MadoEngine::Render::kAllRenderLayers);
		execution_->EndOverlayRender();

		// DockSpaceを先に生成してから、シーンのImGuiウィンドウを作成する
		execution_->BeginImGuiLayout();

		sceneManager_->DrawImGui();

		execution_->PostDraw();

		// シーン遷移の予約があれば、フレームの最後に遷移を実行する
		sceneManager_->ApplyPendingSceneChange();
	}

}
