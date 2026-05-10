#pragma once
#include "SceneManager/IScene.h"

class SceneManager;

/// @brief ゲームシーン
/// @details メインゲームのプレイ画面を管理し、スペースキーでリザルトシーンに遷移
class Game : public IScene
{
public:
	/// @brief コンストラクタ
	Game();

	/// @brief デストラクタ
	~Game() override;

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
