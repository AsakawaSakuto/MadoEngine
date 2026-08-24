#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

namespace {
	constexpr float kTitleGroundHalfSize = 300.0f;
}

Title::Title(CommonData& commonData)
	: commonData_(commonData) {}

Title::~Title() {}

void Title::Initialize() {
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);

	debugCameraHandle_ = cameraManager_.CreateCamera<DebugCamera>("TitleDebugCamera");
	tpsCameraHandle_ = cameraManager_.CreateCamera<TPS_Camera>("TitlePlayerCamera");
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->SetDistance(35.0f);
	}

	// Title内だけでPlayerの接地と移動範囲を成立させるため専用地面を登録
	AABB ground;
	ground.min = { -kTitleGroundHalfSize, -1.0f, -kTitleGroundHalfSize };
	ground.max = { kTitleGroundHalfSize, 0.0f, kTitleGroundHalfSize };
	groundCollider_ = ground;
	groundPosition_ = {};
	MyCollider::RegisterCollider("TitleGround", CollisionTag::MapBlock, &groundCollider_, &groundPosition_, 1.0f);

	player_ = std::make_unique<Player::Base>();
	player_->Initialize({}, SceneType::Title);
	player_->SetCamera(cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_));

	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());
		tpsCamera->SetDistance(15.0f);
		tpsCamera->SetOffset({ 0.0f, 2.0f, 0.0f });
	}
	cameraManager_.CutTo(tpsCameraHandle_);
}

SceneType Title::Update(float dt) {
	SceneType nextSceneType = SceneType::Title;
	const SceneTransitionController& transitionController =
		commonData_.GetSceneTransitionController();

	// Player移動後の座標でColliderと接地状態を確定して描画姿勢へ反映
	player_->Update(dt);
	MyCollider::Update();
	player_->ResolveAfterCollision();
	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());
	}

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
	player_->DrawImGui();

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

Vector3 Title::GetShadowFocusPosition() const {
	if (!player_) {
		return GetCamera().GetPosition();
	}

	return player_->GetPosition();
}

bool Title::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!player_) {
		outPosition = {};
		return false;
	}

	outPosition = player_->GetModelPosition();
	return true;
}

void Title::Finalize() {
	if (player_) {
		player_->SetCamera(nullptr);
	}
	MyCollider::RemoveColliderAll();
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	tpsCameraHandle_ = {};
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
