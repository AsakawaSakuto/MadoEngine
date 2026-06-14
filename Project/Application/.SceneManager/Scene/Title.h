#pragma once
#include ".SceneManager/IScene.h"

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
	/// @param dt デルタタイム
	/// @return 次に遷移するシーンの種類
	SceneType Update(float dt) override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief ImGui描画処理
	void DrawImGui() override;

private:
	Sprite* wallPaperSprite_ = nullptr;
	Sprite* fadeSprite_ = nullptr;

	GameTimer fadeInTimer_;
};
