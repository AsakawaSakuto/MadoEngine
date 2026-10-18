#include "MapEditor.h"
#include "EventObject/BossSpawner/BossSpawner.h"
#include "GameObject/Player/Player.h"
#include "Utility/Random.h"
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"

namespace {
/// @brief 総数変更後も現在比率を維持して二種類の生成数を再配分
/// @param previousTotal 変更前の総数
/// @param newTotal 変更後の総数
/// @param firstCount 一種類目の生成数
/// @param secondCount 二種類目の生成数
void RebalanceTypeCounts(int previousTotal, int newTotal, int& firstCount, int& secondCount) {

	// 総数ゼロで除算せず、新規配置時は二種類へ均等配分
	if (newTotal <= 0) {
		firstCount = 0;
		secondCount = 0;
		return;
	}

	const float firstRatio = [&]() {
		if (previousTotal > 0) {
			return (static_cast<float>(firstCount) / static_cast<float>(previousTotal));
		} else {
			return (0.5f);
		}
	}();
	firstCount = std::clamp(
		static_cast<int>(std::lround(firstRatio * static_cast<float>(newTotal))),
		0,
		newTotal
	);
	secondCount = newTotal - firstCount;
}
}
#endif // USE_IMGUI

void MapEditor::Initialize(uint32_t seed, const MapGenerationSettings& appliedSettings) {
	editorSettings_ = appliedSettings;

	// 保存済みシードよりもSceneが指定した今回のシードを優先
	editorSettings_.seed = seed;
	editorSettings_.LoadFromFile(false);
	editorSettings_.seed = seed;
	editorSettings_.Clamp();
	pendingGenerationSettings_.reset();
}

void MapEditor::DrawImGui(const PreviewData& preview) {

#ifdef USE_IMGUI

	ImGui::SetNextWindowSize(ImVec2(520.0f, 720.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Map Generator")) {
		ImGui::TextDisabled("設定ファイル: %s", MapGenerationSettings::kFilePath);
		if (ImGui::BeginTabBar("MapGeneratorTabs")) {
			if (ImGui::BeginTabItem("MapStatus")) {
				DrawMapStatusEditor(preview.settings);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("EventObjStatus")) {
				DrawEventObjectStatusEditor();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("MapView")) {
				DrawMapViewEditor(preview);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		// 非表示タブの設定も含めて相互依存する範囲と内訳を毎Frame補正
		editorSettings_.Clamp();
	}
	ImGui::End();

#else
	(void)preview;
#endif // USE_IMGUI

}

void MapEditor::DrawMapStatusEditor(const MapGenerationSettings& appliedSettings) {
#ifdef USE_IMGUI
	ImGui::SeparatorText("生成操作");
	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputScalar("シード", ImGuiDataType_U32, &editorSettings_.seed);
	ImGui::SameLine();
	if (ImGui::Button("ランダム")) {
		editorSettings_.seed = MyRand::CreateSeed();
	}

	const bool hasDraftChanges = !editorSettings_.IsEqualTo(appliedSettings);
	if (ImGui::Button("現在の設定でMapを再生成", ImVec2(-1.0f, 0.0f))) {

		// 描画中のResourceを破棄しないようFrame末尾の適用要求だけを保持
		pendingGenerationSettings_ = editorSettings_;
	}
	if (pendingGenerationSettings_) {
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "再生成を予約済み");
	} else if (hasDraftChanges) {
		ImGui::TextColored(ImVec4(0.25f, 0.75f, 1.0f, 1.0f), "未適用の生成設定あり");
	} else {
		ImGui::TextDisabled("生成済みMapと設定が一致");
	}
	if (ImGui::Button("生成済み設定へ戻す")) {
		editorSettings_ = appliedSettings;
		pendingGenerationSettings_.reset();
	}

	ImGui::SeparatorText("Mapサイズ");
	ImGui::SetNextItemWidth(180.0f);
	ImGui::DragInt("横幅", &editorSettings_.mapWidth, 1.0f, MapGenerationSettings::kMinimumMapSize, MapGenerationSettings::kMaximumMapSize, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SetNextItemWidth(180.0f);
	ImGui::DragInt("奥行き", &editorSettings_.mapHeight, 1.0f, MapGenerationSettings::kMinimumMapSize, MapGenerationSettings::kMaximumMapSize, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragFloat3("Blockサイズ", &editorSettings_.blockSize.x, 0.1f, 0.5f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

	ImGui::SeparatorText("地形の高さ");
	ImGui::DragInt("最小高さ", &editorSettings_.minHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("最大高さ", &editorSettings_.maxHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("開始高さ Min", &editorSettings_.minStartHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("開始高さ Max", &editorSettings_.maxStartHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("高さ変化 Min", &editorSettings_.minRangeHeight, 1.0f, -10, 0, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("高さ変化 Max", &editorSettings_.maxRangeHeight, 1.0f, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("Slope出現率", &editorSettings_.slopeSpawnRate, 0.0f, 1.0f, "%.2f");
#else
	(void)appliedSettings;
#endif // USE_IMGUI
}

void MapEditor::DrawEventObjectStatusEditor() {
#ifdef USE_IMGUI

	// 総数の変更時は内訳の比率を維持し、内訳の変更時は残数を反対側へ配分
	ImGui::SeparatorText("イベント配置数");
	ImGui::TextUnformatted("Jar");
	ImGui::Indent();
	const int previousJarTotal = editorSettings_.jarSpawnCount;
	if (ImGui::DragInt("総数##Jar", &editorSettings_.jarSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		RebalanceTypeCounts(
			previousJarTotal,
			editorSettings_.jarSpawnCount,
			editorSettings_.moneyJarSpawnCount,
			editorSettings_.expJarSpawnCount
		);
	}
	if (ImGui::DragInt("Money##Jar", &editorSettings_.moneyJarSpawnCount, 1.0f, 0, editorSettings_.jarSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.moneyJarSpawnCount = std::clamp(editorSettings_.moneyJarSpawnCount, 0, editorSettings_.jarSpawnCount);
		editorSettings_.expJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.moneyJarSpawnCount;
	}
	if (ImGui::DragInt("Exp##Jar", &editorSettings_.expJarSpawnCount, 1.0f, 0, editorSettings_.jarSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.expJarSpawnCount = std::clamp(editorSettings_.expJarSpawnCount, 0, editorSettings_.jarSpawnCount);
		editorSettings_.moneyJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.expJarSpawnCount;
	}
	ImGui::Unindent();

	ImGui::TextUnformatted("Chest");
	ImGui::Indent();
	const int previousChestTotal = editorSettings_.chestSpawnCount;
	if (ImGui::DragInt("総数##Chest", &editorSettings_.chestSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		RebalanceTypeCounts(
			previousChestTotal,
			editorSettings_.chestSpawnCount,
			editorSettings_.normalChestSpawnCount,
			editorSettings_.freeChestSpawnCount
		);
	}
	if (ImGui::DragInt("Normal##Chest", &editorSettings_.normalChestSpawnCount, 1.0f, 0, editorSettings_.chestSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.normalChestSpawnCount = std::clamp(editorSettings_.normalChestSpawnCount, 0, editorSettings_.chestSpawnCount);
		editorSettings_.freeChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.normalChestSpawnCount;
	}
	if (ImGui::DragInt("Free##Chest", &editorSettings_.freeChestSpawnCount, 1.0f, 0, editorSettings_.chestSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.freeChestSpawnCount = std::clamp(editorSettings_.freeChestSpawnCount, 0, editorSettings_.chestSpawnCount);
		editorSettings_.normalChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.freeChestSpawnCount;
	}
	ImGui::Unindent();

	ImGui::TextUnformatted("Karma");
	ImGui::Indent();
	ImGui::DragInt("総数##Karma", &editorSettings_.karmaSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();

	ImGui::TextUnformatted("Magnet");
	ImGui::Indent();
	ImGui::DragInt("総数##Magnet", &editorSettings_.magnetSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();

	ImGui::TextUnformatted("Anvil");
	ImGui::Indent();
	ImGui::DragInt("総数##Anvil", &editorSettings_.anvilSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();

	ImGui::TextUnformatted("BossSpawner");
	ImGui::Indent();
	ImGui::DragInt("総数##BossSpawner", &editorSettings_.bossSpawnerSpawnCount, 1.0f, 0, MapGenerationSettings::kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();
#endif // USE_IMGUI
}

void MapEditor::DrawMapViewEditor(const PreviewData& preview) const {
#ifdef USE_IMGUI

	// 未適用の編集値を混在させないよう生成済み設定と実体だけを参照
	ImGui::SeparatorText("生成済みMap");
	std::size_t slopeCount = 0;
	for (const std::vector<MapBlock>& row : preview.blocks) {
		slopeCount += static_cast<std::size_t>(std::count_if(row.begin(), row.end(), [](const MapBlock& block) {
			return block.GetType() == MapBlockType::Slope;
		}));
	}
	ImGui::Text("シード: %u", preview.settings.seed);
	ImGui::Text("Block: %d x %d = %d", preview.settings.mapWidth, preview.settings.mapHeight, preview.settings.mapWidth * preview.settings.mapHeight);
	ImGui::Text("Slope: %zu / Event Object: %zu", slopeCount, preview.eventObjects.size());
	ImGui::Text("Jar: %d  Money: %d  Exp: %d", preview.settings.jarSpawnCount, preview.settings.moneyJarSpawnCount, preview.settings.expJarSpawnCount);
	ImGui::Text("Chest: %d  Normal: %d  Free: %d", preview.settings.chestSpawnCount, preview.settings.normalChestSpawnCount, preview.settings.freeChestSpawnCount);
	ImGui::Text("Karma: %d  Magnet: %d  BossSpawner: %d", preview.settings.karmaSpawnCount, preview.settings.magnetSpawnCount, preview.settings.bossSpawnerSpawnCount);
	ImGui::Text("Anvil: %d", preview.settings.anvilSpawnCount);
	if (preview.player) {
		const Vector3 playerPosition = preview.player->GetPosition();
		ImGui::Text("Player: X %.2f  Y %.2f  Z %.2f", playerPosition.x, playerPosition.y, playerPosition.z);
	}
	DrawHeightPreview(preview);
#else
	(void)preview;
#endif // USE_IMGUI
}

void MapEditor::DrawHeightPreview(const PreviewData& preview) const {
#ifdef USE_IMGUI
	if (preview.blocks.empty() || preview.blocks.front().empty()) {
		ImGui::TextDisabled("プレビューできるMapがありません");
		return;
	}

	constexpr float cellSize = 14.0f;
	std::vector<bool> bossSpawnerCells(static_cast<std::size_t>(preview.settings.mapWidth) * static_cast<std::size_t>(preview.settings.mapHeight), false);
	for (const std::unique_ptr<MapEventObjectBase>& object : preview.eventObjects) {
		if (!object || dynamic_cast<const BossSpawner*>(object.get()) == nullptr) {
			continue;
		}

		const Vector3 bossSpawnerPosition = object->GetPosition();
		const int bossSpawnerX = std::clamp(static_cast<int>(std::lround(bossSpawnerPosition.x / preview.settings.blockSize.x)), 0, preview.settings.mapWidth - 1);
		const int bossSpawnerZ = std::clamp(static_cast<int>(std::lround(bossSpawnerPosition.z / preview.settings.blockSize.z)), 0, preview.settings.mapHeight - 1);
		const std::size_t cellIndex = static_cast<std::size_t>(bossSpawnerZ) * static_cast<std::size_t>(preview.settings.mapWidth) + static_cast<std::size_t>(bossSpawnerX);
		bossSpawnerCells[cellIndex] = true;
	}

	ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "■ BossSpawner");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.1f, 0.9f, 1.0f, 1.0f), "● Player");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.15f, 1.0f, 0.4f, 1.0f), "□ Slope");
	const ImVec2 canvasSize(
		static_cast<float>(preview.settings.mapWidth) * cellSize,
		static_cast<float>(preview.settings.mapHeight) * cellSize
	);
	ImGui::BeginChild("MapHeightPreview", ImVec2(0.0f, 320.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
	const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("MapHeightPreviewCanvas", canvasSize);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float heightRange = static_cast<float>((std::max)(1, preview.settings.maxHeight - preview.settings.minHeight));

	// 高さを寒色から暖色へ変換し、BossSpawner配置Blockだけ赤色で上書き
	for (int z = 0; z < preview.settings.mapHeight; ++z) {
		for (int x = 0; x < preview.settings.mapWidth; ++x) {
			const MapBlock& block = preview.blocks[z][x];
			const std::size_t cellIndex = static_cast<std::size_t>(z) * static_cast<std::size_t>(preview.settings.mapWidth) + static_cast<std::size_t>(x);
			const float heightRate = std::clamp(
				(static_cast<float>(block.GetHeight()) - static_cast<float>(preview.settings.minHeight)) / heightRange,
				0.0f,
				1.0f
			);
			ImVec4 color(
				0.12f + heightRate * 0.78f,
				0.28f + (1.0f - std::abs(heightRate - 0.5f) * 2.0f) * 0.42f,
				0.88f - heightRate * 0.68f,
				1.0f
			);
			if (bossSpawnerCells[cellIndex]) {
				color = { 0.95f, 0.08f, 0.08f, 1.0f };
			}
			const ImVec2 cellMin(canvasPosition.x + static_cast<float>(x) * cellSize, canvasPosition.y + static_cast<float>(z) * cellSize);
			const ImVec2 cellMax(cellMin.x + cellSize - 1.0f, cellMin.y + cellSize - 1.0f);
			drawList->AddRectFilled(cellMin, cellMax, ImGui::GetColorU32(color));
			drawList->AddRect(
				cellMin,
				cellMax,
				[&]() {
					if (block.GetType() == MapBlockType::Slope) {
						return (IM_COL32(40, 255, 110, 255));
					} else {
						return (IM_COL32(30, 30, 30, 180));
					}
				}()
			);
		}
	}

	if (preview.player) {
		const Vector3 playerPosition = preview.player->GetPosition();
		const float playerGridX = (playerPosition.x + preview.settings.blockSize.x * 0.5f) / preview.settings.blockSize.x;
		const float playerGridZ = (playerPosition.z + preview.settings.blockSize.z * 0.5f) / preview.settings.blockSize.z;
		if (playerGridX >= 0.0f && playerGridX <= static_cast<float>(preview.settings.mapWidth) &&
			playerGridZ >= 0.0f && playerGridZ <= static_cast<float>(preview.settings.mapHeight)) {

			// Block内の移動量も読み取れるようPlayerのワールド座標をセル中心へ丸めず描画座標へ変換
			const ImVec2 playerMarkerPosition(
				canvasPosition.x + playerGridX * cellSize,
				canvasPosition.y + playerGridZ * cellSize
			);
			drawList->AddCircleFilled(playerMarkerPosition, 4.5f, IM_COL32(25, 230, 255, 255));
			drawList->AddCircle(playerMarkerPosition, 5.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);
		}
	}

	if (ImGui::IsItemHovered()) {
		const ImVec2 mousePosition = ImGui::GetIO().MousePos;
		const int x = std::clamp(static_cast<int>((mousePosition.x - canvasPosition.x) / cellSize), 0, preview.settings.mapWidth - 1);
		const int z = std::clamp(static_cast<int>((mousePosition.y - canvasPosition.y) / cellSize), 0, preview.settings.mapHeight - 1);
		const MapBlock& block = preview.blocks[z][x];
		const std::size_t cellIndex = static_cast<std::size_t>(z) * static_cast<std::size_t>(preview.settings.mapWidth) + static_cast<std::size_t>(x);
		const bool hasBossSpawner = bossSpawnerCells[cellIndex];
		ImGui::SetTooltip(
			"X: %d  Z: %d\n高さ: %u\n種別: %s%s",
			x,
			z,
			block.GetHeight(),
			[&]() {
				if (block.GetType() == MapBlockType::Slope) {
					return ("Slope");
				} else {
					return ("Ground");
				}
			}(),
			[&]() {
				if (hasBossSpawner) {
					return ("\nBossSpawner: あり");
				} else {
					return ("");
				}
			}()
		);
	}
	ImGui::EndChild();
#else
	(void)preview;
#endif // USE_IMGUI
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
