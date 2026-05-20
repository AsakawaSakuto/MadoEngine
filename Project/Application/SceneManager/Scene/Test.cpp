#include "Test.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Test::Test()
	: testSprite_(nullptr)
	, testSprite2_(nullptr)
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	testSprite_ = MySprite::Create("testSprite", "uvChecker");
	testSprite2_ = MySprite::Create("testSprite2", "uvChecker");
	testSprite2_->SetPosition({ 100.0f, 100.0f });
}

std::string Test::Update() {
	// スペースキーが押されたらゲームシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		return "Game";
	}
	return "Test";
}

void Test::Finalize() {
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}

void Test::Draw() {
	// テストシーンの描画処理
}