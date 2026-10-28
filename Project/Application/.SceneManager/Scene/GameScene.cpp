#include "GameScene.h"
#include "Audio/MyAudio.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/PostEffectManager.h"
#include "Utility/Logger/Logger.h"
#include "System/PlayLog/PlayLogJsonWriter.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {
	constexpr float kGameSceneTimeLimit = 5.0f * 60.0f; // ゲームシーンの制限時間（秒）

	constexpr float kPlayerPointLightHeight = 32.0f;
	constexpr const char* kPlayerPointLightName = "PlayerPointLight";

	constexpr int kEnemyDamageSoundFirstIndex = 1;
	constexpr int kEnemyDamageSoundLastIndex = 5;
	constexpr const char* kEnemyDamageSoundKeyPrefix = "EnemyDamage";
	constexpr const char* kPlayerDamageSoundKey = "PlayerDamage";
	constexpr const char* kPlayerOutlinePassName = "PlayerOutline";
	constexpr const char* kLowHealthVignettePassName = "LowHealthVignette";
	constexpr float kLowHealthVignetteStartHealthRatio = 0.5f;
	constexpr float kLowHealthVignetteFullHealthRatio = 0.1f;
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
	isSceneRestartRequested_ = false;

	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);
	Enemy::Settings::GetInstance().LoadOrCreate();

	gameCameraController_.Initialize(cameraManager_);

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
	player_->Initialize(commonData_.GetParameterStore(), map_->CreatePlayerSpawnGroundPosition(gameSeed_));
	player_->SetCamera(gameCameraController_.TryGetPlayerCamera(cameraManager_));
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

	weaponInventory_ = std::make_unique<Weapon::Inventory>();

	// Titleで決定した武器を先頭Slotへ装備してゲームを開始
	weaponInventory_->Initialize(commonData_.GetInitialWeaponType(), weaponSlotCount_);

	weaponStatusEditor_ = std::make_unique<Weapon::StatusEditor>();

	weaponUpgradeSystem_ = std::make_unique<Weapon::UpgradeSystem>();
	weaponUpgradeSystem_->Initialize(player_->GetLevel(), gameSeed_);
	relicSystem_ = std::make_unique<Relic::System>();
	if (!relicSystem_->Initialize(gameSeed_)) {
		Logger::Output("[Relic] レリックシステムの初期化に失敗", Logger::Level::Error);
	}
	relicEditor_.Initialize(*relicSystem_);
	appliedRelicRevision_ = relicSystem_->GetRevision();

	// 常設HUDを先に生成し、報酬カードと暗幕を前面へ配置
	hud_.Initialize(weaponSlotCount_, *relicSystem_, map_->CreateMiniMapData(), gameSeed_);
	hud_.SetVisible(false);
	rewardSelectionController_.Initialize();

	// Game進行Phaseと制限時間を全Object初期化後に開始
	inGameSession_ = std::make_unique<System::InGameSession>();
	inGameSession_->Initialize(kGameSceneTimeLimit);
	ConsumePlayLogEvents();

	UpdateMiniMap();
	UpdateLowHealthVignette();
}

SceneType GameScene::Update(float dt) {

	// 前FrameのEditor変更を時間加算前に配送して取得時刻と削除時刻を維持
	ConsumePlayLogEvents();

	// シーン遷移の完了条件を渡し、開始待機と停止状態を反映したゲーム時間を取得
	const bool isSceneTransitioning = commonData_.GetSceneTransitionController().IsTransitioning();
	const bool wasStarting = inGameSession_->GetCurrentPhase() == InGamePhase::Starting;
	const bool wasPaused = inGameSession_->GetCurrentPhase() == InGamePhase::Paused;
	const System::PauseMenuItem previousPauseMenuItem = inGameSession_->GetSelectedPauseMenuItem();
	const float deltaTime = inGameSession_->Update(dt, !isSceneTransitioning);

	// 既存のメニュー効果音を選択変更と決定のFrameだけで再生
	if (wasPaused && previousPauseMenuItem != inGameSession_->GetSelectedPauseMenuItem() &&
		MyAudio::IsLoaded("UpgradeCardSelect")) {
		MyAudio::Play("UpgradeCardSelect");
	}
	const std::optional<System::PauseMenuItem> pauseMenuDecision = inGameSession_->ConsumePauseMenuDecision();
	if (pauseMenuDecision.has_value()) {
		if (MyAudio::IsLoaded("UpgradeCardDecision")) {
			MyAudio::Play("UpgradeCardDecision");
		}

		// 初期武器を維持し、新規Seedの抽選と進行状態の初期化をScene再生成時に実行
		if (pauseMenuDecision.value() == System::PauseMenuItem::Restart) {
			commonData_.GetGameSeedSystem().RequestSeed(std::nullopt);
			isSceneRestartRequested_ = true;
			return SceneType::Game;
		}
		if (pauseMenuDecision.value() == System::PauseMenuItem::Quit) {
			return SceneType::Title;
		}
	}
	if (wasStarting && inGameSession_->GetCurrentPhase() != InGamePhase::Starting) {

		// 開始待機が完了したFrameで常設HUDを再表示
		hud_.SetVisible(true);
	}

	// Sessionの状態に従い、開始待機と停止中のGameObject更新を抑止
	if (inGameSession_->IsPlaying() && !wasPaused) {
		UpdateGameplay(deltaTime);
	}

	// 死亡や選択待機でGameObject更新が停止しても被ダメージ色を実時間で通常色へ復帰
	player_->UpdateAppearance(dt);

	// HPが0になったFrameでGame進行を固定し、死亡Animationだけを描画System側で継続
	if (player_->IsDead()) {
		inGameSession_->SetGameOver();
		rewardSelectionController_.HideChest();
		FinishPlayLog(System::PlayEndReason::Death);
	}

	// Pause中も現在の判定形状を確認できるようEnemy Colliderを毎Frame登録
	enemyManager_->DrawDebugLine();
	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	// Camera更新前に被ダメージ演出を開始し、同じFrameの描画へ反映
	HandlePlayerDamage();

	const bool isCameraInputEnabled = !isSceneTransitioning &&
		inGameSession_->GetCurrentPhase() != InGamePhase::Starting &&
		inGameSession_->GetCurrentPhase() != InGamePhase::Paused &&
		!rewardSelectionController_.ShouldBlockCameraInput();
	gameCameraController_.UpdateTracking(cameraManager_, player_->GetPosition(), deltaTime, isCameraInputEnabled);

	// MapとDrop取得による経験値加算が完了してからLevel差分を確認
	weaponUpgradeSystem_->UpdatePlayerLevel(player_->GetLevel(), *weaponInventory_);

	// 選択中も実時間で演出を進行し、獲得結果の適用をPhase同期より先に確定
	rewardSelectionController_.Update(dt, inGameSession_->GetCurrentPhase(),
		*relicSystem_, *weaponUpgradeSystem_, *weaponInventory_, *player_);
	SetPlayerOutlineSuppressed(rewardSelectionController_.ShouldSuppressPlayerOutline());
	ApplyRelicModifiersIfChanged();
	UpdateLowHealthVignette();
	hud_.UpdateRelicInventory(*relicSystem_);
	ConsumePlayLogEvents();

	// レリック獲得を先に解決し、終了後に保留中のLevel Up選択へ遷移
	const bool isRelicRewardPending = relicSystem_->HasPendingReward();
	inGameSession_->SetItemAcquisitionActive(isRelicRewardPending);
	if (!isRelicRewardPending) {
		inGameSession_->SetUpgradeSelectionActive(weaponUpgradeSystem_->IsUpgrading());
	}

	// 選択画面やPause中はDropの移動と同じ条件でシート再生の時間進行も停止
	MadoEngine::ModelManager::GetInstance().SetSpriteSheetAnimationsPaused(SceneType::Game, !inGameSession_->IsPlaying());

	UpdateHud(dt, deltaTime);

	if (inGameSession_->GetCurrentPhase() != InGamePhase::Starting) {
		gameCameraController_.UpdateSwitchInput(cameraManager_);
	}

	// 衝突解決とCamera切り替え後の最終座標で、Pause中も地形の遮蔽表示を更新
	map_->UpdateCameraOcclusion(cameraManager_.GetRenderCamera().GetPosition(), player_->GetPosition());

	// Camera切り替え後の描画姿勢へ追従し、表示開始Frameから宝箱を画面中央へ配置
	rewardSelectionController_.FollowCamera(cameraManager_.GetRenderCamera());
	map_->SetInteractionTextVisible(inGameSession_->IsPlaying());
	hud_.UpdateWorldViews(deltaTime, inGameSession_->IsPlaying(),
		*enemyManager_, cameraManager_.GetRenderCamera());

	// Playerの移動と衝突解決後の最終座標へライトを追従
	SynchronizePlayerPointLight();

	if (player_->IsDeathAnimationFinished()) {
		return SceneType::Result;
	}

	return SceneType::Game;
}

void GameScene::UpdateGameplay(float deltaTime) {
	playLogRecorder_.AdvancePlayTime(deltaTime);
	player_->Update(deltaTime);
	enemySpawner_->Update(deltaTime);
	enemyManager_->Update(deltaTime);

	// Targetが存在しないFrameも常時展開型ProjectileをPlayerへ追従
	weaponInventory_->SynchronizePersistentProjectiles(player_->GetPosition(), player_->GetTemporaryAttackMultiplier());
	Projectile::Manager::GetInstance().Update(deltaTime, player_->GetPosition());

	// 全GameObjectの移動結果へ衝突解決を適用してから接地と壁登り状態を確定
	MyCollider::Update();
	player_->ResolveAfterCollision();

	// Playerの補間済み傾きを同じFrameのSword描画と命中判定へ反映
	Projectile::Manager::GetInstance().SynchronizeOwnerPose(player_->GetPosition(), player_->GetUpDirection());
	enemyManager_->ResolveAfterCollision();
	DispatchDamageEvents();

	// 全レリック上限時は支払い前に宝箱操作を拒否して一個獲得の契約を維持
	map_->SetEventActionEnabled(
		MapEventAction::RequestRelicReward,
		relicSystem_->CanRequestReward());
	if (!player_->IsDead()) {

		// 致死Damageの成立後に新しいインタラクトを受け付けない順序保証
		map_->Update(*player_, deltaTime);
	}
	DispatchMapEventRequests();
	if (!player_->IsDead()) {

		// 死亡確定後の所持金や経験値の変化を防ぐためDrop回収を停止
		DropObject::Manager::GetInstance().Update(deltaTime, *player_);
	}

	// HP満タン時のHeal取得も表示できるよう、回復量と独立した取得通知をHUDへ転送
	for (DropObject::Type type : DropObject::Manager::GetInstance().ConsumeSpecialCollectionEvents()) {
		hud_.ShowDropObjectGet(type);
	}

	// Playerの状態変更を描画処理へ依存させずSceneで獲得通知へ変換
	for (const Player::ResourceGainEvent& event :
		player_->ConsumeResourceGainEvents()) {
		hud_.ShowResourceGain(event);
		if (event.type == Player::ResourceGainType::Money) {
			playLogRecorder_.RecordMoneyGain(event.amount);
		}
	}

	// 周辺区画から攻撃候補を収集し、各武器の発射時に対象を抽選
	if (!player_->IsDead()) {
		std::vector<Vector3> targetPositions;
		enemyManager_->CollectNearestAttackTargetPositions(weaponInventory_->GetAttackTargetCandidateCount(), targetPositions);
		if (!targetPositions.empty()) {
			weaponInventory_->Update(deltaTime, player_->GetPosition(), targetPositions, player_->GetUpDirection(),
				player_->GetTemporaryAttackMultiplier());
		}
	}
}

void GameScene::HandlePlayerDamage() {

	// 同じFrameの複数被弾を合算した通知を一度だけ取得し、演出の重複開始を防止
	const float playerDamageAmount = player_->ConsumeDamageAmount();
	if (playerDamageAmount <= 0.0f) {
		return;
	}

	// 音声の未読み込み時もCameraとHUDの演出を継続
	if (MyAudio::IsLoaded(kPlayerDamageSoundKey)) {
		MyAudio::Play(kPlayerDamageSoundKey);
	}
	cameraManager_.ExecuteShake(cameraManager_.GetActiveCameraHandle());
	hud_.ShowPlayerDamage(playerDamageAmount);
}

void GameScene::DispatchDamageEvents() {
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
		hud_.ShowDamage(event);
		playLogRecorder_.RecordDamage(event);

		// 毒と火傷を武器戦績へ重複加算せず、直接攻撃だけを既存Weaponへ反映
		if (event.sourceType == Combat::DamageSourceType::Weapon) {
			weaponInventory_->RecordWeaponDamage(event.sourceWeaponId, event.appliedDamage, event.wasKilled);
		}
	}
}

void GameScene::DispatchMapEventRequests() {
	for (const MapEventRequest& request : map_->ConsumeEventRequests()) {

		// Mapを各Game Systemへ依存させずSceneでイベント要求を仲介
		switch (request.action) {
		case MapEventAction::SpawnBoss:
			enemyManager_->SpawnBoss(request.position, SceneType::Game);
			break;
		case MapEventAction::RequestRelicReward:
			rewardSelectionController_.RequestRelicReward(*relicSystem_);
			break;
		case MapEventAction::AttractAllExp:
			DropObject::Manager::GetInstance().AttractAllExp(player_->GetPosition());
			break;
		case MapEventAction::RequestWeaponUpgrade:

			// Anvilの利用を既存の選択待機へ接続し、所持金を消費せず強化を一回追加
			weaponUpgradeSystem_->RequestUpgrade(*weaponInventory_);
			break;
		case MapEventAction::None:
			break;
		}
	}
}

void GameScene::UpdateHud(float dt, float deltaTime) {

	// 通知はSceneで一度だけ消費し、ゲームSystemと表示の所有権を分離
	for (const Weapon::WeaponAttackEvent& event : weaponInventory_->ConsumeWeaponAttackEvents()) {
		hud_.PlayWeaponAttackAnimation(event.slotIndex);
	}

	UI::Game::GameHudState state;
	state.playerStatus = player_->GetStatus();
	state.playerPosition = player_->GetPosition();
	state.wallClimbRemainingTime = player_->GetWallClimbRemainingTime();
	state.wallClimbMaxDuration = player_->GetWallClimbMaxDuration();
	state.remainingTime = inGameSession_->GetRemainingTime();
	state.gameStartProgress = inGameSession_->GetStartDelayProgress();
	state.gamePlayElapsedTime = inGameSession_->GetExecutionTime();
	state.killCount = playLogRecorder_.GetData().totalCombat.killCount;
	state.enemyCount = enemyManager_->GetEnemyCount();
	state.upgradeRerollCost = weaponUpgradeSystem_->GetRerollCost();
	state.isWallClimbGaugeVisible = player_->IsWallClimbGaugeVisible();
	state.isWaitingUpgradeSelection = inGameSession_->IsWaitingUpgradeSelection();
	state.isPaused = inGameSession_->GetCurrentPhase() == InGamePhase::Paused;
	state.pauseMenuSelectedIndex = static_cast<std::size_t>(inGameSession_->GetSelectedPauseMenuItem());
	hud_.Update(dt, deltaTime, state, *weaponInventory_, cameraManager_.GetRenderCamera());
	UpdateMiniMap();
}

void GameScene::Draw() {

}

void GameScene::DrawImGui() {
#ifdef USE_IMGUI

	// Game固有Systemの調整WindowをScene ManagerのDockSpaceへ集約
	player_->DrawImGui();
	hud_.DrawImGui();

	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	relicEditor_.DrawImGui(*relicSystem_);
	rewardSelectionController_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_, *relicSystem_);

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

	// GPU待機済みのFrame末尾で地形タイルを再構築し、再配置したPlayer位置も即時同期
	hud_.RebuildMiniMap(map_->CreateMiniMapData());
	UpdateMiniMap();
	hud_.SetSeed(gameSeed_);
	return true;
}

void GameScene::UpdateMiniMap() {
	if (!map_ || !player_) {
		return;
	}

	// 強化選択や宝箱演出の間も現在地を確認できるよう表示を維持
	hud_.UpdateMiniMap(*map_, player_->GetPosition(), player_->GetFacingYaw());
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
	playerModifiers.incomingDamageMultiplier          = modifiers.Resolve(Relic::Stat::IncomingDamage, 1.0f);

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

void GameScene::UpdateLowHealthVignette() {
	float intensity = 0.0f;
	if (player_) {
		const Player::Status status = player_->GetStatus();

		// 不正なHP値と最大HPが0以下の場合は除算を避けて演出を解除
		if (std::isfinite(status.currentHealth) && std::isfinite(status.maxHealth) && status.maxHealth > 0.0f) {
			const float healthRatio = status.currentHealth / status.maxHealth;

			// HP50%から10%までを強度0から1へ線形変換し、範囲外は両端の強度へ固定
			intensity = std::clamp(
				(kLowHealthVignetteStartHealthRatio - healthRatio) /
				(kLowHealthVignetteStartHealthRatio - kLowHealthVignetteFullHealthRatio),
				0.0f, 1.0f);
		}
	}
	SetLowHealthVignetteIntensity(intensity);
}

void GameScene::SetLowHealthVignetteIntensity(float intensity) {
	MadoEngine::Render::PostEffectManager& postEffectManager =
		MadoEngine::Render::PostEffectManager::GetInstance();

	// Editorの再生成や内部キーの変更へ追従し、対象Passの強度だけを上書き
	for (MadoEngine::Render::PostEffectPassHandle handle : postEffectManager.GetScreenPassHandles()) {
		const MadoEngine::Render::PostEffectPass* pass = postEffectManager.TryGet(handle);
		if (pass && pass->GetName() == kLowHealthVignettePassName) {
			postEffectManager.SetFloatParameter(handle, "Intensity", intensity);
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

bool GameScene::ShouldPauseBgm() const {
	InGamePhase phase = InGamePhase::Starting;
	if (inGameSession_) {
		phase = inGameSession_->GetCurrentPhase();
	}
	return rewardSelectionController_.ShouldPauseBgm(phase);
}

int GameScene::GetSelectionBgmPlaybackId() const {
	if (!inGameSession_ || !relicSystem_ || !weaponUpgradeSystem_) {
		return 0;
	}
	return rewardSelectionController_.GetSelectionBgmPlaybackId(
		inGameSession_->GetCurrentPhase(), *relicSystem_, *weaponUpgradeSystem_);
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

	// 共通のフルスクリーンPassへ低体力時の強度が残らないようScene終了時に解除
	SetLowHealthVignetteIntensity(0.0f);
	rewardSelectionController_.Finalize();
	hud_.Finalize();

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
	if (player_) {
		player_->SetCamera(nullptr);
	}
	cameraManager_.Clear();
	gameCameraController_.Finalize();
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
