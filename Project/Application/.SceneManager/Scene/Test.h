#pragma once
#include ".SceneManager/IScene.h"
#include "Object/Player/Player.h"
#include "Object/Map/Map.h"
#include "Object/Enemy/EnemySpawner.h"
#include "Utility/Light/LightManager.h"
#include "UI/Player/ExpGauge.h"
#include "UI/Player/HealthGauge.h"

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
	
	DebugCamera debugCamera_;
	TPS_Camera tpsCamera_;

	std::unique_ptr<Player::Base> player_;

	std::unique_ptr<Player::ExpGauge> expGauge_;
	std::unique_ptr<Player::HealthGauge> healthGauge_;

	std::unique_ptr<Map> map_;

	std::unique_ptr<EnemySpawner> enemySpawner_;


	Sprite* fadeSprite_ = nullptr;
	GameTimer fadeOutTimer_;

	bool useDebugCamera_ = false;
};