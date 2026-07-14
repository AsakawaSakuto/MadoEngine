#pragma once
#include ".SceneManager/IScene.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Map/Map.h"
#include "GameObject/Map/MapLimit.h"
#include "GameObject/Enemy/EnemySpawner.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponStatusEditor.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include "GameObject/Weapon/Projectile/ProjectileStatus.h"
#include "Utility/Light/LightManager.h"
#include "UI/Player/PlayerExpGauge.h"
#include "UI/Player/PlayerIconUI.h"
#include "UI/Player/PlayerHealthGauge.h"
#include "UI/Weapon/WeaponUpgradeUI.h"
#include "UI/Weapon/WeaponIconUI.h"
#include "System/InGameSession/InGameSession.h"

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

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得します。
	/// @return Playerのワールド座標です。
	Vector3 GetShadowFocusPosition() const override;

	/// @brief シャドウマップ確認用のPlayer描画座標を取得します。
	/// @param outPosition PlayerのModelワールド座標を受け取る変数です。
	/// @return Player座標を取得できた場合はtrueを返します。
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const override;
private:
	DebugCamera debugCamera_;
	TPS_Camera tpsCamera_;

	ColliderShape mapLimitBox_;
	Vector3 mapLimitBoxPos_;

	std::unique_ptr<Player::Base> player_;

	std::unique_ptr<Player::ExpGauge> expGauge_;
	std::unique_ptr<Player::HealthGauge> healthGauge_;

	std::unique_ptr<Map> map_;

	std::unique_ptr<EnemySpawner> enemySpawner_;

	std::unique_ptr<Player::PlayerIconUI> playerIconUI_;

	std::unique_ptr<Weapon::Inventory> weaponInventory_;
	std::unique_ptr<Weapon::StatusEditor> weaponStatusEditor_;
	std::unique_ptr<Weapon::UpgradeSystem> weaponUpgradeSystem_;
	Weapon::UpgradeUI weaponUpgradeUI_;
	std::unique_ptr<Weapon::WeaponIconUI> weaponIconUI_;

	Sprite* fadeSprite_ = nullptr;
	MadoEngine::Text* playerHealthText_ = nullptr;
	MadoEngine::Text* enemyCountText_ = nullptr;
	MadoEngine::Text* fpsText_ = nullptr;
	GameTimer fadeOutTimer_;
	float fpsSampleTime_ = 0.0f;
	int fpsSampleFrameCount_ = 0;
	bool useDebugCamera_ = false;

	// System
	std::unique_ptr<InGameSession> inGameSession_;
};