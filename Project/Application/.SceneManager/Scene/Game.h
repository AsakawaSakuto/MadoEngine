#pragma once
#include ".SceneManager/IScene.h"

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
	/// @param dt デルタタイム
	/// @return 次に遷移するシーンの種類
	SceneType Update(float dt) override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief ImGui描画処理
	void DrawImGui() override;
};
