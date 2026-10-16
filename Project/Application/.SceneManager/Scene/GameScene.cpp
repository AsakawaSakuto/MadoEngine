#include "GameScene.h"
#include "Audio/MyAudio.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Render/PostEffectManager.h"
#include "Utility/Logger/Logger.h"
#include "System/PlayLog/PlayLogJsonWriter.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace {
	constexpr float kGameSceneTimeLimit = 5.0f * 60.0f; // ゲームシーンの制限時間（秒）

	constexpr float kTpsCameraGamePadZoomSpeed = 15.0f;  // ゲームパッドの右スティックY軸でのズーム速度
	constexpr float kTpsCameraMouseWheelZoomStep = 2.0f; // マウスホイール1回転でのズーム距離変化量
	constexpr float kTpsCameraMinDistance = 5.0f;        // TPS_Cameraの最短距離
	constexpr float kTpsCameraMaxDistance = 30.0f;       // TPS_Cameraの最長距離
	constexpr float kPlayerPointLightHeight = 32.0f;
	constexpr const char* kPlayerPointLightName = "PlayerPointLight";
	
	constexpr int kEnemyDamageSoundFirstIndex = 1;
	constexpr int kEnemyDamageSoundLastIndex = 5;
	constexpr const char* kEnemyDamageSoundKeyPrefix = "EnemyDamage";
	constexpr const char* kPlayerOutlinePassName = "PlayerOutline";
}

GameScene::GameScene(CommonData& commonData)
	: commonData_(commonData) {}

GameScene::~GameScene() {}

void GameScene::Initialize() {

	// Titleの選択状態を消費し、未選択時だけ新規抽選と履歴登録を実行
	gameSeed_ = commonData_.GetGameSeedSystem().BeginGame();
	MyRand::SetSeed(gameSeed_);
	playLogRecorder_.Initialize(gameSeed_);
	playLogSaveAttempted_ = false;
	
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
	enemyDamageView_.Initialize();
	enemyStatusEffectIconView_.Initialize();

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
	SynchronizePlayerPointLight();

	// 大量撃破時の生成負荷を読み込み時へ移してExp512枠とMoney128枠を事前確保
	DropObject::Manager::GetInstance().Initialize();

	enemyManager_ = std::make_unique<Enemy::Manager>();
	enemyManager_->Initialize(player_.get());
	enemySpawner_ = std::make_unique<Enemy::Spawner>();
	enemySpawner_->Initialize(player_.get(), enemyManager_.get(), SceneType::Game, kGameSceneTimeLimit);
	enemyEditor_ = std::make_unique<Enemy::Editor>();
	enemyEditor_->Initialize(enemySpawner_.get(), enemyManager_.get());
	SynchronizeMapDependentState();

	playerIconUI_ = std::make_unique<UI::Game::PlayerIconUI>();
	playerIconUI_->Initialize();

	weaponInventory_ = std::make_unique<Weapon::Inventory>();
	weaponInventory_->Initialize(Projectile::Type::FireBall, weaponSlotCount_);

	weaponStatusEditor_ = std::make_unique<Weapon::StatusEditor>();

	weaponUpgradeSystem_ = std::make_unique<Weapon::UpgradeSystem>();
	weaponUpgradeSystem_->Initialize(player_->GetLevel(), gameSeed_);
	relicSystem_ = std::make_unique<Relic::System>();
	if (!relicSystem_->Initialize(gameSeed_)) {
		Logger::Output("[Relic] レリックシステムの初期化に失敗", Logger::Level::Error);
	}
	relicEditor_.Initialize(*relicSystem_);
	appliedRelicRevision_ = relicSystem_->GetRevision();

	// 武器Level表示をUpgrade Cardより先に登録してHUDをCardの背面へ配置
	weaponIconUI_ = std::make_unique<UI::Game::WeaponIconUI>();
	weaponIconUI_->Initialize(weaponSlotCount_);
	weaponUpgradeUI_.Initialize();
	relicInventoryUI_.Initialize(*relicSystem_);
	relicAcquisitionUI_.Initialize();
	chestOpeningPresentation_.Initialize();

	// Game進行Phaseと制限時間を全Object初期化後に開始
	inGameSession_ = std::make_unique<System::InGameSession>();
	inGameSession_->Initialize(kGameSceneTimeLimit);
	ConsumePlayLogEvents();

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

SceneType GameScene::Update(float dt) {

	// 前FrameのEditor変更を時間加算前に配送して取得時刻と削除時刻を維持
	ConsumePlayLogEvents();
	inGameSession_->Update(dt);

	// Pauseと強化選択中はGameObjectへ渡す時間だけを停止
	const float deltaTime = [&]() {
		if (inGameSession_->IsPlaying()) {
			return (dt);
		} else {
			return (0.0f);
		}
	}();

	if (inGameSession_->IsPlaying()) {
		playLogRecorder_.AdvancePlayTime(deltaTime);
		player_->Update(deltaTime);
		enemySpawner_->Update(deltaTime);
		enemyManager_->Update(deltaTime);

		// Targetが存在しないFrameも常時展開型ProjectileをPlayerへ追従
		weaponInventory_->SynchronizePersistentProjectiles(player_->GetPosition());
		Projectile::Manager::GetInstance().Update(deltaTime);

		// 全GameObjectの移動結果へ衝突解決を適用してから接地と壁登り状態を確定
		MyCollider::Update();
		player_->ResolveAfterCollision();
		enemyManager_->ResolveAfterCollision();
		for (const Combat::DamageEvent& event :
			enemyManager_->ConsumeDamageEvents()) {

			// Gameplay乱数へ影響しない専用乱数系列でDamage SEを選択
			const std::string damageSoundKey =
				std::string(kEnemyDamageSoundKeyPrefix) +
				std::to_string(MyRand::GetInt(
					kEnemyDamageSoundFirstIndex,
					kEnemyDamageSoundLastIndex));
			if (MyAudio::IsLoaded(damageSoundKey)) {
				MyAudio::Play(damageSoundKey);
			}
			enemyDamageView_.Spawn(event);
			playLogRecorder_.RecordDamage(event);

			// 毒と火傷を武器戦績へ重複加算せず、直接攻撃だけを既存Weaponへ反映
			if (event.sourceType == Combat::DamageSourceType::Weapon) {
				weaponInventory_->RecordWeaponDamage(event.sourceWeaponId, event.appliedDamage, event.wasKilled);
			}
		}

		// 全レリック上限時は支払い前に宝箱操作を拒否して一個獲得の契約を維持
		map_->SetEventActionEnabled(
			MapEventAction::RequestRelicReward,
			relicSystem_->CanRequestReward());
		if (!player_->IsDead()) {

			// 致死Damageの成立後に新しいインタラクトを受け付けない順序保証
			map_->Update(*player_, deltaTime);
		}
		for (const MapEventRequest& request : map_->ConsumeEventRequests()) {

			// Mapを各Game Systemへ依存させずSceneでイベント要求を仲介
			switch (request.action) {
			case MapEventAction::SpawnBoss:
				enemyManager_->SpawnBoss(request.position, SceneType::Game);
				break;
			case MapEventAction::RequestRelicReward:

				// 報酬を保留して獲得UIの前に専用Modelの開封待ち表示を開始
				if (relicSystem_->RequestReward()) {
					if (const Relic::Definition* reward = relicSystem_->GetPendingReward()) {
						chestOpeningPresentation_.Show(reward->rarity);
					}
				}
				break;
			case MapEventAction::AttractAllExp:
				DropObject::Manager::GetInstance().AttractAllExp(player_->GetPosition());
				break;
			case MapEventAction::None:
				break;
			}
		}
		if (!player_->IsDead()) {

			// 死亡確定後の所持金や経験値の変化を防ぐためDrop回収を停止
			DropObject::Manager::GetInstance().Update(deltaTime, *player_);
		}

		// Playerの状態変更を描画処理へ依存させずSceneで獲得通知へ変換
		for (const Player::ResourceGainEvent& event :
			player_->ConsumeResourceGainEvents()) {
			playerResourceGainView_.Spawn(event.type, event.amount);
			if (event.type == Player::ResourceGainType::Money) {
				playLogRecorder_.RecordMoneyGain(event.amount);
			}
		}

		// 攻撃範囲内にEnemyが存在するFrameだけ最近傍を射撃Targetとして更新
		if (!player_->IsDead() && MyCollider::IsHitWithTag("PlayerAttackRangeSphere", CollisionTag::EnemyHitBox)) {
			Vector3 nearestEnemyPosition;
			if (enemyManager_->TryGetNearestEnemyPosition(nearestEnemyPosition)) {
				weaponInventory_->Update(deltaTime, player_->GetPosition(), nearestEnemyPosition);
			}
		}
	}

	// HPが0になったFrameでGame進行を固定し、死亡Animationだけを描画System側で継続
	if (player_->IsDead()) {
		inGameSession_->SetGameOver();
		chestOpeningPresentation_.Hide();
		FinishPlayLog(System::PlayEndReason::Death);
	}

	// Pause中も現在の判定形状を確認できるようEnemy Colliderを毎Frame登録
	enemyManager_->DrawDebugLine();
	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());
		const bool isCameraInputEnabled = inGameSession_->GetCurrentPhase() != InGamePhase::Paused &&
			!chestOpeningPresentation_.IsActive();
		tpsCamera->SetInputEnabled(isCameraInputEnabled);

		if (isCameraInputEnabled) {
			float distanceDelta = 0.0f;

			// 左右トリガーの差分で同時入力を相殺し、時間比例の距離変化へ変換
			if (tpsCamera->GetUseGamePadInput()) {
				MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
				if (gamePad && gamePad->IsConnected()) {
					const float zoomInput = gamePad->GetLeftTrigger() - gamePad->GetRightTrigger();
					distanceDelta += zoomInput * kTpsCameraGamePadZoomSpeed * deltaTime;
				}
			}

			// 前方ホイールで接近、後方ホイールで後退となる距離変化へ変換
			if (MadoEngine::InputDevice::Mouse* mouse = MyInput::GetMouse()) {
				distanceDelta -= mouse->GetWheelDelta() * kTpsCameraMouseWheelZoomStep;
			}

			if (distanceDelta != 0.0f) {
				const float nextDistance = tpsCamera->GetDistance() + distanceDelta;
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

	// Editorの報酬解除時にも宝箱表示を残さず通常の待機解除へ追従
	if (chestOpeningPresentation_.IsActive() && !relicSystem_->HasPendingReward()) {
		chestOpeningPresentation_.Hide();
	}

	// 獲得待ち中も進む時間を使用し、獲得確定後も開封フラッシュの透明化を継続
	const bool didFinishChestPresentation = chestOpeningPresentation_.Update(
		dt, inGameSession_->IsWaitingItemAcquisition());

	// Chestの表示開始Frameからフラッシュ終了までPlayerの輪郭が前面へ重なる描画を抑止
	SetPlayerOutlineSuppressed(chestOpeningPresentation_.IsActive() || chestOpeningPresentation_.IsFlashActive());

	// ImGuiのプレビュー経路も含め、宝箱の膨張演出が完了するまではカード表示を禁止
	relicAcquisitionUI_.SetDisplayEnabled(!chestOpeningPresentation_.IsActive());

	// 背景の暗転だけをフラッシュ終了まで待たせ、カードの表示開始時期を維持
	relicAcquisitionUI_.SetBackgroundEnabled(!chestOpeningPresentation_.IsFlashActive());
	if (inGameSession_->IsWaitingItemAcquisition()) {
		if (!chestOpeningPresentation_.IsActive()) {

			// 膨張終了FrameのGamePad Aで表示直後のカードまで閉じないよう決定入力を抑止
			relicAcquisitionUI_.Update(*relicSystem_, !didFinishChestPresentation);
		}
	} else if (inGameSession_->GetCurrentPhase() == InGamePhase::WaitingUpgrade) {
		weaponUpgradeUI_.Update(dt, *weaponUpgradeSystem_, *weaponInventory_, *player_);
	}
	ApplyRelicModifiersIfChanged();
	relicInventoryUI_.Update(*relicSystem_);
	ConsumePlayLogEvents();

	// レリック獲得を先に解決し、終了後に保留中のLevel Up選択へ遷移
	const bool isRelicRewardPending = relicSystem_->HasPendingReward();
	inGameSession_->SetItemAcquisitionActive(isRelicRewardPending);
	if (!isRelicRewardPending) {
		inGameSession_->SetUpgradeSelectionActive(weaponUpgradeSystem_->IsUpgrading());
	}

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
		const std::string displayText = std::format("{}", playLogRecorder_.GetData().totalCombat.killCount);
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
		cameraManager_.CutTo([&]() {
			if (useDebugCamera_) {
				return (debugCameraHandle_);
			} else {
				return (tpsCameraHandle_);
			}
		}());
	}

	// Camera切り替え後の描画姿勢へ追従し、表示開始Frameから宝箱を画面中央へ配置
	chestOpeningPresentation_.FollowCamera(cameraManager_.GetRenderCamera());
	map_->SetInteractionTextVisible(inGameSession_->IsPlaying());
	enemyDamageView_.SetVisible(inGameSession_->IsPlaying());
	enemyDamageView_.Update(deltaTime, cameraManager_.GetRenderCamera());
	enemyStatusEffectIconView_.SetVisible(inGameSession_->IsPlaying());
	enemyStatusEffectIconView_.Update(*enemyManager_, cameraManager_.GetRenderCamera());
	playerResourceGainView_.SetVisible(inGameSession_->IsPlaying());
	playerResourceGainView_.Update(deltaTime);

	// Playerの移動と衝突解決後の最終座標へライトを追従
	SynchronizePlayerPointLight();

	if (player_->IsDeathAnimationFinished()) {
		return SceneType::Result;
	}

	return SceneType::Game;
}

void GameScene::Draw() {

}

void GameScene::DrawImGui() {
#ifdef USE_IMGUI

	// Game固有Systemの調整WindowをScene ManagerのDockSpaceへ集約
	player_->DrawImGui();
	playerResourceGainView_.DrawImGui();
	
	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	weaponUpgradeUI_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_);
	relicEditor_.DrawImGui(*relicSystem_);
	relicAcquisitionUI_.DrawImGui(*relicSystem_);

	enemyEditor_->DrawImGui();
	
	MyCollider::DrawImGui();

#endif // USE_IMGUI
}

void GameScene::DrawMapGeneratorImGui() {
	if (map_) {
		map_->DrawImGui(player_.get());
	}
}

std::string GameScene::CaptureMapGeneratorEditorState() const {
	if (map_) {
		return map_->CaptureEditorState();
	} else {
		return std::string{};
	}
}

void GameScene::RestoreMapGeneratorEditorState(const std::string& snapshot) {
	if (map_) {
		map_->RestoreEditorState(snapshot);
	}
}

bool GameScene::SaveEditorDocuments() const {
	const bool mapSucceeded = map_ && map_->SaveEditorSettings();
	const bool relicSucceeded = relicSystem_ && relicEditor_.Save(*relicSystem_);
	return mapSucceeded && relicSucceeded;
}

bool GameScene::ReloadEditorDocuments() {
	const bool mapSucceeded = map_ && map_->ReloadEditorSettings();
	const bool relicSucceeded = relicSystem_ && relicEditor_.Reload(*relicSystem_);
	return mapSucceeded && relicSucceeded;
}

bool GameScene::ApplyPendingEditorOperations() {
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

void GameScene::SynchronizeMapDependentState() {
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

void GameScene::ApplyRelicModifiersIfChanged() {
	if (!relicSystem_ || relicSystem_->GetRevision() == appliedRelicRevision_) {
		return;
	}

	const Relic::ModifierSnapshot& modifiers = relicSystem_->GetModifiers();
	Player::RuntimeModifiers playerModifiers;

	playerModifiers.maxHealthFlat                     = modifiers.GetFlat(Relic::Stat::MaxHealth);
	playerModifiers.movementSpeedMultiplier           = modifiers.Resolve(Relic::Stat::MovementSpeed, 1.0f);
	playerModifiers.moneyGainMultiplier               = modifiers.Resolve(Relic::Stat::MoneyGain, 1.0f);
	playerModifiers.expGainMultiplier                 = modifiers.Resolve(Relic::Stat::ExpGain, 1.0f);
	playerModifiers.healthRegenerationSpeedMultiplier = modifiers.Resolve(Relic::Stat::HealthRegenerationSpeed, 1.0f);

	const float additionalJumpCount = modifiers.Resolve(Relic::Stat::JumpCount, 0.0f);
	if (std::isfinite(additionalJumpCount)) {
		const float clampedCount = std::clamp(additionalJumpCount, 0.0f, static_cast<float>((std::numeric_limits<int>::max)()));
		playerModifiers.additionalJumpCount = static_cast<int>(std::lround(clampedCount));
	}

	player_->SetRuntimeModifiers(playerModifiers);

	Weapon::RuntimeModifiers weaponModifiers;
	weaponModifiers.damageMultiplier   = modifiers.Resolve(Relic::Stat::OutgoingDamage, 1.0f);
	weaponModifiers.criticalChanceFlat = modifiers.GetFlat(Relic::Stat::ProjectileCriticalChance);
	weaponModifiers.criticalChanceRate = modifiers.GetAdditiveRate(Relic::Stat::ProjectileCriticalChance);
	weaponModifiers.shotCooldownFlat   = modifiers.GetFlat(Relic::Stat::WeaponShotCooldown);
	weaponModifiers.shotCooldownRate   = modifiers.GetAdditiveRate(Relic::Stat::WeaponShotCooldown);

	const float additionalShotMaxCount = modifiers.Resolve(Relic::Stat::WeaponShotMaxCount, 0.0f);
	if (std::isfinite(additionalShotMaxCount)) {
		const float clampedCount = std::clamp(additionalShotMaxCount, 0.0f, static_cast<float>((std::numeric_limits<int>::max)()));
		weaponModifiers.additionalShotMaxCount = static_cast<int>(std::lround(clampedCount));
	}

	weaponInventory_->SetRuntimeModifiers(weaponModifiers);
	appliedRelicRevision_ = relicSystem_->GetRevision();
}

void GameScene::SynchronizePlayerPointLight() {
	if (!player_) {
		return;
	}

	LightManager& lightManager = LightManager::GetInstance();
	if (!lightManager.IsValid(playerPointLightHandle_)) {

		// Json再読み込み後のHandle再生成に追従できるよう名前から再解決
		playerPointLightHandle_ = lightManager.Find(kPlayerPointLightName);
	}

	PointLight* playerPointLight = lightManager.MutablePointLight(playerPointLightHandle_);
	if (!playerPointLight) {
		return;
	}

	const Vector3 playerPosition = player_->GetPosition();
	playerPointLight->position = {
		playerPosition.x,
		playerPosition.y + kPlayerPointLightHeight,
		playerPosition.z
	};
}

void GameScene::SetPlayerOutlineSuppressed(bool isSuppressed) {
	MadoEngine::Render::PostEffectManager& postEffectManager =
		MadoEngine::Render::PostEffectManager::GetInstance();

	// 対象Passだけを名前で特定し、Chest演出の有無で有効状態を直接切り替え
	for (MadoEngine::Render::PostEffectPassHandle handle : postEffectManager.GetLayerPassHandles()) {
		const MadoEngine::Render::PostEffectPass* pass = postEffectManager.TryGet(handle);
		if (pass && pass->GetName() == kPlayerOutlinePassName) {
			postEffectManager.SetEnabled(handle, !isSuppressed);
			return;
		}
	}
}

Vector3 GameScene::GetShadowFocusPosition() const {
	if (!player_) {
		return GetCamera().GetPosition();
	}

	return player_->GetPosition();
}

bool GameScene::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!player_) {
		outPosition = {};
		return false;
	}

	outPosition = player_->GetModelPosition();
	return true;
}

void GameScene::Finalize() {

	// 終了要求のない通常Shutdownにも対応し、Resource解放前に一回だけ保存
	if (!playLogRecorder_.IsFinished()) {
		FinishPlayLog(System::PlayEndReason::ApplicationExit);
	}
#ifdef ENABLE_PLAY_LOG_SAVE
	if (!playLogSaveAttempted_ && playLogRecorder_.IsFinished()) {
		playLogSaveAttempted_ = true;
		System::PlayLogJsonWriter writer;
		writer.Save(playLogRecorder_.GetData());
	}
#endif

	// 演出途中のScene遷移でもPlayerOutlineが無効のまま残らないよう有効化
	SetPlayerOutlineSuppressed(false);
	chestOpeningPresentation_.Finalize();
	relicAcquisitionUI_.Finalize();
	relicInventoryUI_.Finalize();
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

	DropObject::Manager::GetInstance().Finalize();
	Projectile::Manager::GetInstance().Clear();
	MyCollider::RemoveColliderAll();
	fpsMeasurementView_.Finalize();
	gamePlayTimerView_.Finalize();
	playerResourceGainView_.Finalize();
	enemyDamageView_.Finalize();
	enemyStatusEffectIconView_.Finalize();
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
	playerPointLightHandle_ = {};

	Logger::Output("ゲームシーンの終了処理を実行しました", Logger::Level::Application);
}

void GameScene::ConsumePlayLogEvents() {

	// UI経由の操作もゲーム本体の成功通知として同じ集計へ配送
	for (const Weapon::InventoryChangeEvent& event : weaponInventory_->ConsumeInventoryChangeEvents()) {
		playLogRecorder_.RecordWeaponChange(event);
	}
	for (const int paidMoney : weaponUpgradeSystem_->ConsumeRerollPayments()) {
		playLogRecorder_.RecordReroll(paidMoney);
	}
	for (const MapInteractionEvent& event : map_->ConsumeInteractionEvents()) {
		playLogRecorder_.RecordInteraction(event);
	}
}

void GameScene::FinishPlayLog(System::PlayEndReason reason) {
	if (playLogRecorder_.IsFinished()) {
		return;
	}

	// 死亡Frame内で成立した通知を回収してから所持品の最終状態を固定
	ConsumePlayLogEvents();
	for (std::size_t slot = 0; slot < static_cast<std::size_t>(weaponSlotCount_); ++slot) {
		if (const Weapon::BaseWeapon* weapon = weaponInventory_->GetWeaponAtSlot(slot)) {
			playLogRecorder_.UpdateWeaponLevel(weapon->GetWeaponId(), weapon->GetUpgradeLevel());
		}
	}
	std::vector<System::PlayRelicRecord> relics;
	for (const Relic::Definition& definition : relicSystem_->GetDefinitions()) {
		const int count = relicSystem_->GetInventory().GetCount(definition.id);
		if (count > 0) {
			relics.push_back({ definition.displayName, count, definition.rarity });
		}
	}
	playLogRecorder_.Finish(reason, player_->GetStatus().currentMoney, player_->GetKarmaCount(), std::move(relics));

	// 死亡以外の終了要求でも演出完了待ちの間にゲーム状態が変化しない停止状態へ移行
	inGameSession_->SetGameOver();
}

void GameScene::OnExitRequested(SceneType destination) {

	// 死亡による終了を後続のScene操作で別理由へ変更しない優先判定
	if (player_->IsDead()) {
		FinishPlayLog(System::PlayEndReason::Death);
		return;
	}
	switch (destination) {
	case SceneType::Game:
		FinishPlayLog(System::PlayEndReason::Restart);
		break;
	case SceneType::Title:
		FinishPlayLog(System::PlayEndReason::ReturnToTitle);
		break;
	case SceneType::None:
		FinishPlayLog(System::PlayEndReason::ApplicationExit);
		break;
	default:
		FinishPlayLog(System::PlayEndReason::SceneChange);
		break;
	}
}
