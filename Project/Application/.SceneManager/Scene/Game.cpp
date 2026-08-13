#include "Game.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <format>

namespace {
	constexpr float kGameSceneTimeLimit = 5.0f * 60.0f;
}

Game::Game(CommonData& commonData)
	: commonData_(commonData) {}

Game::~Game() {}

void Game::Initialize() {

	// Titleの選択状態を消費し、未選択時だけ新規抽選と履歴登録を実行
	gameSeed_ = commonData_.GetGameSeedSystem().BeginGame();
	MyRand::SetSeed(gameSeed_);

	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);

	debugCameraHandle_ = cameraManager_.CreateCamera<DebugCamera>("GameDebugCamera");
	tpsCameraHandle_ = cameraManager_.CreateCamera<TPS_Camera>("GamePlayerCamera");
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->SetPosition({ 0.0f, 10.0f, -20.0f });
	}
	cameraManager_.CutTo(tpsCameraHandle_);

	expGauge_ = std::make_unique<UI::Game::PlayerExpGauge>();
	expGauge_->Initialize();

	healthGauge_ = std::make_unique<UI::Game::PlayerHealthGauge>();
	healthGauge_->Initialize();

	enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Game, MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
	fpsMeasurementView_.Initialize();
	gamePlayTimerView_.Initialize();
	projectileDamageView_.Initialize();

	AABB mapLimitBox;
	MapLimit mapLimit;
	mapLimitBox.min = mapLimit.min;
	mapLimitBox.max = mapLimit.max;
	mapLimitBox.center = { 0.0f,0.0f,0.0f };
	mapLimitBoxPos_ = mapLimitBox.center;
	mapLimitBox_ = mapLimitBox;

	// ProjectileやEnemyの生存範囲を共通の包含Colliderとして登録
	MyCollider::RegisterCollider("MapLimitBox", CollisionTag::MapLimitBox, &mapLimitBox_, &mapLimitBoxPos_, 1.0f);
	
	map_ = std::make_unique<Map>();
	map_->Initialize(gameSeed_);

	// 地形確定後に通常Block上面の中心を取得してPlayerの初期配置へ使用
	player_ = std::make_unique<Player::Base>();
	player_->Initialize(map_->CreatePlayerSpawnGroundPosition(gameSeed_));
	player_->SetCamera(cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_));

	enemyManager_ = std::make_unique<Enemy::Manager>();
	enemyManager_->Initialize(player_.get());
	enemySpawner_ = std::make_unique<Enemy::Spawner>();
	enemySpawner_->Initialize(player_.get(), enemyManager_.get(), SceneType::Game);

	weaponIconUI_ = std::make_unique<UI::Game::WeaponIconUI>();
	weaponIconUI_->Initialize(4);

	playerIconUI_ = std::make_unique<UI::Game::PlayerIconUI>();
	playerIconUI_->Initialize();

	weaponInventory_ = std::make_unique<Weapon::Inventory>();
	weaponInventory_->Initialize(Projectile::Type::FireBall);
	weaponStatusEditor_ = std::make_unique<Weapon::StatusEditor>();
	weaponUpgradeSystem_ = std::make_unique<Weapon::UpgradeSystem>();
	weaponUpgradeSystem_->Initialize(player_->GetLevel(), gameSeed_);
	weaponUpgradeUI_.Initialize();

	fadeOutTimer_.Start(2.0f);

	// Game進行Phaseと制限時間を全Object初期化後に開始
	inGameSession_ = std::make_unique<System::InGameSession>();
	inGameSession_->Initialize(kGameSceneTimeLimit);

	auto textHandle = MyText::Find("Text");
	if (MadoEngine::Text* text = MyText::TryGet(textHandle)) {
		text->SetColor({ 1.0f,0.0f,0.0f,1.0f });
	}

	moneyText_ = MyText::Find("MoneyText");
	killCountText_ = MyText::Find("KillCountText");
	displayedMoney_ = -1;

	const MadoEngine::TextHandle seedValueTextHandle = MyText::Find("SeedValueText");
	if (MadoEngine::Text* seedValueText = MyText::TryGet(seedValueTextHandle)) {
		seedValueText->SetText(std::format("Seed : {}", gameSeed_));
	}
}

SceneType Game::Update(float dt) {
	inGameSession_->Update(dt);

	// Pauseと強化選択中はGameObjectへ渡す時間だけを停止
	const float deltaTime = inGameSession_->IsPlaying() ? dt : 0.0f;

	if (inGameSession_->IsPlaying()) {
		player_->Update(deltaTime);
		enemySpawner_->Update(deltaTime);
		enemyManager_->Update(deltaTime);
		Projectile::Manager::GetInstance().Update(deltaTime);

		// 全GameObjectの移動後にColliderを一度だけ更新してから衝突を解決
		MyCollider::Update();
		player_->ResolveAfterCollision();
		enemyManager_->ResolveAfterCollision();
		for (const Enemy::ProjectileDamageEvent& event :
			enemyManager_->ConsumeProjectileDamageEvents()) {
			projectileDamageView_.Spawn(event.damage, event.worldPosition);
			weaponInventory_->RecordProjectileDamage(event.sourceWeaponId, event.damage, event.wasKilled);
		}

		map_->Update(*player_);
		DropObject::Manager::GetInstance().Update(deltaTime, *player_);

		// 攻撃範囲内にEnemyが存在するFrameだけ最近傍を射撃Targetとして更新
		if (MyCollider::IsHitWithTag("PlayerAttackRangeSphere", CollisionTag::EnemyHitBox)) {
			Vector3 nearestEnemyPosition;
			if (enemyManager_->TryGetNearestEnemyPosition(nearestEnemyPosition)) {
				weaponInventory_->Update(deltaTime, player_->GetPosition(), nearestEnemyPosition);
			}
		}
	}

	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	fadeOutTimer_.Update(deltaTime);
	
	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());
	}
	cameraManager_.Update(deltaTime);

	// MapとDrop取得による経験値加算が完了してからLevel差分を確認
	weaponUpgradeSystem_->UpdatePlayerLevel(player_->GetLevel(), *weaponInventory_);
	if (inGameSession_->GetCurrentPhase() == InGamePhase::WaitingUpgrade) {
		weaponUpgradeUI_.Update(dt, *weaponUpgradeSystem_, *weaponInventory_);
	}
	inGameSession_->SetUpgradeSelectionActive(weaponUpgradeSystem_->IsUpgrading());

	// 当Frameの射撃Eventを一度だけ消費して対応SlotのUI演出へ変換
	for (const Weapon::WeaponFiredEvent& event :
		weaponInventory_->ConsumeWeaponFiredEvents()) {
		weaponIconUI_->PlayFireAnimation(event.slotIndex);
	}
	weaponIconUI_->Update(deltaTime, *weaponInventory_);

	auto status = player_->GetStatus();
	expGauge_->Update(static_cast<float>(status.currentExp), static_cast<float>(status.expToNextLevel));
	expGauge_->IsUpgrade(inGameSession_->IsWaitingUpgradeSelection(), dt);

	healthGauge_->Update(static_cast<float>(status.currentHealth), static_cast<float>(status.maxHealth));

	const int currentMoney = static_cast<int>(status.currentMoney);
	if (currentMoney != displayedMoney_) {

		// 所持金が変化したFrameだけText Handleを再解決して表示を更新
		MadoEngine::Text* moneyText = MyText::TryGet(moneyText_);
		if (!moneyText) {
			moneyText_ = MyText::Find("MoneyText");
			moneyText = MyText::TryGet(moneyText_);
		}
		if (moneyText) {
			moneyText->SetText(std::format("{}", currentMoney));
			displayedMoney_ = currentMoney;
		}
	}

	// 全Weaponの累計撃破数が変化したFrameだけText表示を更新
	MadoEngine::Text* killCountText = MyText::TryGet(killCountText_);
	if (!killCountText) {
		killCountText_ = MyText::Find("KillCountText");
		killCountText = MyText::TryGet(killCountText_);
	}
	if (killCountText) {
		const std::string displayText = std::format("{}", weaponInventory_->GetTotalKillCount());
		if (killCountText->GetText() != displayText) {
			killCountText->SetText(displayText);
		}
	}

	auto enemyCountHandle = MyText::Find("EnemyCountText");
	if (MadoEngine::Text* enemyCountText = MyText::TryGet(enemyCountHandle)) {
		enemyCountText->SetText(std::format("Enemy : {}", enemyManager_->GetEnemyCount()));
	}
	fpsMeasurementView_.Update(dt);
	gamePlayTimerView_.Update(inGameSession_->GetRemainingTime());

	if (inGameSession_->IsPlaying() && MyInput::GetKeybord()->IsTrigger(DIK_9)) {
		if (!weaponInventory_->AddWeapon(Projectile::Type::Pistol)) {
			Logger::Output("[Debug] デバッグ操作による武器追加は拒否されました。", Logger::Level::Debug);
		}
	}

	const CameraHandle activeCameraHandle = cameraManager_.GetActiveCameraHandle();
	if (activeCameraHandle == debugCameraHandle_) {
		useDebugCamera_ = true;
	} else if (activeCameraHandle == tpsCameraHandle_) {
		useDebugCamera_ = false;
	}

	if (MyInput::GetKeybord()->IsTrigger(DIK_F9)) {

		// EditorやJson復元で変更されたActive Cameraと同期した上で二つの操作Cameraを交互に選択
		useDebugCamera_ = !useDebugCamera_;
		cameraManager_.CutTo(useDebugCamera_ ? debugCameraHandle_ : tpsCameraHandle_);
	}
	projectileDamageView_.SetVisible(inGameSession_->IsPlaying());
	projectileDamageView_.Update(deltaTime, cameraManager_.GetRenderCamera());

	return SceneType::Game;
}

void Game::Draw() {

}

void Game::DrawImGui() {
#ifdef USE_IMGUI

	// Game固有Systemの調整WindowをScene ManagerのDockSpaceへ集約
	player_->DrawImGui();

	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	weaponUpgradeUI_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_);

	map_->DrawImGui();

	enemySpawner_->DrawImGui();
	projectileDamageView_.DrawImGui();

	MyCollider::DrawImGui();

	ImGui::Begin("seed");

	ImGui::Text("game seed : %u", gameSeed_);

	ImGui::End();

#endif // USE_IMGUI
}

Vector3 Game::GetShadowFocusPosition() const {
	if (!player_) {
		return GetCamera().GetPosition();
	}

	return player_->GetPosition();
}

bool Game::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!player_) {
		outPosition = {};
		return false;
	}

	outPosition = player_->GetModelPosition();
	return true;
}

void Game::Finalize() {
	weaponUpgradeUI_.Finalize();

	// Spawnerの参照先を破棄する前に生成予定とEnemy所有権を解放
	if (enemySpawner_) {
		enemySpawner_->Clear();
	}
	if (enemyManager_) {
		enemyManager_->Clear();
	}

	DropObject::Manager::GetInstance().Clear();
	MyCollider::RemoveColliderAll();
	fpsMeasurementView_.Finalize();
	gamePlayTimerView_.Finalize();
	projectileDamageView_.Finalize();
	enemyCountText_ = {};
	moneyText_ = {};
	killCountText_ = {};
	displayedMoney_ = -1;
	if (player_) {
		player_->SetCamera(nullptr);
	}
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	tpsCameraHandle_ = {};

	Logger::Output("ゲームシーンの終了処理を実行しました", Logger::Level::Application);
}
