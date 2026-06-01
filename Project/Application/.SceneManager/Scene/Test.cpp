#include "Test.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Test::Test()
	
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	for (int i = 0; i < sprites_.size(); ++i) {
		sprites_[i] = MySprite::Create("testSprite" + std::to_string(i), "uvChecker", SceneType::Test);
		sprites_[i]->SetPosition({ 32.0f * i, 32.0f * i });
		sprites_[i]->SetVisible(false);
	}

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

	sprite_ = std::make_unique<Sprite>("a");

	player_ = std::make_unique<Player>();
	player_->Initialize();
	player_->SetCamera(&tpsCamera_);

	Plane p;
	p.normal = { 0.0f, 1.0f, 0.5f };
	p.size = 5.0f;
	plane_ = p;
	planePos_ = { 0.0f, 1.15f, -4.7f };

	MyCollider::RegisterCollider("TestPlane",   CollisionTag::Plane,  &plane_,  &planePos_,  1.0f);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerAABB, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerSphere, CollisionTag::MapBlock, true);


	map_ = std::make_unique<Map>();
	map_->Initialize();
}

SceneType Test::Update(float dt) {
	// スペースキーが押されたらゲームシーンに遷移
	/*if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		return SceneType::Game;
	}*/

	//debugCamera_.Update();

	auto deltaTime = 1.0f / 60.0f;

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(deltaTime);

	player_->Update(deltaTime);

	MyDebugLine::AddShape(std::get<Plane>(plane_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });

	map_->Update();

	return SceneType::Test;
}

void Test::Draw() {
	MyDebugLine::Draw(tpsCamera_);
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI
	ImGui::Begin("test");

	ImGui::DragFloat2("pos", &testPos_.x, 0.1f);

	ImGui::End();

	ImGui::Begin("testPos");

	ImGui::DragFloat3("planePos", &planePos_.x, 0.1f);

	ImGui::End();

	tpsCamera_.DrawImGui();

	sprites_[0]->SetPosition(testPos_);

	debugCamera_.DrawImGui();

#endif // USE_IMGUI
}

void Test::Finalize() {
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}