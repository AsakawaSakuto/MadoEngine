#pragma once
#include ".SceneManager/IScene.h"
#include "Object/Player/Player.h"
#include "Object/Map/Map.h"

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
	std::array<Sprite* ,10> sprites_;
	Vector2 testPos_ = { 0.0f, 0.0f };

	DebugCamera debugCamera_;
	TPS_Camera tpsCamera_;

	std::unique_ptr<Sprite> sprite_;

	std::unique_ptr<Player> player_;

	std::unique_ptr<Map> map_;

	Model* model_ = nullptr;
	Model* selectedModel_ = nullptr;
	Vector3 modelPos_ = { -10.0f, 0.0f, -10.0f };

	Sprite* fadeSprite_ = nullptr;
	GameTimer fadeOutTimer_;
};
