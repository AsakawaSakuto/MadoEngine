#include "EditorToolbar.h"

#ifdef USE_IMGUI

#include "History/EditorHistory.h"
#include "ImGuiHeaders.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <utility>

namespace MadoEngine::Editor {

namespace {

constexpr const char* kEditorToolbarSettingsPath = "SavedData/EditorToolbar.json";
constexpr float kMinimumTimeScale = 0.05f;
constexpr float kMaximumTimeScale = 4.0f;
constexpr float kMinimumFixedStep = 1.0f / 240.0f;
constexpr float kMaximumFixedStep = 1.0f;
constexpr double kOperationStatusDuration = 3.0;

struct EditorWindowMenuItem {
	EditorWindow window;
	const char* label;
};

constexpr std::array<EditorWindowMenuItem, 20> kWindowMenuItems = {
	EditorWindowMenuItem{ EditorWindow::GameView, "Game View" },
	EditorWindowMenuItem{ EditorWindow::FixedGameView, "Game View 1280x720" },
	EditorWindowMenuItem{ EditorWindow::EngineInfo, "Engine Info" },
	EditorWindowMenuItem{ EditorWindow::SceneManager, "Scene Manager" },
	EditorWindowMenuItem{ EditorWindow::Camera, "Camera Editor" },
	EditorWindowMenuItem{ EditorWindow::SceneDebug, "Scene固有デバッグ" },
	EditorWindowMenuItem{ EditorWindow::ModelGizmo, "Model Gizmo" },
	EditorWindowMenuItem{ EditorWindow::PostEffect, "Post Effect Editor" },
	EditorWindowMenuItem{ EditorWindow::Audio, "Audio Editor" },
	EditorWindowMenuItem{ EditorWindow::Light, "Light Editor" },
	EditorWindowMenuItem{ EditorWindow::Model, "Model Editor" },
	EditorWindowMenuItem{ EditorWindow::Sprite, "Sprite Editor" },
	EditorWindowMenuItem{ EditorWindow::Text, "Text Editor" },
	EditorWindowMenuItem{ EditorWindow::Particle, "Particle Editor" },
	EditorWindowMenuItem{ EditorWindow::Cylinder, "Cylinder Editor" },
	EditorWindowMenuItem{ EditorWindow::Ribbon, "Ribbon Editor" },
	EditorWindowMenuItem{ EditorWindow::Beam, "Beam Editor" },
	EditorWindowMenuItem{ EditorWindow::EffectSequence, "Effect Sequence Editor" },
	EditorWindowMenuItem{ EditorWindow::StyleColor, "ImGui Style Color" },
	EditorWindowMenuItem{ EditorWindow::Logger, "Logger" },
};

/// @brief enum classを配列添字へ変換
/// @tparam Enum 変換する列挙型
/// @param value 変換する列挙値
/// @return 配列添字
template <class Enum>
constexpr std::size_t ToIndex(Enum value) {
	return static_cast<std::size_t>(value);
}

/// @brief Jsonから有限なfloat値を取得
/// @param json 読み込み元Json
/// @param key 読み込むキー
/// @param fallback 読み込み失敗時の値
/// @return 読み込んだ有限値
float ReadFiniteFloat(const nlohmann::json& json, const char* key, float fallback) {
	const auto it = json.find(key);
	if (it == json.end() || !it->is_number()) {
		return fallback;
	}

	const float value = it->get<float>();
	return std::isfinite(value) ? value : fallback;
}

} // namespace

EditorToolbar& EditorToolbar::GetInstance() {
	static EditorToolbar instance;
	return instance;
}

EditorToolbar::EditorToolbar() {
	ApplyDefaultSettings();
}

void EditorToolbar::Initialize() {
	if (isInitialized_) {
		return;
	}

	ApplyDefaultSettings();
	LoadSettings();
	isInitialized_ = true;
}

void EditorToolbar::Finalize() {
	if (!isInitialized_) {
		return;
	}

	SaveSettings();
	isInitialized_ = false;
}

float EditorToolbar::Draw() {
	HandleShortcuts();
	DrawMainMenu();
	const float toolbarHeight = DrawPlaybackToolbar();
	DrawReloadConfirmationPopup();
	return toolbarHeight;
}

bool EditorToolbar::ConsumeGameDeltaTime(float realDeltaTime, float& outGameDeltaTime) {
	const float safeRealDeltaTime = std::isfinite(realDeltaTime)
		? (std::max)(0.0f, realDeltaTime)
		: 0.0f;

	if (!isPaused_) {
		outGameDeltaTime = safeRealDeltaTime * timeScale_;
		return true;
	}

	if (!stepRequested_) {
		outGameDeltaTime = 0.0f;
		return false;
	}

	// Step要求を一度だけ消費して固定deltaTimeの一Frameへ変換
	stepRequested_ = false;
	outGameDeltaTime = fixedStepDeltaTime_ * timeScale_;
	return true;
}

bool EditorToolbar::IsWindowVisible(EditorWindow window) const {
	const std::size_t index = ToIndex(window);
	return index < windowVisibility_.size() && windowVisibility_[index];
}

bool EditorToolbar::ConsumeSaveAllRequest() {
	const bool requested = saveAllRequested_;
	saveAllRequested_ = false;
	return requested;
}

bool EditorToolbar::ConsumeReloadAllRequest() {
	const bool requested = reloadAllRequested_;
	reloadAllRequested_ = false;
	return requested;
}

bool EditorToolbar::ConsumeSaveLayoutRequest() {
	const bool requested = saveLayoutRequested_;
	saveLayoutRequested_ = false;
	return requested;
}

bool EditorToolbar::ConsumeResetLayoutRequest() {
	const bool requested = resetLayoutRequested_;
	resetLayoutRequested_ = false;
	return requested;
}

void EditorToolbar::BeginDocumentCapture(EditorDocument document) {
	if (document == EditorDocument::Count || isDocumentCaptureActive_) {
		return;
	}

	ImGuiContext* context = ImGui::GetCurrentContext();
	capturedDocument_ = document;
	documentEditedAtCaptureStart_ = context && context->ActiveIdHasBeenEditedThisFrame;
	isDocumentCaptureActive_ = true;
}

void EditorToolbar::EndDocumentCapture() {
	if (!isDocumentCaptureActive_) {
		return;
	}

	ImGuiContext* context = ImGui::GetCurrentContext();
	if (context && !documentEditedAtCaptureStart_ && context->ActiveIdHasBeenEditedThisFrame) {
		MarkDocumentDirty(capturedDocument_);
	}

	capturedDocument_ = EditorDocument::Count;
	documentEditedAtCaptureStart_ = false;
	isDocumentCaptureActive_ = false;
}

void EditorToolbar::MarkDocumentDirty(EditorDocument document) {
	const std::size_t index = ToIndex(document);
	if (index < dirtyDocuments_.size()) {
		dirtyDocuments_[index] = true;
	}
}

void EditorToolbar::MarkDocumentSaved(EditorDocument document) {
	const std::size_t index = ToIndex(document);
	if (index < dirtyDocuments_.size()) {
		dirtyDocuments_[index] = false;
	}
}

void EditorToolbar::MarkAllDocumentsSaved() {
	dirtyDocuments_.fill(false);
}

void EditorToolbar::NotifyDocumentOperationResult(const char* actionName, bool succeeded) {
	NotifyOperationResult(actionName, succeeded);
	if (succeeded) {
		MarkAllDocumentsSaved();
	}
}

void EditorToolbar::NotifyOperationResult(const char* actionName, bool succeeded) {
	operationStatusMessage_ = actionName ? actionName : "Editor操作";
	operationStatusMessage_ += succeeded ? "に成功" : "に失敗";
	operationStatusSucceeded_ = succeeded;
	operationStatusExpireTime_ = ImGui::GetTime() + kOperationStatusDuration;
}

void EditorToolbar::ApplyDefaultSettings() {
	windowVisibility_.fill(true);
	windowVisibility_[ToIndex(EditorWindow::Logger)] = false;
	dirtyDocuments_.fill(false);
	isPaused_ = false;
	stepRequested_ = false;
	timeScale_ = 1.0f;
	fixedStepDeltaTime_ = 1.0f / 60.0f;
}

bool EditorToolbar::LoadSettings() {
	if (!Json::JsonFile::Exists(kEditorToolbarSettingsPath)) {
		return false;
	}

	nlohmann::json root;
	if (!Json::JsonFile::Load(kEditorToolbarSettingsPath, root) || !root.is_object()) {
		return false;
	}

	timeScale_ = std::clamp(
		ReadFiniteFloat(root, "timeScale", timeScale_),
		kMinimumTimeScale,
		kMaximumTimeScale
	);
	fixedStepDeltaTime_ = std::clamp(
		ReadFiniteFloat(root, "fixedStepDeltaTime", fixedStepDeltaTime_),
		kMinimumFixedStep,
		kMaximumFixedStep
	);

	const nlohmann::json visibility = root.value("windowVisibility", nlohmann::json::object());
	if (visibility.is_object()) {
		for (const EditorWindowMenuItem& item : kWindowMenuItems) {
			const auto it = visibility.find(item.label);
			if (it != visibility.end() && it->is_boolean()) {
				windowVisibility_[ToIndex(item.window)] = it->get<bool>();
			}
		}
	}

	return true;
}

bool EditorToolbar::SaveSettings() const {
	nlohmann::json root;
	root["timeScale"] = timeScale_;
	root["fixedStepDeltaTime"] = fixedStepDeltaTime_;
	nlohmann::json visibility = nlohmann::json::object();
	for (const EditorWindowMenuItem& item : kWindowMenuItems) {
		visibility[item.label] = windowVisibility_[ToIndex(item.window)];
	}
	root["windowVisibility"] = std::move(visibility);
	return Json::JsonFile::Save(kEditorToolbarSettingsPath, root, 4, false);
}

void EditorToolbar::HandleShortcuts() {
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput || !io.KeyCtrl) {
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_S) && io.KeyShift) {
		saveAllRequested_ = true;
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_R) && io.KeyShift) {
		openReloadConfirmation_ = true;
		return;
	}

	EditorHistory& history = EditorHistory::GetInstance();
	if (ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyShift && history.CanRedo()) {
		history.Redo();
		MarkDocumentDirty(EditorDocument::Model);
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Y) && history.CanRedo()) {
		history.Redo();
		MarkDocumentDirty(EditorDocument::Model);
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Z) && history.CanUndo()) {
		history.Undo();
		MarkDocumentDirty(EditorDocument::Model);
	}
}

void EditorToolbar::DrawMainMenu() {
	if (!ImGui::BeginMainMenuBar()) {
		return;
	}

	if (ImGui::BeginMenu("ファイル")) {
		if (ImGui::MenuItem("すべて保存", "Ctrl+Shift+S")) {
			saveAllRequested_ = true;
		}
		if (ImGui::MenuItem("すべて再読込", "Ctrl+Shift+R")) {
			openReloadConfirmation_ = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("レイアウトを保存")) {
			saveLayoutRequested_ = true;
		}
		if (ImGui::MenuItem("レイアウトを初期化")) {
			resetLayoutRequested_ = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("編集")) {
		EditorHistory& history = EditorHistory::GetInstance();
		if (ImGui::MenuItem("元に戻す", "Ctrl+Z", false, history.CanUndo())) {
			history.Undo();
			MarkDocumentDirty(EditorDocument::Model);
		}
		if (ImGui::MenuItem("やり直す", "Ctrl+Y / Ctrl+Shift+Z", false, history.CanRedo())) {
			history.Redo();
			MarkDocumentDirty(EditorDocument::Model);
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("ウィンドウ")) {
		for (const EditorWindowMenuItem& item : kWindowMenuItems) {
			ImGui::MenuItem(item.label, nullptr, &windowVisibility_[ToIndex(item.window)]);
		}
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

float EditorToolbar::DrawPlaybackToolbar() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float toolbarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize({ viewport->WorkSize.x, toolbarHeight });
	ImGui::SetNextWindowViewport(viewport->ID);

	constexpr ImGuiWindowFlags toolbarFlags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::Begin("##EditorToolbar", nullptr, toolbarFlags);
	ImGui::PopStyleVar();

	if (ImGui::Button(isPaused_ ? "再生" : "一時停止")) {
		isPaused_ = !isPaused_;
		if (!isPaused_) {
			stepRequested_ = false;
		}
	}
	ImGui::SameLine();
	if (!isPaused_) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("1フレーム")) {
		stepRequested_ = true;
	}
	if (!isPaused_) {
		ImGui::EndDisabled();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat(
		"時間倍率",
		&timeScale_,
		0.01f,
		kMinimumTimeScale,
		kMaximumTimeScale,
		"%.2fx",
		ImGuiSliderFlags_AlwaysClamp
	);
	ImGui::SameLine();
	ImGui::TextColored(
		isPaused_ ? ImVec4(1.0f, 0.75f, 0.2f, 1.0f) : ImVec4(0.2f, 1.0f, 0.35f, 1.0f),
		isPaused_ ? "一時停止中" : "実行中"
	);

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	if (ImGui::Button("すべて保存")) {
		saveAllRequested_ = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("すべて再読込")) {
		openReloadConfirmation_ = true;
	}
	if (HasDirtyDocument()) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.15f, 1.0f), "未保存の変更あり");
	}

	if (!operationStatusMessage_.empty() && ImGui::GetTime() <= operationStatusExpireTime_) {
		ImGui::SameLine();
		ImGui::TextColored(
			operationStatusSucceeded_ ? ImVec4(0.2f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
			"%s",
			operationStatusMessage_.c_str()
		);
	}

	ImGui::End();
	return toolbarHeight;
}

void EditorToolbar::DrawReloadConfirmationPopup() {
	if (openReloadConfirmation_) {
		ImGui::OpenPopup("すべて再読込##EditorToolbar");
		openReloadConfirmation_ = false;
	}

	if (!ImGui::BeginPopupModal(
		"すべて再読込##EditorToolbar",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}

	ImGui::TextUnformatted("未保存の編集内容を破棄して、すべての設定を再読込しますか？");
	if (ImGui::Button("再読込", ImVec2(120.0f, 0.0f))) {
		reloadAllRequested_ = true;
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("キャンセル", ImVec2(120.0f, 0.0f))) {
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

bool EditorToolbar::HasDirtyDocument() const {
	return std::any_of(dirtyDocuments_.begin(), dirtyDocuments_.end(), [](bool isDirty) {
		return isDirty;
	});
}

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
