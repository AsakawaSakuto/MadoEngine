#pragma once
#include ".SceneManager/CommonData.h"
#include ".SceneManager/IScene.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Map/Map.h"
#include "GameObject/Map/MapLimit.h"
#include "GameObject/Enemy/EnemyManager.h"
#include "GameObject/Enemy/EnemySpawner.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponStatusEditor.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include "GameObject/Weapon/Projectile/ProjectileStatus.h"
#include "Utility/Light/LightManager.h"
#include "UI/UIHeaders.h"
#include "System/InGameSession/InGameSession.h"
#include <cstdint>

/// @brief テストシーン
/// @details 動作確認用のシーン。スペースキーでゲームシーンに遷移
class Game : public IScene
{
public:
	/// @brief コンストラクタ
	/// @param commonData Sceneをまたいで保持するApplication共通データ
	explicit Game(CommonData& commonData);

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

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得
	/// @return Playerのワールド座標
	Vector3 GetShadowFocusPosition() const override;

	/// @brief シャドウマップ確認用のPlayer描画座標を取得
	/// @param outPosition PlayerのModelワールド座標を受け取る変数
	/// @return Player座標を取得できた場合はtrue
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const override;
private:
	CommonData& commonData_;
	std::uint32_t gameSeed_ = 0;

	CameraHandle debugCameraHandle_{};
	CameraHandle tpsCameraHandle_{};

	ColliderShape mapLimitBox_;
	Vector3 mapLimitBoxPos_;

	std::unique_ptr<Player::Base> player_;

	std::unique_ptr<Map> map_;

	std::unique_ptr<Enemy::Manager> enemyManager_;
	std::unique_ptr<Enemy::Spawner> enemySpawner_;

	std::unique_ptr<Weapon::Inventory> weaponInventory_;
	std::unique_ptr<Weapon::StatusEditor> weaponStatusEditor_;
	std::unique_ptr<Weapon::UpgradeSystem> weaponUpgradeSystem_;
	
	std::unique_ptr<UI::Game::PlayerIconUI> playerIconUI_;
	std::unique_ptr<UI::Game::PlayerExpGauge> expGauge_;
	std::unique_ptr<UI::Game::PlayerHealthGauge> healthGauge_;
	std::unique_ptr<UI::Game::WeaponIconUI> weaponIconUI_;
	UI::Game::UpgradeUI weaponUpgradeUI_;

	MadoEngine::TextHandle enemyCountText_{};
	MadoEngine::TextHandle moneyText_{};
	MadoEngine::TextHandle killCountText_{};
	UI::Game::FpsMeasurementView fpsMeasurementView_;
	UI::Game::GamePlayTimerView gamePlayTimerView_;
	UI::Game::ProjectileDamageView projectileDamageView_;
	int displayedMoney_ = -1;
	bool useDebugCamera_ = false;

	// System
	std::unique_ptr<System::InGameSession> inGameSession_;
};
