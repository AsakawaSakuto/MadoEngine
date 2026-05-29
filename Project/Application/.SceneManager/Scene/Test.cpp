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

	AABB s1;
	s1.min = { -2.5f, 0.0f, -2.5f };
	s1.max = { 2.5f, 2.5f, 2.5f };
	shape1_ = s1;
	shapePos1_ = { 5.0f, 0.0f, 5.0f };

	AABB s2;
	s2.min = { -2.5f, 0.0f, -2.5f };
	s2.max = { 2.5f, 2.5f, 2.5f };
	shape2_ = s2;
	shapePos2_ = { -5.0f, 0.0f, 5.0f };

	AABB s3;
	s3.min = { -2.5f, 0.0f, -2.5f };
	s3.max = { 2.5f, 2.5f, 2.5f };
	shape3_ = s3;
	shapePos3_ = { 5.0f, 0.0f, -5.0f };

	AABB s4;
	s4.min = { -2.5f, 0.0f, -2.5f };
	s4.max = { 2.5f, 2.5f, 2.5f };
	shape4_ = s4;
	shapePos4_ = { -5.0f, 0.0f, 0.0f };

	AABB s5;
	s5.min = { -2.5f, 0.0f, -2.5f };
	s5.max = { 2.5f, 2.5f, 2.5f };
	shape5_ = s5;
	shapePos5_ = { 5.0f, 0.0f, 0.0f };

	Plane p;
	p.normal = { 0.0f, 1.0f, 0.5f };
	p.size = 5.0f;
	plane_ = p;
	planePos_ = { 0.0f, 2.5f, -5.0f };

	// マネージャーへの登録（Shapeのアドレスと、独立したマスター座標のアドレスを渡す）
	MyCollider::RegisterCollider("TestSphere",  CollisionTag::AABB,   &shape1_, &shapePos1_, 2.0f);
	MyCollider::RegisterCollider("TestSphere2", CollisionTag::AABB,   &shape2_, &shapePos2_, 2.0f);
	MyCollider::RegisterCollider("TestSphere3", CollisionTag::AABB,   &shape3_, &shapePos3_, 2.0f);
	MyCollider::RegisterCollider("TestSphere4", CollisionTag::AABB,   &shape4_, &shapePos4_, 2.0f);
	MyCollider::RegisterCollider("TestSphere5", CollisionTag::AABB,   &shape5_, &shapePos5_, 2.0f);
	MyCollider::RegisterCollider("TestPlane",   CollisionTag::Plane,  &plane_,  &planePos_,  1.0f);

	MyCollider::RegisterCollisionPair(CollisionTag::AABB, CollisionTag::AABB, true);
	MyCollider::RegisterCollisionPair(CollisionTag::Player, CollisionTag::AABB, true);
	MyCollider::RegisterCollisionPair(CollisionTag::Player, CollisionTag::Plane, true);
}

SceneType Test::Update() {
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

	MyDebugLine::AddShape(std::get<AABB>(shape1_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(shape2_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(shape3_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(shape4_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(shape5_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<Plane>(plane_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });

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

	ImGui::DragFloat3("pos1", &shapePos1_.x, 0.1f);
	ImGui::DragFloat3("pos2", &shapePos2_.x, 0.1f);
	ImGui::DragFloat3("pos3", &shapePos3_.x, 0.1f);
	ImGui::DragFloat3("pos4", &shapePos4_.x, 0.1f);
	ImGui::DragFloat3("pos5", &shapePos5_.x, 0.1f);
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