#pragma once
#include "SceneManager/IScene.h"

class SceneManager;

/// @brief タイトルシーン
/// @details ゲームのタイトル画面を表示し、スペースキーでゲームシーンに遷移
class Title : public IScene
{
public:
	/// @brief コンストラクタ
	Title();

	/// @brief デストラクタ
	~Title() override;

	/// @brief 初期化処理
	void Initialize() override;

	/// @brief 更新処理
	void Update() override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief シーンマネージャーを設定
	/// @param sceneManager シーンマネージャーのポインタ
	void SetSceneManager(SceneManager* sceneManager);

private:
	SceneManager* sceneManager_;  // シーンマネージャーへの参照
};
