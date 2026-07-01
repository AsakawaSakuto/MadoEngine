#include "Test.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <cmath>
#include <format>

namespace {
	constexpr float kFpsTextUpdateInterval = 0.25f;
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
	if (enemyCountText_) {
		enemyCountText_->SetPosition({ 60.0f, 110.0f });
		enemyCountText_->SetFontSize(28.0f);
		enemyCountText_->SetAreaSize({ 360.0f, 40.0f });
		enemyCountText_->SetWordWrap(false);
	}

	fpsText_ = MyText::Create("FpsText", "FPS : 0.0", SceneType::Test, MadoEngine::Render::RenderLayer::UI);
	if (fpsText_) {
		fpsText_->SetPosition({ 60.0f, 145.0f });
		fpsText_->SetFontSize(28.0f);
		fpsText_->SetAreaSize({ 240.0f, 40.0f });
		fpsText_->SetWordWrap(false);
	}

	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapSlope, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::EnemyMovementSphere, true);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, true);

	map_ = std::make_unique<Map>();
	map_->Initialize(MyRand::CreateSeed());
	
	enemySpawner_ = std::make_unique<EnemySpawner>();
	enemySpawner_->Initialize(player_.get(), SceneType::Test);

	fadeSprite_ = MySprite::Create("testFade", "black2x2", SceneType::Test);
	fadeSprite_->SetColor({1.0f,1.0f,1.0f,0.0f});
	fadeSprite_->SetFitToScreen(true);

	fadeOutTimer_.Start(2.0f);
	fpsSampleTime_ = 0.0f;
	fpsSampleFrameCount_ = 0;
}

SceneType Test::Update(float dt) {
	
	fadeOutTimer_.Update(dt);
	if (fadeOutTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOutTimer_.GetReverseProgress() });
	}

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(dt);
	debugCamera_.Update(dt);

	player_->Update(dt);

	map_->Update(*player_);

	enemySpawner_->Update(dt);

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

	debugCamera_.DrawImGui();

	player_->DrawImGui();

	map_->DrawImGui();

	ImGui::Begin("EnemySpawner");
	ImGui::Text("Enemy Count : %zu", enemySpawner_->GetEnemyCount());
	ImGui::End();

	MyCollider::DrawImGui();

	ImGui::Begin("seed");

	ImGui::Text("map seed : %u", map_->GetSeed());

	ImGui::End();

	expGauge_->DrawImGui();
	healthGauge_->DrawImGui();

#endif // USE_IMGUI
}

void Test::Finalize() {
	
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
