#pragma once
#include ".SceneManager/IScene.h"
#include "Object/Player/Player.h"

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

	/// @brief ImGui描画処理
	void DrawImGui() override;
private:
	std::array<Sprite* ,10> sprites_;
	Vector2 testPos_ = { 0.0f, 0.0f };

	DebugCamera debugCamera_;

	Shape testShape1_;
	Shape testShape2_;

	Vector3 testPos1_ = { -2.0f, 2.0f, 0.0f };
	Vector3 testPos2_ = { 2.0f, 2.0f, 0.0f };

	std::unique_ptr<Sprite> sprite_;
	std::unique_ptr<Player> player_;
};
