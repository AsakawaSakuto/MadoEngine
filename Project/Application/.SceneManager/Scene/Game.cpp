#include "Game.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <format>

namespace {
	constexpr float kGameSceneTimeLimit = 5.0f * 60.0f;
	constexpr float kTpsCameraZoomSpeed = 15.0f;
	constexpr float kTpsCameraMinDistance = 5.0f;
	constexpr float kTpsCameraMaxDistance = 30.0f;
}

Game::Game(CommonData& commonData)
	: commonData_(commonData) {}

Game::~Game() {}

void Game::Initialize() {

	// Titleの選択状態を消費し、未選択時だけ新規抽選と履歴登録を実行
	gameSeed_ = commonData_.GetGameSeedSystem().BeginGame();
	MyRand::SetSeed(gameSeed_);

	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);
	Enemy::Settings::GetInstance().LoadOrCreate();

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

	staminaGauge_ = std::make_unique<UI::Game::PlayerStaminaGauge>();
	staminaGauge_->Initialize();

	enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Game, MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
	fpsMeasurementView_.Initialize();
	gamePlayTimerView_.Initialize();
	playerResourceGainView_.Initialize();
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
	enemySpawner_->Initialize(player_.get(), enemyManager_.get(), SceneType::Game, kGameSceneTimeLimit);
	enemyEditor_ = std::make_unique<Enemy::Editor>();
	enemyEditor_->Initialize(enemySpawner_.get(), enemyManager_.get());
	SynchronizeMapDependentState();

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

		// Targetが存在しないFrameも常時展開型ProjectileをPlayerへ追従
		weaponInventory_->SynchronizePersistentProjectiles(player_->GetPosition());
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

		map_->Update(*player_, deltaTime);
		for (const MapEventRequest& request : map_->ConsumeEventRequests()) {

			// Mapを各Game Systemへ依存させずSceneでイベント要求を仲介
			switch (request.action) {
			case MapEventAction::SpawnBoss:
				enemyManager_->SpawnBoss(request.position, SceneType::Game);
				break;
			case MapEventAction::RequestWeaponUpgrade:
				weaponUpgradeSystem_->RequestUpgrade(*weaponInventory_);
				break;
			case MapEventAction::None:
				break;
			}
		}
		DropObject::Manager::GetInstance().Update(deltaTime, *player_);

		// Playerの状態変更を描画処理へ依存させずSceneで獲得通知へ変換
		for (const Player::ResourceGainEvent& event :
			player_->ConsumeResourceGainEvents()) {
			playerResourceGainView_.Spawn(event.type, event.amount);
		}

		// 攻撃範囲内にEnemyが存在するFrameだけ最近傍を射撃Targetとして更新
		if (MyCollider::IsHitWithTag("PlayerAttackRangeSphere", CollisionTag::EnemyHitBox)) {
			Vector3 nearestEnemyPosition;
			if (enemyManager_->TryGetNearestEnemyPosition(nearestEnemyPosition)) {
				weaponInventory_->Update(deltaTime, player_->GetPosition(), nearestEnemyPosition);
			}
		}
	}

	// Pause中も現在の判定形状を確認できるようEnemy Colliderを毎Frame登録
	enemyManager_->DrawDebugLine();
	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());

		// 左右トリガーの差分で同時入力を相殺し、時間比例でカメラ距離を変更
		if (tpsCamera->GetUseGamePadInput()) {
			MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
			if (gamePad && gamePad->IsConnected()) {
				const float zoomInput = gamePad->GetLeftTrigger() - gamePad->GetRightTrigger();
				const float nextDistance = tpsCamera->GetDistance() + zoomInput * kTpsCameraZoomSpeed * deltaTime;
				tpsCamera->SetDistance(std::clamp(
					nextDistance,
					kTpsCameraMinDistance,
					kTpsCameraMaxDistance
				));
			}
		}
	}
	cameraManager_.Update(deltaTime);

	// MapとDrop取得による経験値加算が完了してからLevel差分を確認
	weaponUpgradeSystem_->UpdatePlayerLevel(player_->GetLevel(), *weaponInventory_);
	if (inGameSession_->GetCurrentPhase() == InGamePhase::WaitingUpgrade) {
		weaponUpgradeUI_.Update(dt, *weaponUpgradeSystem_, *weaponInventory_);
	}
	inGameSession_->SetUpgradeSelectionActive(weaponUpgradeSystem_->IsUpgrading());

	// 当Frameの攻撃Eventを一度だけ消費して対応SlotのUI演出へ変換
	for (const Weapon::WeaponAttackEvent& event :
		weaponInventory_->ConsumeWeaponAttackEvents()) {
		weaponIconUI_->PlayAttackAnimation(event.slotIndex);
	}
	weaponIconUI_->Update(deltaTime, *weaponInventory_);

	auto status = player_->GetStatus();
	expGauge_->Update(static_cast<float>(status.currentExp), static_cast<float>(status.expToNextLevel));
	expGauge_->IsUpgrade(inGameSession_->IsWaitingUpgradeSelection(), dt);

	healthGauge_->Update(
		player_->GetPosition(),
		static_cast<float>(status.currentHealth),
		static_cast<float>(status.maxHealth),
		cameraManager_.GetRenderCamera()
	);
	staminaGauge_->Update(
		player_->GetWallClimbRemainingTime(),
		player_->GetWallClimbMaxDuration(),
		player_->IsWallClimbGaugeVisible()
	);

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
	playerResourceGainView_.SetVisible(inGameSession_->IsPlaying());
	playerResourceGainView_.Update(deltaTime);

	return SceneType::Game;
}

void Game::Draw() {

}

void Game::DrawImGui() {
#ifdef USE_IMGUI

	// Game固有Systemの調整WindowをScene ManagerのDockSpaceへ集約
	player_->DrawImGui();
	playerResourceGainView_.DrawImGui();
	
	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	weaponUpgradeUI_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_);

	enemyEditor_->DrawImGui();
	
	MyCollider::DrawImGui();

#endif // USE_IMGUI
}

void Game::DrawMapGeneratorImGui() {
	if (map_) {
		map_->DrawImGui(player_.get());
	}
}

std::string Game::CaptureMapGeneratorEditorState() const {
	return map_ ? map_->CaptureEditorState() : std::string{};
}

void Game::RestoreMapGeneratorEditorState(const std::string& snapshot) {
	if (map_) {
		map_->RestoreEditorState(snapshot);
	}
}

bool Game::SaveEditorDocuments() const {
	return map_ && map_->SaveEditorSettings();
}

bool Game::ReloadEditorDocuments() {
	return map_ && map_->ReloadEditorSettings();
}

bool Game::ApplyPendingEditorOperations() {
	if (!map_ || !map_->ApplyPendingEditorGeneration()) {
		return false;
	}

	// 旧地形座標に依存する一時Objectを破棄して再開後の不正な衝突を防止
	if (enemySpawner_) {
		enemySpawner_->Clear();
	}
	if (enemyManager_) {
		enemyManager_->Clear();
	}
	DropObject::Manager::GetInstance().Clear();
	Projectile::Manager::GetInstance().Clear();

	gameSeed_ = map_->GetCurrentSeed();
	SynchronizeMapDependentState();
	if (player_) {
		player_->TeleportToGroundPosition(map_->CreatePlayerSpawnGroundPosition(gameSeed_));
	}
	const MadoEngine::TextHandle seedValueTextHandle = MyText::Find("SeedValueText");
	if (MadoEngine::Text* seedValueText = MyText::TryGet(seedValueTextHandle)) {
		seedValueText->SetText(std::format("Seed : {}", gameSeed_));
	}
	return true;
}

void Game::SynchronizeMapDependentState() {
	if (!map_) {
		return;
	}

	const MapLimit mapLimit = map_->CreateMapLimit();
	AABB mapLimitBox;
	mapLimitBox.min = mapLimit.min;
	mapLimitBox.max = mapLimit.max;
	mapLimitBox.center = {};
	mapLimitBox_ = mapLimitBox;
	mapLimitBoxPos_ = mapLimitBox.center;
	if (player_) {
		player_->SetMapLimit(mapLimit);
	}
	if (enemyManager_) {
		enemyManager_->SetMapLimit(mapLimit);
	}
	if (enemySpawner_) {
		enemySpawner_->SetMapLimit(mapLimit);
	}
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
	if (healthGauge_) {
		healthGauge_->Finalize();
	}
	if (staminaGauge_) {
		staminaGauge_->Finalize();
	}

	// Spawnerの参照先を破棄する前に生成予定とEnemy所有権を解放
	if (enemySpawner_) {
		enemySpawner_->Clear();
	}
	if (enemyManager_) {
		enemyManager_->Clear();
	}

	DropObject::Manager::GetInstance().Clear();
	Projectile::Manager::GetInstance().Clear();
	MyCollider::RemoveColliderAll();
	fpsMeasurementView_.Finalize();
	gamePlayTimerView_.Finalize();
	playerResourceGainView_.Finalize();
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
