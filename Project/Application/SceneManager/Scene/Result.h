#pragma once
#include "SceneManager/IScene.h"

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
	/// @return 次に遷移するシーン名（遷移しない場合は "Result"）
	std::string Update() override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief ImGui描画処理
	void DrawImGui() override;
};
