#include "Test.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <cmath>
#include <format>

namespace {
	constexpr float kFpsTextUpdateInterval = 0.25f;
	constexpr std::uint32_t kWeaponUpgradeRandomSeed = 0x4d41444fu;
}

Test::Test()
	
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

	player_ = std::make_unique<Player::Base>();
	player_->Initialize();
	player_->SetCamera(&tpsCamera_);

	expGauge_ = std::make_unique<Player::ExpGauge>();
	expGauge_->Initialize();

	healthGauge_ = std::make_unique<Player::HealthGauge>();
	healthGauge_->Initialize();

	playerHealthText_ = MyText::Create("PlayerHealthText", "HP : 0 / 0", SceneType::Test, MadoEngine::Render::RenderLayer::UI);
	enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Test, MadoEngine::Render::RenderLayer::UI);
	fpsText_ = MyText::Create("FpsText", "FPS : 0.0", SceneType::Test, MadoEngine::Render::RenderLayer::UI);

	AABB mapLimitBox;
	MapLimit mapLimit;
	mapLimitBox.min = mapLimit.min;
	mapLimitBox.max = mapLimit.max;
	mapLimitBox.center = { 0.0f,0.0f,0.0f };
	mapLimitBoxPos_ = mapLimitBox.center;
	mapLimitBox_ = mapLimitBox;
	MyCollider::RegisterCollider("MapLimitBox", CollisionTag::MapLimitBox, &mapLimitBox_, &mapLimitBoxPos_, 1.0f);
	MyCollider::RegisterCollisionPair(CollisionTag::MapLimitBox, CollisionTag::PlayerProjectileHitBox, false);

	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapSlope, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::EnemyMovementSphere, true);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, true);

	MyCollider::RegisterCollisionPair(CollisionTag::EnemyHitBox, CollisionTag::PlayerProjectileHitBox, false);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerDropObjectGetSphere, CollisionTag::DropObjectHitBox, false);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerHitBox, CollisionTag::DropObjectHitBox, false);

	map_ = std::make_unique<Map>();
	map_->Initialize(MyRand::CreateSeed());
	
	enemySpawner_ = std::make_unique<EnemySpawner>();
	enemySpawner_->Initialize(player_.get(), SceneType::Test);

	weaponInventory_ = std::make_unique<Weapon::Inventory>();
	weaponInventory_->Initialize(Projectile::Type::Pistol);
	weaponStatusEditor_ = std::make_unique<Weapon::StatusEditor>();
	weaponUpgradeSystem_ = std::make_unique<Weapon::UpgradeSystem>();
	weaponUpgradeSystem_->Initialize(player_->GetLevel(), kWeaponUpgradeRandomSeed);
	weaponUpgradeUI_.Initialize();

	fadeSprite_ = MySprite::Create("testFade", "black2x2", SceneType::Test);
	fadeSprite_->SetColor({1.0f,1.0f,1.0f,0.0f});
	fadeSprite_->SetFitToScreen(true);

	fadeOutTimer_.Start(2.0f);
	fpsSampleTime_ = 0.0f;
	fpsSampleFrameCount_ = 0;

	weaponIconUI_ = std::make_unique<Weapon::WeaponIconUI>();
	weaponIconUI_->Initialize(4);

	playerIconUI_ = std::make_unique<Player::PlayerIconUI>();
	playerIconUI_->Initialize();

	// System
	inGameSession_ = std::make_unique<InGameSession>();
	inGameSession_->Initialize();

	MadoEngine::TextManager::GetInstance().LoadFromFile("Assets/Json/TextObjects.json");
}

SceneType Test::Update(float dt) {
	inGameSession_->Update(dt);
	const float deltaTime = inGameSession_->IsPlaying() ? dt : 0.0f;

	if (inGameSession_->IsPlaying()) {
		player_->Update(deltaTime);
	}

	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	fadeOutTimer_.Update(deltaTime);
	if (fadeOutTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOutTimer_.GetReverseProgress() });
	}

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(deltaTime);

	debugCamera_.Update(deltaTime);

	if (inGameSession_->IsPlaying()) {
		map_->Update(*player_);
		enemySpawner_->Update(deltaTime);
		Projectile::Manager::GetInstance().Update(deltaTime);
		DropObject::Manager::GetInstance().Update(deltaTime, *player_);
		weaponInventory_->Update(deltaTime, player_->GetPosition(), enemySpawner_->GetNearestEnemyPosition());
	}

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
		enemyCountText_->SetText(std::format("Enemy : {}", enemySpawner_->GetEnemyCount()));
	}
	if (fpsText_) {
		fpsSampleTime_ += dt;
		++fpsSampleFrameCount_;
		if (fpsSampleTime_ >= kFpsTextUpdateInterval) {
			const float fps = fpsSampleTime_ > 0.0f ? static_cast<float>(fpsSampleFrameCount_) / fpsSampleTime_ : 0.0f;
			fpsText_->SetText(std::format("FPS : {:.1f}", fps));
			fpsSampleTime_ = 0.0f;
			fpsSampleFrameCount_ = 0;
		}
	}

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

	return SceneType::Test;
}

void Test::Draw() {
	
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI

	tpsCamera_.DrawImGui();

	//debugCamera_.DrawImGui();

	player_->DrawImGui();

	weaponInventory_->DrawImGui();
	weaponStatusEditor_->DrawImGui();
	weaponUpgradeUI_.DrawImGui(*weaponUpgradeSystem_, *weaponInventory_);

	map_->DrawImGui();

	ImGui::Begin("EnemySpawner");
	ImGui::Text("Enemy Count : %zu", enemySpawner_->GetEnemyCount());
	ImGui::End();

	MyCollider::DrawImGui();

	ImGui::Begin("seed");

	ImGui::Text("map seed : %u", map_->GetSeed());

	ImGui::End();

	//expGauge_->DrawImGui();
	//healthGauge_->DrawImGui();

#endif // USE_IMGUI
}

Vector3 Test::GetShadowFocusPosition() const {
	if (!player_) {
		return sceneCamera_.GetPosition();
	}

	return player_->GetPosition();
}

bool Test::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!player_) {
		outPosition = {};
		return false;
	}

	outPosition = player_->GetModelPosition();
	return true;
}

void Test::Finalize() {
	
	DropObject::Manager::GetInstance().Clear();
	MyCollider::RemoveColliderAll();
	MySprite::DestroyByScene(SceneType::Test);
	MyText::Destroy("PlayerHealthText");
	MyText::Destroy("EnemyCountText");
	MyText::Destroy("FpsText");
	MyModel::DestroyByScene(SceneType::Test);
	playerHealthText_ = nullptr;
	enemyCountText_ = nullptr;
	fpsText_ = nullptr;

	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
