#ifdef USE_IMGUI

#include "MapEditor.h"

#include "ImGuiHeaders.h"
#include "Utility/Random.h"
#include "EventObject/BossSpawner/BossSpawner.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <cmath>
#include <limits>

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

/// @brief 生成数テーブルのラベルと数値入力を一行で描画
/// @param label 生成数の表示名とImGui識別子
/// @param count 編集対象の生成数
/// @return 入力値を変更した場合はtrue
bool DrawEventObjectCountRow(const char* label, int& count) {
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::TableSetColumnIndex(1);

	// 数値欄の幅を揃え、増減ボタンと直接入力を併用可能な操作領域を確保
	ImGui::PushID(label);
	ImGui::SetNextItemWidth((std::min)(ImGui::GetContentRegionAvail().x, 220.0f));
	const bool isChanged = ImGui::InputInt("##Count", &count, 1, 10);
	ImGui::PopID();
	return isChanged;
}

/// @brief AABBテーブルの三軸入力を一行で描画
/// @param label MinまたはMaxの表示名とImGui識別子
/// @param value 編集対象のローカル座標
void DrawEventObjectColliderRow(const char* label, Vector3& value) {
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::PushID(label);

	// 軸ごとの列幅と識別子を固定し、MinとMaxを見比べながら独立して調整
	for (int axis = 0; axis < 3; ++axis) {
		ImGui::TableSetColumnIndex(axis + 1);
		ImGui::PushID(axis);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::DragFloat("##Value", &value[axis], 0.05f, 0.0f, 0.0f, "%.2f");
		ImGui::PopID();
	}
	ImGui::PopID();
}

/// @brief イベント種別で共有する自動発火時間と接触範囲の編集欄を描画
/// @param label 編集対象のImGui識別子
/// @param settings 編集対象の共通イベント設定
void DrawEventObjectInteractionSettings(const char* label, MapEventObjectSettings& settings) {
	ImGui::PushID(label);
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;

	// 長い日本語ラベルで入力領域が縮まないようラベルと値を別の列へ配置
	if (ImGui::BeginTable("InteractionSettings", 2, tableFlags)) {
		ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 140.0f);
		ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("自動発火まで");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth((std::min)(ImGui::GetContentRegionAvail().x, 220.0f));
		ImGui::DragFloat("##Duration", &settings.autoInteractionDuration, 0.05f,
			0.0f, (std::numeric_limits<float>::max)(), "%.2f 秒", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("0秒で接触時に即時発火");
		}
		ImGui::EndTable();
	}

	ImGui::SeparatorText("AABB（原点からのオフセット）");
	if (ImGui::BeginTable("ColliderSettings", 4, tableFlags | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("範囲", ImGuiTableColumnFlags_WidthFixed, 52.0f);
		ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();
		DrawEventObjectColliderRow("Min", settings.colliderMin);
		DrawEventObjectColliderRow("Max", settings.colliderMax);
		ImGui::EndTable();
	}
	ImGui::PushTextWrapPos();
	ImGui::TextDisabled("左右ドラッグで調整 / Ctrl+クリックで数値入力");
	ImGui::PopTextWrapPos();
	ImGui::PopID();
}
}
void MapEditor::DrawImGui(const PreviewData& preview) {


	ImGui::SetNextWindowSize(ImVec2(520.0f, 720.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Map Generator")) {
		ImGui::TextDisabled("設定ファイル: %s", MapGenerationSettings::kFilePath);
		if (ImGui::BeginTabBar("MapGeneratorTabs")) {
			if (ImGui::BeginTabItem("MapStatus")) {
				DrawMapStatusEditor(preview.settings);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("EventObjStatus")) {
				DrawEventObjectStatusEditor(preview.settings);
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


}

void MapEditor::DrawGenerationControls(const MapGenerationSettings& appliedSettings) {
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
}

void MapEditor::DrawMapStatusEditor(const MapGenerationSettings& appliedSettings) {
	DrawGenerationControls(appliedSettings);

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
}

void MapEditor::DrawEventObjectStatusEditor(const MapGenerationSettings& appliedSettings) {
	DrawGenerationControls(appliedSettings);
	ImGui::TextDisabled("変更はMap再生成時に反映");

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

	// 総数の一覧と詳細設定を分け、画面幅に依存しない固定幅の数値入力へ統一
	ImGui::SeparatorText("生成数（総数）");
	if (ImGui::BeginTable("EventObjectCounts", 2, tableFlags)) {
		ImGui::TableSetupColumn("種類", ImGuiTableColumnFlags_WidthFixed, 140.0f);
		ImGui::TableSetupColumn("生成数", ImGuiTableColumnFlags_WidthStretch);
		const int previousJarTotal = editorSettings_.jarSpawnCount;
		if (DrawEventObjectCountRow("Jar", editorSettings_.jarSpawnCount)) {
			editorSettings_.jarSpawnCount = std::clamp(editorSettings_.jarSpawnCount, 0, MapGenerationSettings::kMaximumEventObjectCount);
			RebalanceTypeCounts(previousJarTotal, editorSettings_.jarSpawnCount,
				editorSettings_.moneyJarSpawnCount, editorSettings_.expJarSpawnCount);
		}
		const int previousChestTotal = editorSettings_.chestSpawnCount;
		if (DrawEventObjectCountRow("Chest", editorSettings_.chestSpawnCount)) {
			editorSettings_.chestSpawnCount = std::clamp(editorSettings_.chestSpawnCount, 0, MapGenerationSettings::kMaximumEventObjectCount);
			RebalanceTypeCounts(previousChestTotal, editorSettings_.chestSpawnCount,
				editorSettings_.normalChestSpawnCount, editorSettings_.freeChestSpawnCount);
		}
		DrawEventObjectCountRow("Karma", editorSettings_.karmaSpawnCount);
		DrawEventObjectCountRow("Magnet", editorSettings_.magnetSpawnCount);
		DrawEventObjectCountRow("Anvil", editorSettings_.anvilSpawnCount);
		DrawEventObjectCountRow("BossSpawner", editorSettings_.bossSpawnerSpawnCount);
		ImGui::EndTable();
	}

	// 選択中の種類だけ詳細を表示し、生成数の内訳と共通の相互作用設定を明確化
	ImGui::SeparatorText("種類別の設定");
	if (ImGui::BeginTabBar("EventObjectSettingsTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
		if (ImGui::BeginTabItem("Jar")) {
			ImGui::SeparatorText("生成数の内訳");
			if (ImGui::BeginTable("JarTypeCounts", 2, tableFlags)) {
				ImGui::TableSetupColumn("内訳", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("生成数", ImGuiTableColumnFlags_WidthStretch);
				if (DrawEventObjectCountRow("Money", editorSettings_.moneyJarSpawnCount)) {
					editorSettings_.moneyJarSpawnCount = std::clamp(editorSettings_.moneyJarSpawnCount, 0, editorSettings_.jarSpawnCount);
					editorSettings_.expJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.moneyJarSpawnCount;
				}
				if (DrawEventObjectCountRow("Exp", editorSettings_.expJarSpawnCount)) {
					editorSettings_.expJarSpawnCount = std::clamp(editorSettings_.expJarSpawnCount, 0, editorSettings_.jarSpawnCount);
					editorSettings_.moneyJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.expJarSpawnCount;
				}
				ImGui::EndTable();
			}
			ImGui::SeparatorText("共通設定（Money / Exp）");
			DrawEventObjectInteractionSettings("Jar", editorSettings_.jarSettings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Chest")) {
			ImGui::SeparatorText("生成数の内訳");
			if (ImGui::BeginTable("ChestTypeCounts", 2, tableFlags)) {
				ImGui::TableSetupColumn("内訳", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("生成数", ImGuiTableColumnFlags_WidthStretch);
				if (DrawEventObjectCountRow("Normal", editorSettings_.normalChestSpawnCount)) {
					editorSettings_.normalChestSpawnCount = std::clamp(editorSettings_.normalChestSpawnCount, 0, editorSettings_.chestSpawnCount);
					editorSettings_.freeChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.normalChestSpawnCount;
				}
				if (DrawEventObjectCountRow("Free", editorSettings_.freeChestSpawnCount)) {
					editorSettings_.freeChestSpawnCount = std::clamp(editorSettings_.freeChestSpawnCount, 0, editorSettings_.chestSpawnCount);
					editorSettings_.normalChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.freeChestSpawnCount;
				}
				ImGui::EndTable();
			}
			ImGui::SeparatorText("共通設定（Normal / Free）");
			DrawEventObjectInteractionSettings("Chest", editorSettings_.chestSettings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Karma")) {
			DrawEventObjectInteractionSettings("Karma", editorSettings_.karmaSettings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Magnet")) {
			DrawEventObjectInteractionSettings("Magnet", editorSettings_.magnetSettings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Anvil")) {
			DrawEventObjectInteractionSettings("Anvil", editorSettings_.anvilSettings);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("BossSpawner")) {
			DrawEventObjectInteractionSettings("BossSpawner", editorSettings_.bossSpawnerSettings);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void MapEditor::DrawMapViewEditor(const PreviewData& preview) const {

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
}

void MapEditor::DrawHeightPreview(const PreviewData& preview) const {
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
}


#endif // USE_IMGUI
