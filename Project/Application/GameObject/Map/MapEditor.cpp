#include "MapEditor.h"
#include <nlohmann/json.hpp>

void MapEditor::Initialize(uint32_t seed, const MapGenerationSettings& appliedSettings) {
	editorSettings_ = appliedSettings;

	// 保存済みシードよりもSceneが指定した今回のシードを優先
	editorSettings_.seed = seed;
	editorSettings_.LoadFromFile(false);
	editorSettings_.seed = seed;
	editorSettings_.Clamp();
	pendingGenerationSettings_.reset();
}

std::optional<MapGenerationSettings> MapEditor::ConsumePendingGeneration() {

	// 再生成中に編集状態が変化しても要求時点の値を適用できるよう値で移譲
	const std::optional<MapGenerationSettings> settings = pendingGenerationSettings_;
	pendingGenerationSettings_.reset();
	return settings;
}

std::string MapEditor::CaptureState(const MapGenerationSettings& appliedSettings) const {
	nlohmann::json root;
	root["editing"] = editorSettings_.ToJson();
	root["applied"] = appliedSettings.ToJson();
	if (pendingGenerationSettings_) {

		// 適用前の予約も保存して再生成操作のUndoとRedoを再現
		root["pending"] = pendingGenerationSettings_->ToJson();
	}
	return root.dump();
}

void MapEditor::RestoreState(const std::string& snapshot, const MapGenerationSettings& currentAppliedSettings) {
	const nlohmann::json root = nlohmann::json::parse(snapshot, nullptr, false);
	if (root.is_discarded() || !root.is_object()) {
		return;
	}

	MapGenerationSettings editingSettings = editorSettings_;
	const auto editingIt = root.find("editing");
	if (editingIt == root.end() || !editingSettings.LoadFromJson(*editingIt)) {
		return;
	}

	MapGenerationSettings appliedSettings = currentAppliedSettings;
	const auto appliedIt = root.find("applied");
	if (appliedIt == root.end() || !appliedSettings.LoadFromJson(*appliedIt)) {
		return;
	}

	// 編集値と生成済み状態を分離して再生成操作のUndoでも直前の地形を復元
	editorSettings_ = editingSettings;
	const auto pendingIt = root.find("pending");
	if (pendingIt != root.end()) {
		MapGenerationSettings pendingSettings = appliedSettings;
		if (pendingSettings.LoadFromJson(*pendingIt)) {
			pendingGenerationSettings_ = pendingSettings;
		}
	} else if (!appliedSettings.IsEqualTo(currentAppliedSettings)) {
		pendingGenerationSettings_ = appliedSettings;
	} else {
		pendingGenerationSettings_.reset();
	}
}

bool MapEditor::SaveSettings() const {
	return editorSettings_.SaveToFile();
}

bool MapEditor::ReloadSettings() {
	MapGenerationSettings settings = editorSettings_;
	if (!settings.LoadFromFile(true)) {
		return false;
	}

	// 読込中にMapを破棄せずFrame末尾で適用するため予約だけを更新
	editorSettings_ = settings;
	pendingGenerationSettings_ = settings;
	return true;
}
