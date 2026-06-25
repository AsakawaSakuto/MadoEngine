#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title() {}

Title::~Title() {}

void Title::Initialize() {
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);

	wallPaperSprite_ = MySprite::Create("TitleWallPaper", "wallPaper", SceneType::Title);
	wallPaperSprite_->SetFitToScreen(true);
	fadeSprite_ = MySprite::Create("TitleFade", "black128x72", SceneType::Title);
	fadeSprite_->SetFitToScreen(true);
	fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
}

SceneType Title::Update(float dt) {
	// フェードイン処理
	fadeInTimer_.Update(dt);
	
	if (MyInput::Trigger("Decision")) {
		if (!fadeInTimer_.IsActive()) { fadeInTimer_.Start(1.0f); }
	}

	if (fadeInTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeInTimer_.GetProgress() });
	}
    
	if (fadeInTimer_.IsFinished()) {
		Logger::Output("Decisionが押されました - Testシーンへ遷移", Logger::Level::Application);
		return SceneType::Test;
	}
	return SceneType::Title;
}

void Title::Draw() {
	// タイトルシーンの描画処理
}

void Title::DrawImGui() {
	// タイトルシーンのImGui描画処理
}

void Title::Finalize() {
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
