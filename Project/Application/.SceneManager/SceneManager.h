#pragma once
#include "SceneType.h"
#include "Render/Object/2d/IRenderLayerBatchContext.h"
#include "Render/Object/RenderLayer.h"
#include "Render/Object/3d/Model/ModelManager.h"
#include <memory>
#include <functional>
#include <map>
#include <memory>

class IScene;

/// @brief シーンのライフサイクルを管理します
/// @details シーンの登録、更新、描画、および遷移を処理します
class SceneManager
{
public:
	/// @brief シーン生成関数の型
	using CreatorFunc = std::function<std::unique_ptr<IScene>()>;

	/// @brief コンストラクタ
	SceneManager();

	/// @brief デストラクタ
	~SceneManager();

	/// @brief シーン生成関数を登録
	/// @param type シーンのタイプ
	/// @param creator シーン生成関数
	void RegisterScene(SceneType type, CreatorFunc creator);

	/// @brief SceneManagerを初期化
	/// @param initialScene 初期シーンのタイプ
	void Initialize(SceneType initialScene);

	/// @brief 現在のシーンを更新
	void Update(float dt);

	/// @brief 現在のシーンを描画
	void Draw();

	/// @brief 指定した描画レイヤーのみを描画
	/// @param layer 描画対象のレイヤー
	void DrawLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 指定したレイヤーマスクに含まれる描画対象を描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれるScene段階の描画対象を描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawSceneLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれる透過Effectを描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawParticleLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれるSpriteとTextを描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawOverlayLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief SpriteとTextを元の描画順の連続レイヤーバッチとして描画
	/// @param batchContext バッチ前後の描画処理を受け取るContext
	void DrawOverlayInOrder(MadoEngine::Render::IRenderLayerBatchContext& batchContext);

	/// @brief 現在のシーン固有の描画処理を実行
	void DrawCurrentScene();

	/// @brief 現在のシーンのImGuiを描画
	void DrawImGui();

	/// @brief 現在のシーン種別を取得
	/// @return 現在のシーン種別
	SceneType GetCurrentSceneType() const { return currentSceneType_; }

	/// @brief 現在のシーンカメラを取得
	/// @return 現在のシーンカメラ
	const Camera& GetCurrentCamera() const;

	/// @brief 現在のシーンがシャドウマップ生成で注視したい座標を取得
	/// @return シャドウマップの注視点
	Vector3 GetShadowFocusPosition() const;

	/// @brief 現在のシーンからシャドウマップ確認用の対象座標を取得
	/// @param outPosition 対象のワールド座標を受け取る変数
	/// @return 対象座標を取得できた場合はtrue
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const;

	/// @brief 保留中のシーン遷移を適用
	void ApplyPendingSceneChange();

private:
	/// @brief SceneManagerのデバッグ用ImGuiを描画
	void DrawSceneManagerImGui();

	/// @brief 現在のフレームの終了時にシーン遷移をリクエスト
	/// @param type 遷移先のシーンのタイプ
	void RequestSceneChange(SceneType type);

	/// @brief 指定されたシーンに変更
	/// @param type 遷移先のシーンのタイプ
	void ChangeScene(SceneType type);

	std::map<SceneType, CreatorFunc> creators_;  // 登録されたシーン生成関数
	std::unique_ptr<IScene> currentScene_;       // 現在のシーン
	SceneType currentSceneType_;                 // 現在のシーンのタイプ
	SceneType pendingSceneType_;                 // 保留中の遷移先シーン
	bool hasPendingSceneChange_;                 // シーン遷移が保留中かどうか

	MadoEngine::ModelHandle selectedModel_{}; // 選択されたモデル
};
