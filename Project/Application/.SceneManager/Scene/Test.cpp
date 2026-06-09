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

	Slope slope;
	slope.center = { -20.0f, 0.0f, -10.0f };
	slope.min = { -5.0f, 0.0f, -5.0f };
	slope.max = { 5.0f, 5.0f, 5.0f };
	slope.direction = SlopeDirection::PulsX;
	slope_ = slope;

	Slope slope2;
	slope2.center = { -10.0f, 10.0f, -10.0f };
	slope2.min = { -5.0f, -5.0f, -5.0f };
	slope2.max = { 5.0f, 5.0f, 5.0f };
	slope2.direction = SlopeDirection::PulsX;
	slope2_ = slope2;

	AABB aabb;
	aabb.center = { -20.0f, 5.0f, -10.0f };
	aabb.min = { -25.0f, -5.0f, -25.0f };
	aabb.max = { 5.0f, 0.0f, 5.0f };
	aabb_ = aabb;

	MyCollider::RegisterCollider("TestPlane", CollisionTag::Plane, &plane_, &planePos_, 1.0f);
	MyCollider::RegisterCollider("TestSlope", CollisionTag::MapSlope, &slope_, &slopePos_, 1.0f);
	MyCollider::RegisterCollider("TestSlope2", CollisionTag::MapSlope, &slope2_, &slope2Pos_, 1.0f);
	MyCollider::RegisterCollider("AABB", CollisionTag::MapBlock, &aabb_, &aabbPos_, 1.0f);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerAABB, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerSphere, CollisionTag::MapSlope, true);

	map_ = std::make_unique<Map>();
	map_->Initialize();

	model_ = MyModel::Create("testModel", "cube", SceneType::Test);
}

SceneType Test::Update(float dt) {
	// スペースキーが押されたらゲームシーンに遷移
	/*if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		return SceneType::Game;
	}*/

	//debugCamera_.Update();

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(dt);

	player_->Update(dt);

	MyDebugLine::AddShape(std::get<Plane>(plane_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddShape(std::get<Slope>(slope_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddShape(std::get<Slope>(slope2_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(aabb_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });

	map_->Update();

	return SceneType::Test;
}

void Test::Draw() {
	MyDebugLine::Draw(tpsCamera_);
	MadoEngine::ModelManager::GetInstance().DrawAll(SceneType::Test, tpsCamera_);
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI

	std::get<Slope>(slope_).DrawImGui("slope");
	std::get<Slope>(slope2_).DrawImGui("slope2");
	std::get<AABB>(aabb_).DrawImGui("aabb");

	tpsCamera_.DrawImGui();

	debugCamera_.DrawImGui();

	player_->DrawImGui();

	map_->DrawImGui();

#endif // USE_IMGUI
}

void Test::Finalize() {
	MyCollider::RemoveCollider("TestPlane");
	MyCollider::RemoveCollider("TestSlope");
	MyCollider::RemoveCollider("TestSlope2");
	MyCollider::RemoveCollider("AABB");

	if (player_) {
		player_->Finalize();
		player_.reset();
	}

	if (map_) {
		map_->Finalize();
		map_.reset();
	}

	if (model_) {
		MyModel::Destroy("testModel");
		model_ = nullptr;
	}

	for (int i = 0; i < static_cast<int>(sprites_.size()); ++i) {
		if (sprites_[i]) {
			MySprite::Destroy("testSprite" + std::to_string(i));
			sprites_[i] = nullptr;
		}
	}

	sprite_.reset();

	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
