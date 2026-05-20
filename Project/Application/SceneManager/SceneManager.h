#pragma once
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

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
	/// @param sceneName シーン名文字列（例: "Title", "Game"）
	/// @param creator シーン生成関数
	void RegisterScene(const std::string& sceneName, CreatorFunc creator);

	/// @brief シーンマネージャーの初期化
	/// @param initialScene 最初に起動するシーン名文字列
	void Initialize(const std::string& initialScene);

	/// @brief 現在のシーンを更新
	void Update();

	/// @brief 現在のシーンを描画
	void Draw();

private:
	/// @brief 次のシーンに遷移
	/// @param sceneName 遷移先のシーン名文字列
	void ChangeScene(const std::string& sceneName);

	std::unordered_map<std::string, CreatorFunc> creators_;  // 登録済みシーン生成関数マップ
	std::unique_ptr<IScene> currentScene_;                   // 現在のシーン
	std::string currentSceneName_;                           // 現在のシーン名
};