#include "TitleScene.h"
#include "Audio/MyAudio.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"
#include <cmath>

namespace {
	constexpr float kTitleGroundHalfSize = 4.0f;
	constexpr float kTitleMapHorizontalLimit = 16.0f;
	constexpr float kWeaponSelectionStickInputThreshold = 0.5f;
	constexpr float kWeaponSelectionStickNeutralThreshold = 0.2f;
}

TitleScene::TitleScene(CommonData& commonData)
	: commonData_(commonData) {}

TitleScene::~TitleScene() {}

void TitleScene::Initialize() {
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
	player_->Initialize(commonData_.GetParameterStore(), {}, SceneType::Title);

	MapLimit titleMapLimit;
	titleMapLimit.min.x = -kTitleMapHorizontalLimit;
	titleMapLimit.min.z = -kTitleMapHorizontalLimit;
	titleMapLimit.max.x = kTitleMapHorizontalLimit;
	titleMapLimit.max.z = kTitleMapHorizontalLimit;
	player_->SetMapLimit(titleMapLimit);

	player_->SetCamera(cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_));
	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->SetTargetPosition(player_->GetPosition());
		tpsCamera->SetDistance(15.0f);
		tpsCamera->SetOffset({ 0.0f, 2.0f, 0.0f });
	}
	cameraManager_.CutTo(tpsCameraHandle_);
	InitializeWeaponSelection();
}

SceneType TitleScene::Update(float dt) {
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

	// 遷移開始後の選択変更を止め、表示とGameへ渡す武器の一致を維持
	if (!transitionController.IsTransitioning()) {
		UpdateWeaponSelection();
	}

	// 遷移中の連続入力で初期武器とSeed要求を上書きしないため決定操作を制限
	if (!transitionController.IsTransitioning() && MyInput::Trigger("Decision")) {
		const std::vector<System::GameSeedSystem::HistoryEntry>& history =
			commonData_.GetGameSeedSystem().GetHistory();
		std::optional<std::uint32_t> requestedSeed;
		if (selectedSeedIndex_.has_value() && selectedSeedIndex_.value() < history.size()) {
			requestedSeed = history[selectedSeedIndex_.value()].seed;
		}

		// 遷移演出中の選択変更に影響されないよう受付時の武器とSeed要求を固定
		commonData_.SetInitialWeaponType(Projectile::kPlayableWeaponTypes[selectedWeaponIndex_]);
		commonData_.GetGameSeedSystem().RequestSeed(requestedSeed);
		MyAudio::Play("UpgradeCardDecision");
		nextSceneType = SceneType::Game;
	}

	MyDebugLine::AddShape(groundCollider_, Vector4({ 1.0f, 0.0f, 0.0f, 1.0f }));

	cameraManager_.Update(dt);

	return nextSceneType;
}

void TitleScene::InitializeWeaponSelection() {
	const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;
	weaponSelectionBackground_ = MySprite::Create(
		"TitleWeaponSelectionBackground", "white16x16", SceneType::Title, layer);
	if (Sprite* background = MySprite::TryGet(weaponSelectionBackground_)) {
		background->SetFitToScreen(true);
		background->SetColor({ 0.025f, 0.035f, 0.06f, 0.97f });
	}

	selectedWeaponIndex_ = 0;
	isWeaponSelectionStickReady_ = false;

	// 使用可能武器の共通一覧を行優先で配置し、再訪時は前回決定した武器を復元
	for (std::size_t index = 0; index < weaponIcons_.size(); ++index) {
		const Vector2 position = {
			330.0f + static_cast<float>(index % kWeaponColumnCount) * 124.0f,
			250.0f + static_cast<float>(index / kWeaponColumnCount) * 140.0f,
		};
		const Projectile::Type weaponType = Projectile::kPlayableWeaponTypes[index];
		weaponIcons_[index].Initialize(weaponType, position);
		if (weaponType == commonData_.GetInitialWeaponType()) {
			selectedWeaponIndex_ = index;
		}
	}

	weaponSelectionHeading_ = MyText::Create(
		"TitleWeaponSelectionHeading", "初期武器を選択", SceneType::Title, layer);
	selectedWeaponName_ = MyText::Create(
		"TitleSelectedWeaponName", "", SceneType::Title, layer);
	selectedWeaponDescription_ = MyText::Create(
		"TitleSelectedWeaponDescription", "", SceneType::Title, layer);
	weaponSelectionGuide_ = MyText::Create("TitleWeaponSelectionGuide",
		"矢印キー / WASD / 十字キー / Lスティック：選択　　Space / Aボタン：ゲーム開始",
		SceneType::Title, layer);

	// 説明文の行数に左右されない固定領域で選択中武器と操作案内を整列
	const std::array<MadoEngine::TextHandle, 4> textHandles = {
		weaponSelectionHeading_, selectedWeaponName_, selectedWeaponDescription_, weaponSelectionGuide_,
	};
	const std::array<float, 4> positionsY = { 120.0f, 520.0f, 578.0f, 665.0f };
	const std::array<float, 4> fontSizes = { 40.0f, 28.0f, 22.0f, 20.0f };
	for (std::size_t index = 0; index < textHandles.size(); ++index) {
		if (MadoEngine::Text* text = MyText::TryGet(textHandles[index])) {
			text->SetFontAsset("Assets/Font/YuGothB.ttc", "游ゴシック");
			text->SetFontSize(fontSizes[index]);
			text->SetAreaSize({ 1120.0f, 72.0f });
			text->SetAnchorPoint({ 0.5f, 0.5f });
			text->SetPosition({ 640.0f, positionsY[index] });
			text->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			text->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		}
	}
	if (MadoEngine::Text* name = MyText::TryGet(selectedWeaponName_)) {
		name->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
	}
	RefreshWeaponSelection();
}

void TitleScene::UpdateWeaponSelection() {
	std::size_t column = selectedWeaponIndex_ % kWeaponColumnCount;
	std::size_t row = selectedWeaponIndex_ / kWeaponColumnCount;
	const Vector2 stickInput = GetWeaponSelectionStickInput();

	// 横移動は同じ行、縦移動は同じ列を保った循環選択
	if (MyInput::Trigger("Left") || stickInput.x < 0.0f) {
		column = (column + kWeaponColumnCount - 1) % kWeaponColumnCount;
	} else if (MyInput::Trigger("Right") || stickInput.x > 0.0f) {
		column = (column + 1) % kWeaponColumnCount;
	} else if (MyInput::Trigger("Up") || stickInput.y > 0.0f) {
		row = (row + kWeaponRowCount - 1) % kWeaponRowCount;
	} else if (MyInput::Trigger("Down") || stickInput.y < 0.0f) {
		row = (row + 1) % kWeaponRowCount;
	}
	const std::size_t selectedIndex = row * kWeaponColumnCount + column;
	if (selectedIndex == selectedWeaponIndex_) {
		return;
	}

	selectedWeaponIndex_ = selectedIndex;
	RefreshWeaponSelection();
	MyAudio::Play("UpgradeCardSelect");
}

Vector2 TitleScene::GetWeaponSelectionStickInput() {
	const MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
	if (!gamePad) {
		return {};
	}
	const Vector2 stick = gamePad->GetLeftStick();
	const float horizontalAmount = std::abs(stick.x);
	const float verticalAmount = std::abs(stick.y);

	// 中立への復帰だけで再入力を許可し、倒し続けや反対方向への切り返しによる連続移動を防止
	if (horizontalAmount <= kWeaponSelectionStickNeutralThreshold &&
		verticalAmount <= kWeaponSelectionStickNeutralThreshold) {
		isWeaponSelectionStickReady_ = true;
		return {};
	}
	if (!isWeaponSelectionStickReady_ ||
		(horizontalAmount < kWeaponSelectionStickInputThreshold &&
			verticalAmount < kWeaponSelectionStickInputThreshold)) {
		return {};
	}
	isWeaponSelectionStickReady_ = false;

	// 斜め入力は傾きの大きい軸へ限定し、一度の入力で移動する行または列を一つに固定
	if (horizontalAmount >= verticalAmount) {
		return { stick.x, 0.0f };
	}
	return { 0.0f, stick.y };
}

void TitleScene::RefreshWeaponSelection() {

	// 選択変更時だけTextを差し替えて毎Frameの文字画像再生成を回避
	for (std::size_t index = 0; index < weaponIcons_.size(); ++index) {
		weaponIcons_[index].SetSelected(index == selectedWeaponIndex_);
	}
	const Projectile::Type weaponType = Projectile::kPlayableWeaponTypes[selectedWeaponIndex_];
	if (MadoEngine::Text* name = MyText::TryGet(selectedWeaponName_)) {
		name->SetText(Projectile::ProjectileTypeToDisplayName(weaponType));
	}
	if (MadoEngine::Text* description = MyText::TryGet(selectedWeaponDescription_)) {
		description->SetText(Projectile::ProjectileTypeToDescription(weaponType));
	}
}

void TitleScene::Draw() {
}

void TitleScene::DrawImGui() {
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
					if (isSelected) {
						selectedSeedIndex_ = std::optional<std::size_t>{ historyIndex };
					} else {
						selectedSeedIndex_ = std::nullopt;
					}
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

Vector3 TitleScene::GetShadowFocusPosition() const {
	if (!player_) {
		return GetCamera().GetPosition();
	}

	return player_->GetPosition();
}

bool TitleScene::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!player_) {
		outPosition = {};
		return false;
	}

	outPosition = player_->GetModelPosition();
	return true;
}

void TitleScene::Finalize() {

	// SceneManagerによる描画リソース破棄の前にUI側の参照を解放
	for (UI::Title::WeaponSelectIcon& icon : weaponIcons_) {
		icon.Finalize();
	}
	weaponSelectionBackground_ = {};
	weaponSelectionHeading_ = {};
	selectedWeaponName_ = {};
	selectedWeaponDescription_ = {};
	weaponSelectionGuide_ = {};

	if (player_) {
		player_->SetCamera(nullptr);
	}
	MyCollider::RemoveColliderAll();
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	tpsCameraHandle_ = {};
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
