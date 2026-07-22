#include "Game.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <format>

namespace {
	constexpr float kGameSceneTimeLimit = 10.0f * 60.0f;
	constexpr std::uint32_t kWeaponUpgradeRandomSeed = 0x4d41444fu;
}

Game::Game()

{}

Game::~Game() {}

void Game::Initialize() {
	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

	player_ = std::make_unique<Player::Base>();
	player_->Initialize();
	player_->SetCamera(&tpsCamera_);

	expGauge_ = std::make_unique<UI::ExpGauge>();
	expGauge_->Initialize();

	healthGauge_ = std::make_unique<UI::HealthGauge>();
	healthGauge_->Initialize();

	playerHealthText_ = MyText::Create("PlayerHealthText", "HP : 0 / 0", SceneType::Game, MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
	enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Game, MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
	fpsMeasurementView_.Initialize();
	gamePlayTimerView_.Initialize();

	AABB mapLimitBox;
	MapLimit mapLimit;
	mapLimitBox.min = mapLimit.min;
	mapLimitBox.max = mapLimit.max;
	mapLimitBox.center = { 0.0f,0.0f,0.0f };
	mapLimitBoxPos_ = mapLimitBox.center;
	mapLimitBox_ = mapLimitBox;
	MyCollider::RegisterCollider("MapLimitBox", CollisionTag::MapLimitBox, &mapLimitBox_, &mapLimitBoxPos_, 1.0f);
	
	map_ = std::make_unique<Map>();
	map_->Initialize(MyRand::CreateSeed());

	enemyManager_ = std::make_unique<Enemy::Manager>();
	enemyManager_->Initialize(player_.get());
	enemySpawner_ = std::make_unique<Enemy::Spawner>();
	enemySpawner_->Initialize(player_.get(), enemyManager_.get(), SceneType::Game);

	weaponInventory_ = std::make_unique<Weapon::Inventory>();
	weaponInventory_->Initialize(Projectile::Type::Pistol);
	weaponStatusEditor_ = std::make_unique<Weapon::StatusEditor>();
	weaponUpgradeSystem_ = std::make_unique<Weapon::UpgradeSystem>();
	weaponUpgradeSystem_->Initialize(player_->GetLevel(), kWeaponUpgradeRandomSeed);
	weaponUpgradeUI_.Initialize();

	fadeSprite_ = MySprite::Create("testFade", "black2x2", SceneType::Game);
	fadeSprite_->SetColor({ 1.0f,1.0f,1.0f,0.0f });
	fadeSprite_->SetFitToScreen(true);

	fadeOutTimer_.Start(2.0f);

	weaponIconUI_ = std::make_unique<UI::WeaponIconUI>();
	weaponIconUI_->Initialize(4);

	playerIconUI_ = std::make_unique<UI::PlayerIconUI>();
	playerIconUI_->Initialize();

	// System
	inGameSession_ = std::make_unique<System::InGameSession>();
	inGameSession_->Initialize(kGameSceneTimeLimit);

	MadoEngine::TextManager::GetInstance().LoadFromFile("Assets/Json/TextObjects.json");
}

SceneType Game::Update(float dt) {
	inGameSession_->Update(dt);
	const float deltaTime = inGameSession_->IsPlaying() ? dt : 0.0f;

	if (inGameSession_->IsPlaying()) {
		player_->Update(deltaTime);
		enemySpawner_->Update(deltaTime);
		enemyManager_->Update(deltaTime);
		Projectile::Manager::GetInstance().Update(deltaTime);

		// 全GameObjectの移動後にColliderを一度だけ更新してから衝突を解決する。
		MyCollider::Update();
		player_->ResolveAfterCollision();
		enemyManager_->ResolveAfterCollision();

		map_->Update(*player_);
		DropObject::Manager::GetInstance().Update(deltaTime, *player_);
		weaponInventory_->Update(deltaTime, player_->GetPosition(), enemyManager_->GetNearestEnemyPosition());
	}

	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });
	enemyManager_->DrawDebugLine();

	fadeOutTimer_.Update(deltaTime);
	if (fadeOutTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOutTimer_.GetReverseProgress() });
	}

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(deltaTime);

	debugCamera_.Update(deltaTime);

	// Mapとドロップ取得による経験値加算が完了してからレベル差分を確認する。
	weaponUpgradeSystem_->UpdatePlayerLevel(player_->GetLevel(), *weaponInventory_);
	if (inGameSession_->GetCurrentPhase() == InGamePhase::WaitingUpgrade) {
		weaponUpgradeUI_.UpdateInput(*weaponUpgradeSystem_, *weaponInventory_);
	}
	inGameSession_->SetUpgradeSelectionActive(weaponUpgradeSystem_->IsUpgrading());

	weaponIconUI_->Update(*weaponInventory_);

	auto status = player_->GetStatus();
	expGauge_->Update(static_cast<float>(status.currentExp), static_cast<float>(status.expToNextLevel));
	healthGauge_->Update(static_cast<float>(status.currentHealth), static_cast<float>(status.maxHealth));
	if (playerHealthText_) {
		playerHealthText_->SetText(std::format("HP : {} / {}", status.currentHealth, status.maxHealth));
	}
	if (enemyCountText_) {
		enemyCountText_->SetText(std::format("Enemy : {}", enemyManager_->GetEnemyCount()));
	}
	fpsMeasurementView_.Update(dt);
	gamePlayTimerView_.Update(inGameSession_->GetRemainingTime());

	if (inGameSession_->IsPlaying() && MyInput::GetKeybord()->IsTrigger(DIK_9)) {
		if (!weaponInventory_->AddWeapon(Projectile::Type::Pistol)) {
			Logger::Output("[Debug] デバッグ操作による武器追加は拒否されました。", Logger::Level::Debug);
		}
	}

	if (useDebugCamera_) {
		if (MyInput::GetKeybord()->IsTrigger(DIK_F9)) {
			useDebugCamera_ = false;
		}
		sceneCamera_ = debugCamera_;
	} else {
		if (MyInput::GetKeybord()->IsTrigger(DIK_F9)) {
			useDebugCamera_ = true;
		}
		sceneCamera_ = tpsCamera_;
	}

	return SceneType::Game;
}

void Game::Draw() {

}

void Game::DrawImGui() {
	// ゲームシーンの描画処理
#ifdef USE_IMGUI

	tpsCamera_.DrawImGui();

	//debugCamera_.DrawImGui();

	player_->DrawImGui();

	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	weaponUpgradeUI_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_);

	map_->DrawImGui();

	enemySpawner_->DrawImGui();

	MyCollider::DrawImGui();

	ImGui::Begin("seed");

	ImGui::Text("map seed : %u", map_->GetSeed());

	ImGui::End();

	//expGauge_->DrawImGui();
	//healthGauge_->DrawImGui();

#endif // USE_IMGUI
}

Vector3 Game::GetShadowFocusPosition() const {
	if (!player_) {
		return sceneCamera_.GetPosition();
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
	if (enemySpawner_) {
		enemySpawner_->Clear();
	}
	if (enemyManager_) {
		enemyManager_->Clear();
	}

	DropObject::Manager::GetInstance().Clear();
	MyCollider::RemoveColliderAll();
	MySprite::DestroyByScene(SceneType::Game);
	MyText::Destroy("PlayerHealthText");
	MyText::Destroy("EnemyCountText");
	fpsMeasurementView_.Finalize();
	gamePlayTimerView_.Finalize();
	MyModel::DestroyByScene(SceneType::Game);
	playerHealthText_ = nullptr;
	enemyCountText_ = nullptr;

	Logger::Output("ゲームシーンの終了処理を実行しました", Logger::Level::Application);
}
