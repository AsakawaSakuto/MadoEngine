#pragma once
#include "SceneManager/IScene.h"

class SceneManager;

/// @brief リザルトシーン
/// @details ゲームの結果を表示し、スペースキーでタイトルシーンに遷移
class Result : public IScene
{
public:
	/// @brief コンストラクタ
	Result();

	/// @brief デストラクタ
	~Result() override;

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
