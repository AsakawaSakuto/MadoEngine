#pragma once
#include "SceneManager/IScene.h"

/// @brief テストシーン
/// @details 動作確認用のシーン。スペースキーでゲームシーンに遷移
class Test : public IScene
{
public:
	/// @brief コンストラクタ
	Test();

	/// @brief デストラクタ
	~Test() override;

	/// @brief 初期化処理
	void Initialize() override;

	/// @brief 更新処理
	/// @return 次に遷移するシーン名（遷移しない場合は "Test"）
	std::string Update() override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief 描画処理
	void Draw() override;

private:
	std::array<Sprite* ,10> sprites_;
};
