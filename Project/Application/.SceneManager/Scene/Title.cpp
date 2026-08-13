#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title(CommonData& commonData)
	: commonData_(commonData) {}

Title::~Title() {}

void Title::Initialize() {
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);

	debugCameraHandle_ = cameraManager_.CreateCamera<DebugCamera>("TitleDebugCamera");
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->SetDistance(35.0f);
	}
	cameraManager_.CutTo(debugCameraHandle_);
}

SceneType Title::Update(float dt) {
	SceneType nextSceneType = SceneType::Title;
	const SceneTransitionController& transitionController =
		commonData_.GetSceneTransitionController();

	// 遷移中の連続入力でSeed要求と遷移先を上書きしないため決定操作を制限
	if (!transitionController.IsTransitioning() && MyInput::Trigger("Decision")) {
		const std::vector<System::GameSeedSystem::HistoryEntry>& history =
			commonData_.GetGameSeedSystem().GetHistory();
		std::optional<std::uint32_t> requestedSeed;
		if (selectedSeedIndex_.has_value() && selectedSeedIndex_.value() < history.size()) {
			requestedSeed = history[selectedSeedIndex_.value()].seed;
		}

		// 遷移演出中の選択変更に影響されないよう受付時のSeed要求を固定
		commonData_.GetGameSeedSystem().RequestSeed(requestedSeed);
		nextSceneType = SceneType::Game;
		Logger::Output("Decisionが押されました - ゲームシーンへの遷移を要求しました", Logger::Level::Application);
	}

	cameraManager_.Update(dt);

	return nextSceneType;
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
		const bool isSelectionLocked =
			commonData_.GetSceneTransitionController().IsTransitioning();
		ImGui::BeginDisabled(isSelectionLocked);

		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_NoSavedSettings;
		if (ImGui::BeginTable("SeedHistoryTable", 3, tableFlags)) {
			ImGui::TableSetupColumn("Seed", ImGuiTableColumnFlags_WidthFixed, 102.0f);
			ImGui::TableSetupColumn("Use", ImGuiTableColumnFlags_WidthFixed, 72.0f);
			ImGui::TableSetupColumn("Favorite", ImGuiTableColumnFlags_WidthFixed, 92.0f);

			// 固定列へ各項目を配置し、Seedの桁数に左右されない整列を維持
			for (std::size_t displayIndex = 0; displayIndex < history.size(); ++displayIndex) {
				const std::size_t historyIndex = history.size() - displayIndex - 1;
				bool isSelected = selectedSeedIndex_ == historyIndex;
				bool isFavorite = history[historyIndex].isFavorite;

				ImGui::PushID(static_cast<int>(historyIndex));
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%u", history[historyIndex].seed);
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Checkbox("Use", &isSelected)) {

					// Checkboxの表示を使いながら選択状態を一件だけに限定
					selectedSeedIndex_ = isSelected
						? std::optional<std::size_t>{ historyIndex }
						: std::nullopt;
				}
				ImGui::TableSetColumnIndex(2);
				if (ImGui::Checkbox("Favorite", &isFavorite)) {
					gameSeedSystem.SetFavorite(historyIndex, isFavorite);
				}
				ImGui::PopID();
			}

			ImGui::EndTable();
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
