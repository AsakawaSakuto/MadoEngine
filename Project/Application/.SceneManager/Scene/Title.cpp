#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title() {}

Title::~Title() {}

void Title::Initialize() {
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);

	fadeSprite_ = MySprite::Create("TitleFade", "black2x2", SceneType::Title);
	if (Sprite* fadeSprite = MySprite::TryGet(fadeSprite_)) {
		fadeSprite->SetColor({1.0f,1.0f,1.0f,0.0f});
		fadeSprite->SetFitToScreen(true);
	}

	debugCameraHandle_ = cameraManager_.CreateCamera<DebugCamera>("TitleDebugCamera");
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->SetDistance(35.0f);
	}
	cameraManager_.CutTo(debugCameraHandle_);
}

SceneType Title::Update(float dt) {

	// Scene遷移中だけ進行する白Fadeの更新
	fadeInTimer_.Update(dt);

	if (MyInput::Trigger("Decision")) {

		// 連続入力でFadeの進捗を再開始しない一度限りの遷移受付
		if (!fadeInTimer_.IsActive()) {
			fadeInTimer_.Start(1.0f);
		}
	}

	if (fadeInTimer_.IsActive()) {
		if (Sprite* fadeSprite = MySprite::TryGet(fadeSprite_)) {
			fadeSprite->SetColor({ 1.0f, 1.0f, 1.0f, fadeInTimer_.GetProgress() });
		}
	}
    
	if (fadeInTimer_.IsFinished()) {
		Logger::Output("Decisionが押されました - ゲームシーンへ遷移", Logger::Level::Application);
		return SceneType::Game;
	}

	cameraManager_.Update(dt);

	return SceneType::Title;
}

void Title::Draw() {
}

void Title::DrawImGui() {
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->DrawImGui();
	}
}

void Title::Finalize() {
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
