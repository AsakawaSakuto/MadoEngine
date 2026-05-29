#pragma once
#include <memory>
#include <functional>
#include <map>
#include "SceneType.h"

class IScene;

/// @brief シーンを管理するマネージャークラス
/// @details シーンの遷移、更新、描画を一元管理する
class SceneManager
{
public:
	/// @brief シーン生成関数の型エイリアス
	using CreatorFunc = std::function<std::unique_ptr<IScene>()>;

	/// @brief コンストラクタ
	SceneManager();

	/// @brief デストラクタ
	~SceneManager();

	/// @brief シーンを登録する
	/// @param type シーンの種類
	/// @param creator シーン生成関数
	void RegisterScene(SceneType type, CreatorFunc creator);

	/// @brief シーンマネージャーの初期化
	/// @param initialScene 最初に起動するシーンの種類
	void Initialize(SceneType initialScene);

	/// @brief 現在のシーンを更新
	void Update();

	/// @brief 現在のシーンを描画
	void Draw();

	/// @brief 現在のシーンのImGui描画
	void DrawImGui();
private:
	/// @brief 次のシーンに遷移
	/// @param type 遷移先のシーンの種類
	void ChangeScene(SceneType type);

	std::map<SceneType, CreatorFunc> creators_;  // 登録済みシーン生成関数マップ
	std::unique_ptr<IScene> currentScene_;        // 現在のシーン
	SceneType currentSceneType_;                  // 現在のシーンの種類
};