#include "Test.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Test::Test()
	
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	for (int i = 0; i < sprites_.size(); ++i) {
		sprites_[i] = MySprite::Create("testSprite" + std::to_string(i), "uvChecker", "Test");
		sprites_[i]->SetPosition({ 32.0f * i, 32.0f * i });
		sprites_[i]->SetVisible(false);
	}

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

	sprite_ = std::make_unique<Sprite>("a");

	player_ = std::make_unique<Player>();
	player_->Initialize();

	AABB s1;
	s1.min = { -0.5f, -0.5f, -0.5f };
	s1.max = { 0.5f, 0.5f, 0.5f };
	testShape1_ = s1;

	// 2つ目のSphere
	AABB s2;
	s2.min = { -0.5f, -0.5f, -0.5f };
	s2.max = { 0.5f, 0.5f, 0.5f };
	testShape2_ = s2;

	Plane p;
	p.normal = { 0.0f, 1.0f, 0.5f };
	p.size = 10.0f;
	plane_ = p;

	// マネージャーへの登録（Shapeのアドレスと、独立したマスター座標のアドレスを渡す）
	MyCollider::RegisterCollider("TestSphere", "Sphere", &testShape1_, &testPos1_);
	MyCollider::RegisterCollider("TestSphere2", "Sphere", &testShape2_, &testPos2_);
	MyCollider::RegisterCollider("TestPlane", "Plane", &plane_, &testPos3_);

	MyCollider::RegisterCollisionPair("Sphere", "Sphere", true);
	MyCollider::RegisterCollisionPair("Sphere", "Plane", true);
}

std::string Test::Update() {
	// スペースキーが押されたらゲームシーンに遷移
	//if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
	//	Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
	//	return "Game";
	//}

	debugCamera_.Update();

	MyDebugLine::AddShape(std::get<AABB>(testShape1_), { 0.0f, 0.0f, 1.0f, 1.0f });
	MyDebugLine::AddShape(std::get<AABB>(testShape2_), { 0.0f, 1.0f, 0.0f, 1.0f });
	MyDebugLine::AddShape(std::get<Plane>(plane_), { 1.0f, 1.0f, 1.0f, 1.0f });
	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });

	player_->Update();

	return "Test";
}

void Test::Draw() {
	MyDebugLine::Draw(debugCamera_);
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI
	ImGui::Begin("test");

	ImGui::DragFloat2("pos", &testPos_.x, 0.1f);

	ImGui::End();

	ImGui::Begin("testPos");

	ImGui::DragFloat3("pos1", &testPos1_.x, 0.1f);
	ImGui::DragFloat3("pos2", &testPos2_.x, 0.1f);
	ImGui::DragFloat3("pos3", &testPos3_.x, 0.1f);

	ImGui::End();

	sprites_[0]->SetPosition(testPos_);

	debugCamera_.DrawImGui();

#endif // USE_IMGUI
}

void Test::Finalize() {
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}