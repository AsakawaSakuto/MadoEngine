#pragma once
#include <memory>
#include <string>

class IScene;

/// @brief シーンを管理するマネージャークラス
/// @details シーンの遷移、更新、描画を一元管理する
class SceneManager
{
public:
	/// @brief コンストラクタ
	SceneManager();

	/// @brief デストラクタ
	~SceneManager();

	/// @brief シーンマネージャーの初期化
	void Initialize();

	/// @brief 現在のシーンを更新
	void Update();

	/// @brief 現在のシーンを描画
	void Draw();

	/// @brief 次のシーンに遷移
	/// @param sceneName 遷移先のシーン名（"Title", "Game", "Result"）
	void ChangeScene(const std::string& sceneName);

private:
	/// @brief シーン名から対応するシーンのインスタンスを生成
	/// @param sceneName シーン名
	/// @return 生成されたシーンのユニークポインタ
	std::unique_ptr<IScene> CreateScene(const std::string& sceneName);

	std::unique_ptr<IScene> currentScene_;  // 現在のシーン
	std::unique_ptr<IScene> nextScene_;     // 次のシーン
	std::string currentSceneName_;          // 現在のシーン名
};