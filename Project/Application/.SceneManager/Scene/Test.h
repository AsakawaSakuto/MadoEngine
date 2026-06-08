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

	Shape plane_;

	Shape slope_;
	Vector3 slopePos_ = { -30.0f, 5.0f, -10.0f };

	Shape slope2_;
	Vector3 slope2Pos_ = { -20.0f, 10.0f, -10.0f };

	Shape aabb_;
	Vector3 aabbPos_ = { -20.0f, 5.0f, -10.0f };

	Vector3 planePos_ = { 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Sprite> sprite_;

	std::unique_ptr<Player> player_;

	std::unique_ptr<Map> map_;

	Model* model_ = nullptr;
};
