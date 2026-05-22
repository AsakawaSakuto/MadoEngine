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
	}
}

std::string Test::Update() {
	// スペースキーが押されたらゲームシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		return "Game";
	}

	return "Test";
}

void Test::Draw() {
	
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI
	ImGui::Begin("test");

	ImGui::DragFloat2("pos", &testPos_.x, 0.1f);

	ImGui::End();

	sprites_[0]->SetPosition(testPos_);
#endif // USE_IMGUI
}

void Test::Finalize() {
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}