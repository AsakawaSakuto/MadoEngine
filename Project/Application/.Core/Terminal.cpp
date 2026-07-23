#include "Terminal.h"
#include <cstddef>

Terminal::Terminal(HINSTANCE hInstance)
{
	execution_ = std::make_unique<MadoEngine::EngineExecution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->RegisterScene(SceneType::Test,   []() { return std::make_unique<Test>(); });
	sceneManager_->RegisterScene(SceneType::Title,  []() { return std::make_unique<Title>(); });
	sceneManager_->RegisterScene(SceneType::Game,   []() { return std::make_unique<Game>(); });
	sceneManager_->RegisterScene(SceneType::Result, []() { return std::make_unique<Result>(); });
	sceneManager_->Initialize(SceneType::Game);
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
			sceneManager_->DrawLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
			execution_->EndSceneColorRender();

			const std::vector<MadoEngine::Render::LayerEffectPass>& layerEffectPasses = execution_->GetLayerEffectPasses();
			for (std::size_t passIndex = 0; passIndex < layerEffectPasses.size(); ++passIndex) {
				const MadoEngine::Render::LayerEffectPass& layerEffectPass = layerEffectPasses[passIndex];
				if (!layerEffectPass.IsEnabled() || layerEffectPass.GetTargetLayerMask() == 0) {
					continue;
				}

				const MadoEngine::Render::RenderLayerMask chainLayerMask = layerEffectPass.GetTargetLayerMask();
				execution_->BeginLayerEffectRender(layerEffectPass);
				sceneManager_->DrawLayerMask(chainLayerMask);
				execution_->EndLayerEffectRender();

				execution_->ApplyLayerEffectToChain(layerEffectPass);

				while (passIndex + 1 < layerEffectPasses.size()) {
					const MadoEngine::Render::LayerEffectPass& nextLayerEffectPass = layerEffectPasses[passIndex + 1];
					if (!nextLayerEffectPass.IsEnabled() || nextLayerEffectPass.GetTargetLayerMask() == 0) {
						++passIndex;
						continue;
					}

					if (nextLayerEffectPass.GetTargetLayerMask() != chainLayerMask) {
						break;
					}

					++passIndex;
					execution_->ApplyLayerEffectToChain(nextLayerEffectPass);
				}

				execution_->CompositeLayerEffectChain();
			}
		} else {
			sceneManager_->DrawLayerMask(MadoEngine::Render::kAllRenderLayers);
			sceneManager_->DrawCurrentScene();
		}

		// DockSpaceを先に生成してから、シーンのImGuiウィンドウを作成する
		execution_->BeginImGuiLayout();

		sceneManager_->DrawImGui();

		execution_->PostDraw();

		// シーン遷移の予約があれば、フレームの最後に遷移を実行する
		sceneManager_->ApplyPendingSceneChange();
	}

}
