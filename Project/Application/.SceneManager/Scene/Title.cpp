#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title(CommonData& commonData)
	: commonData_(commonData) {}

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
			const std::vector<System::GameSeedSystem::HistoryEntry>& history =
				commonData_.GetGameSeedSystem().GetHistory();
			std::optional<std::uint32_t> requestedSeed;
			if (selectedSeedIndex_.has_value() && selectedSeedIndex_.value() < history.size()) {
				requestedSeed = history[selectedSeedIndex_.value()].seed;
			}

			// Fade中の選択変更に影響されないよう遷移受付時のSeed要求を固定
			commonData_.GetGameSeedSystem().RequestSeed(requestedSeed);
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
#ifdef USE_IMGUI
	System::GameSeedSystem& gameSeedSystem = commonData_.GetGameSeedSystem();
	const std::vector<System::GameSeedSystem::HistoryEntry>& history = gameSeedSystem.GetHistory();

	ImGui::Begin("Seed History");
	ImGui::TextDisabled("No selection: generate a new Seed");
	ImGui::Separator();

	if (history.empty()) {
		ImGui::TextDisabled("No Seed history");
	} else {
		const bool isSelectionLocked = fadeInTimer_.IsActive();
		ImGui::BeginDisabled(isSelectionLocked);

		// 履歴の保存順は維持したままUIでは新しいSeedから表示
		for (std::size_t displayIndex = 0; displayIndex < history.size(); ++displayIndex) {
			const std::size_t historyIndex = history.size() - displayIndex - 1;
			bool isSelected = selectedSeedIndex_ == historyIndex;
			bool isFavorite = history[historyIndex].isFavorite;

			ImGui::PushID(static_cast<int>(historyIndex));
			ImGui::Text("%u", history[historyIndex].seed);
			ImGui::SameLine();
			if (ImGui::Checkbox("Use", &isSelected)) {

				// Checkboxの表示を使いながら選択状態を一件だけに限定
				selectedSeedIndex_ = isSelected
					? std::optional<std::size_t>{ historyIndex }
					: std::nullopt;
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Favorite", &isFavorite)) {
				gameSeedSystem.SetFavorite(historyIndex, isFavorite);
			}
			ImGui::PopID();
		}

		ImGui::EndDisabled();
	}

	ImGui::End();
#endif // USE_IMGUI
}

void Title::Finalize() {
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
