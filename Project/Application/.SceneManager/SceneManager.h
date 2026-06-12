#pragma once
#include "SceneType.h"
#include "Render/Object/RenderLayer.h"
#include "Render/Object/3d/Model/Model.h"
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

	/// @brief シーン生成関数を登録します
	/// @param type シーンのタイプ
	/// @param creator シーン生成関数
	void RegisterScene(SceneType type, CreatorFunc creator);

	/// @brief SceneManagerを初期化します
	/// @param initialScene 初期シーンのタイプ
	void Initialize(SceneType initialScene);

	/// @brief 現在のシーンを更新します
	void Update(float dt);

	/// @brief 現在のシーンを描画します
	void Draw();

	/// @brief 指定した描画レイヤーのみを描画します
	/// @param layer 描画対象のレイヤー
	void DrawLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 指定したレイヤーマスクに含まれる描画対象を描画します
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 現在のシーン固有の描画処理を実行します
	void DrawCurrentScene();

	/// @brief 現在のシーンのImGuiを描画します
	void DrawImGui();

	/// @brief 保留中のシーン遷移を適用します
	void ApplyPendingSceneChange();

private:
	/// @brief SceneManagerのデバッグ用ImGuiを描画します
	void DrawSceneManagerImGui();

	/// @brief 現在のフレームの終了時にシーン遷移をリクエストします
	/// @param type 遷移先のシーンのタイプ
	void RequestSceneChange(SceneType type);

	/// @brief 指定されたシーンに変更します
	/// @param type 遷移先のシーンのタイプ
	void ChangeScene(SceneType type);

	std::map<SceneType, CreatorFunc> creators_;  // 登録されたシーン生成関数
	std::unique_ptr<IScene> currentScene_;       // 現在のシーン
	SceneType currentSceneType_;                 // 現在のシーンのタイプ
	SceneType pendingSceneType_;                 // 保留中の遷移先シーン
	bool hasPendingSceneChange_;                 // シーン遷移が保留中かどうか

	Model* selectedModel_;     // 選択されたモデル
};
