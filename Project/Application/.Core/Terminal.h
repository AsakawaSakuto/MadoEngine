#pragma once
#include ".Execution/Execution.h"
#include ".SceneManager/SceneManager.h"
#include ".SceneManager/Scene/Test.h"
#include ".SceneManager/Scene/Title.h"
#include ".SceneManager/Scene/Game.h"
#include ".SceneManager/Scene/Result.h"
#include <vector>

class Terminal final : private MadoEngine::Render::IRenderLayerBatchContext {
public:

	Terminal(HINSTANCE hInstance);

	void Run();

private:
	/// @brief シーン遷移進行度をPixelArtのピクセルサイズへ反映
	void UpdateSceneTransitionPixelArt();

	/// @brief 指定段階のレイヤーポストエフェクトPassを実行
	/// @param stage 実行する適用段階
	void ApplyLayerEffectPasses(MadoEngine::Render::LayerEffectStage stage);

	/// @brief 2D描画の連続レイヤーバッチを開始
	/// @param layer 描画するレイヤー
	void BeginRenderLayerBatch(MadoEngine::Render::RenderLayer layer) override;

	/// @brief 2D描画の連続レイヤーバッチを終了
	/// @param layer 描画したレイヤー
	void EndRenderLayerBatch(MadoEngine::Render::RenderLayer layer) override;

	std::unique_ptr<MadoEngine::EngineExecution> execution_;
	std::unique_ptr<SceneManager> sceneManager_;
	std::vector<MadoEngine::Render::PostEffectPassHandle> activeOverlayPassHandles_;
	MadoEngine::Render::RenderLayer activeOverlayLayer_ = MadoEngine::Render::RenderLayer::Default;
	bool isOverlayEffectBatchActive_ = false;
};
