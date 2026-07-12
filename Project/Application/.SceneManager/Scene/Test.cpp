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
	constexpr const char* kUpgradeUpAction = "Up";
	constexpr const char* kUpgradeDownAction = "Down";
	constexpr const char* kUpgradeDecisionAction = "Decision";
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

#ifdef RELEASE
	if (enemyCountText_) {
		enemyCountText_->SetPosition({ 60.0f, 110.0f });
		enemyCountText_->SetFontSize(28.0f);
		enemyCountText_->SetAreaSize({ 360.0f, 40.0f });
		enemyCountText_->SetWordWrap(false);
	}
	if (fpsText_) {
		fpsText_->SetPosition({ 60.0f, 145.0f });
		fpsText_->SetFontSize(28.0f);
		fpsText_->SetAreaSize({ 240.0f, 40.0f });
		fpsText_->SetWordWrap(false);
	}
#endif // RELEASE

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
	selectedUpgradeChoiceIndex_ = 0;
	selectedUpgradeGeneration_ = 0;

	MyInput::RegisterInput(kUpgradeUpAction, { DIK_UP, DIK_W }, { GAMEPAD_UP });
	MyInput::RegisterInput(kUpgradeDownAction, { DIK_DOWN, DIK_S }, { GAMEPAD_DOWN });
	MyInput::RegisterInput(kUpgradeDecisionAction, { DIK_SPACE }, { GAMEPAD_A });

	fadeSprite_ = MySprite::Create("testFade", "black2x2", SceneType::Test);
	fadeSprite_->SetColor({1.0f,1.0f,1.0f,0.0f});
	fadeSprite_->SetFitToScreen(true);

	fadeOutTimer_.Start(2.0f);
	fpsSampleTime_ = 0.0f;
	fpsSampleFrameCount_ = 0;
}

SceneType Test::Update(float dt) {
	
	float deltaTime;

	if (weaponUpgradeSystem_->IsUpgrading()) {
		deltaTime = 0.0f;
	} else {
		deltaTime = dt;
	}

	MyDebugLine::AddShape(std::get<AABB>(mapLimitBox_), { 1.0f,1.0f,0.0f,1.0f });

	fadeOutTimer_.Update(dt);
	if (fadeOutTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOutTimer_.GetReverseProgress() });
	}

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(deltaTime);

	//if (MyInput::GetKeybord()->IsTrigger(DIK_0)) {
	//	tpsCamera_.Shake(0.3f, 3.5f, ShakeType::X);
	//}

	debugCamera_.Update(deltaTime);

	player_->Update(deltaTime);

	map_->Update(*player_);

	enemySpawner_->Update(deltaTime);

	Projectile::Manager::GetInstance().Update(deltaTime);

	DropObject::Manager::GetInstance().Update(deltaTime, *player_);

	// Mapとドロップ取得による経験値加算が完了してからレベル差分を確認します。
	weaponUpgradeSystem_->UpdatePlayerLevel(player_->GetLevel(), *weaponInventory_);
	UpdateWeaponUpgradeInput();

	weaponInventory_->Update(deltaTime, player_->GetPosition(), enemySpawner_->GetNearestEnemyPosition());

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

	if (MyInput::GetKeybord()->IsTrigger(DIK_9)) {
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

void Test::UpdateWeaponUpgradeInput() {
	if (!weaponUpgradeSystem_ || !weaponInventory_ || !weaponUpgradeSystem_->IsUpgrading()) {
		selectedUpgradeChoiceIndex_ = 0;
		selectedUpgradeGeneration_ = 0;
		return;
	}

	const std::vector<Weapon::UpgradeChoice>& choices = weaponUpgradeSystem_->GetChoices();
	if (choices.empty()) {
		selectedUpgradeChoiceIndex_ = 0;
		selectedUpgradeGeneration_ = 0;
		return;
	}

	const std::uint64_t currentGeneration = choices.front().generation;
	if (selectedUpgradeGeneration_ != currentGeneration) {
		selectedUpgradeChoiceIndex_ = 0;
		selectedUpgradeGeneration_ = currentGeneration;
	}

	if (selectedUpgradeChoiceIndex_ >= choices.size()) {
		selectedUpgradeChoiceIndex_ = 0;
	}

	if (MyInput::Trigger(kUpgradeUpAction)) {
		selectedUpgradeChoiceIndex_ =
			(selectedUpgradeChoiceIndex_ + choices.size() - 1) % choices.size();
	} else if (MyInput::Trigger(kUpgradeDownAction)) {
		selectedUpgradeChoiceIndex_ =
			(selectedUpgradeChoiceIndex_ + 1) % choices.size();
	}

	if (!MyInput::Trigger(kUpgradeDecisionAction)) {
		return;
	}

	const std::uint64_t generation = choices[selectedUpgradeChoiceIndex_].generation;
	if (weaponUpgradeSystem_->SelectChoice(
		selectedUpgradeChoiceIndex_,
		generation,
		*weaponInventory_)) {
		selectedUpgradeChoiceIndex_ = 0;
		selectedUpgradeGeneration_ = 0;
	}
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
	DrawWeaponUpgradeImGui();

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

void Test::DrawWeaponUpgradeImGui() {

#ifdef USE_IMGUI

	if (!weaponUpgradeSystem_ || !weaponInventory_) {
		return;
	}

	ImGui::Begin("武器アップグレード");
	ImGui::Text("未処理アップグレード: %d", weaponUpgradeSystem_->GetPendingUpgradeCount());
	ImGui::TextDisabled("↑ / W・↓ / S: 選択　Space / A: 決定");
	ImGui::Separator();

	const std::vector<Weapon::UpgradeChoice>& choices = weaponUpgradeSystem_->GetChoices();
	if (choices.empty()) {
		ImGui::TextDisabled(weaponUpgradeSystem_->IsUpgrading() ? "候補を生成できませんでした" : "レベルアップ待機中");
		ImGui::End();
		return;
	}

	for (std::size_t choiceIndex = 0; choiceIndex < choices.size(); ++choiceIndex) {
		const Weapon::UpgradeChoice& choice = choices[choiceIndex];
		ImGui::PushID(static_cast<int>(choiceIndex));
		if (choiceIndex == selectedUpgradeChoiceIndex_) {
			ImGui::TextColored(
				ImVec4(1.0f, 0.85f, 0.15f, 1.0f),
				"▶ 選択中 %zu: %s",
				choiceIndex + 1,
				choice.weaponDisplayName.c_str()
			);
		} else {
			ImGui::Text("候補 %zu: %s", choiceIndex + 1, choice.weaponDisplayName.c_str());
		}
		ImGui::Text("内容: %s", choice.choiceTypeDisplayName.c_str());

		if (choice.choiceType == Weapon::UpgradeChoiceType::OwnedWeaponUpgrade) {
			const auto& color = choice.rarityDisplayColor;
			ImGui::TextColored(ImVec4(color[0], color[1], color[2], color[3]),
				"レアリティ: %s", choice.rarityDisplayName.c_str());
			ImGui::Text("強化ステータス: %s", choice.statDisplayName.c_str());
			ImGui::Text("加算値: %+.3f", choice.calculatedAmount);
		} else {
			ImGui::TextDisabled("強化ステータス・レアリティなし");
		}

		const std::uint64_t generation = choice.generation;
		if (ImGui::Button("この候補を選択")) {
			selectedUpgradeChoiceIndex_ = choiceIndex;
			if (weaponUpgradeSystem_->SelectChoice(choiceIndex, generation, *weaponInventory_)) {
				selectedUpgradeChoiceIndex_ = 0;
				selectedUpgradeGeneration_ = 0;
			}
			ImGui::PopID();
			break;
		}

		ImGui::Separator();
		ImGui::PopID();
	}

	ImGui::End();

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
