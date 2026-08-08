#include "EffectSequenceEditor.h"
#include "ImGuiHeaders.h"
#include "Render/Object/3d/BeamEffect/BeamEffectSystem3d.h"
#include "Render/Object/3d/EffectSequence/EffectSequenceSystem.h"
#include "Render/Object/3d/Particle/ParticleSystem3d.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectSystem3d.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectSystem3d.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <charconv>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::EffectSequence;

	/// @brief 文字列を固定長Bufferへコピーする
	/// @tparam Size Buffer要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 使用可能なSequence Asset名を生成する
	/// @param system 名前を確認するSystem
	/// @param createdName 初期名または直前に作成した名前
	/// @return 使用可能な名前
	std::string MakeAvailableAssetName(
		const EffectSequenceSystem& system,
		const std::string& createdName) {
		if (system.IsAssetNameAvailable(createdName)) {
			return createdName;
		}
		std::size_t suffixStart = createdName.size();
		while (suffixStart > 0) {
			const char character = createdName[suffixStart - 1];
			if (character < '0' || character > '9') {
				break;
			}
			--suffixStart;
		}
		std::string baseName = createdName.substr(0, suffixStart);
		uint64_t suffix = 1;
		if (suffixStart < createdName.size()) {
			const char* begin = createdName.data() + suffixStart;
			const char* end = createdName.data() + createdName.size();
			const std::from_chars_result result = std::from_chars(begin, end, suffix);
			if (result.ec == std::errc{} && result.ptr == end) {
				++suffix;
			} else {
				baseName = createdName;
				suffix = 1;
			}
		}
		for (;;) {
			const std::string candidate = baseName + std::to_string(suffix);
			if (system.IsAssetNameAvailable(candidate)) {
				return candidate;
			}
			++suffix;
		}
	}

	/// @brief Node Typeの日本語表示名を取得する
	/// @param nodeType Node Type
	/// @return 表示名
	const char* GetNodeTypeName(EffectSequenceNodeType nodeType) {
		switch (nodeType) {
		case EffectSequenceNodeType::Particle: return "Particle";
		case EffectSequenceNodeType::PrimitiveEffect: return "Cylinder";
		case EffectSequenceNodeType::Ribbon: return "Ribbon";
		case EffectSequenceNodeType::Beam: return "Beam";
		case EffectSequenceNodeType::Count:
		default: return "不明";
		}
	}

	/// @brief Node Typeに対応するEffect Asset名一覧を取得する
	/// @param nodeType Node Type
	/// @return 対応Asset名一覧
	std::vector<std::string> GetEffectAssetNames(EffectSequenceNodeType nodeType) {
		switch (nodeType) {
		case EffectSequenceNodeType::Particle:
			return MadoEngine::Particle::ParticleSystem3d::GetInstance().GetAssetNames();
		case EffectSequenceNodeType::PrimitiveEffect:
			return MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().GetAssetNames();
		case EffectSequenceNodeType::Ribbon:
			return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().GetAssetNames();
		case EffectSequenceNodeType::Beam:
			return MadoEngine::Beam::BeamEffectSystem3d::GetInstance().GetAssetNames();
		case EffectSequenceNodeType::Count:
		default:
			return {};
		}
	}

	/// @brief Node Typeに対応する固有設定へ置き換える
	/// @param node 設定対象Node
	void ResetNodeSettings(EffectSequenceNode& node) {
		switch (node.nodeType) {
		case EffectSequenceNodeType::PrimitiveEffect:
			node.settings = PrimitiveEffectNodeSettings{};
			break;
		case EffectSequenceNodeType::Ribbon:
			node.settings = RibbonNodeSettings{};
			break;
		case EffectSequenceNodeType::Beam:
			node.settings = BeamNodeSettings{};
			break;
		case EffectSequenceNodeType::Particle:
		case EffectSequenceNodeType::Count:
		default:
			node.settings = ParticleNodeSettings{};
			break;
		}
		const std::vector<std::string> assetNames = GetEffectAssetNames(node.nodeType);
		node.effectAssetName = assetNames.empty() ? std::string{} : assetNames.front();
	}

	/// @brief Node IDからNodeを検索する
	/// @param nodes 検索対象Node一覧
	/// @param nodeId 検索するNode ID
	/// @return 該当Node。存在しない場合はnullptr
	EffectSequenceNode* FindNode(std::vector<EffectSequenceNode>& nodes, uint32_t nodeId) {
		const auto found = std::find_if(nodes.begin(), nodes.end(), [nodeId](const EffectSequenceNode& node) {
			return node.nodeId == nodeId;
		});
		return found != nodes.end() ? &*found : nullptr;
	}

	/// @brief Parent候補が循環参照を作るか確認する
	/// @param nodes Node一覧
	/// @param nodeId 編集対象Node ID
	/// @param parentNodeId Parent候補Node ID
	/// @return 循環参照になる場合はtrue
	bool WouldCreateParentCycle(
		const std::vector<EffectSequenceNode>& nodes,
		uint32_t nodeId,
		uint32_t parentNodeId) {
		uint32_t currentId = parentNodeId;
		for (std::size_t depth = 0; depth <= nodes.size(); ++depth) {
			if (currentId == nodeId) {
				return true;
			}
			const auto found = std::find_if(nodes.begin(), nodes.end(), [currentId](const EffectSequenceNode& node) {
				return node.nodeId == currentId;
			});
			if (found == nodes.end() || !found->parentNodeId.has_value()) {
				return false;
			}
			currentId = found->parentNodeId.value();
		}
		return true;
	}

	/// @brief Preview SequenceをImmediate停止する
	/// @param system 停止に使用するSystem
	/// @param handle 停止するPreview Handle
	void StopPreview(EffectSequenceSystem& system, EffectSequenceHandle& handle) {
		if (system.IsAlive(handle)) {
			system.Stop(handle, EffectSequenceStopMode::Immediate);
		}
		handle = {};
	}

	/// @brief Preview Sequenceを再生する
	/// @param system 再生に使用するSystem
	/// @param assetName 再生するAsset名
	/// @param rootTransform Preview Root Transform
	/// @param isLoop Loop再生する場合はtrue
	/// @param playbackSpeed Preview再生速度
	/// @return Preview Handle
	EffectSequenceHandle PlayPreview(
		EffectSequenceSystem& system,
		const std::string& assetName,
		const Transform3D& rootTransform,
		bool isLoop,
		float playbackSpeed) {
		EffectSequencePlayDesc desc;
		desc.rootTransform = rootTransform;
		desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
		desc.loopOverride = isLoop;
		desc.playbackSpeedOverride = playbackSpeed;
		desc.context = EffectSequencePlaybackContext::EditorPreview;
		return system.Play(assetName, desc);
	}

	/// @brief Transform編集UIを描画する
	/// @param transform 編集対象Transform
	/// @return 値を変更した場合はtrue
	bool DrawTransformEditor(Transform3D& transform) {
		bool changed = false;
		changed |= ImGui::DragFloat3("位置", &transform.translate.x, 0.05f, -1000000.0f, 1000000.0f);
		changed |= ImGui::DragFloat3("回転（ラジアン）", &transform.rotate.x, 0.01f, -10000.0f, 10000.0f);
		changed |= ImGui::DragFloat3("スケール", &transform.scale.x, 0.01f, 0.001f, 10000.0f);
		return changed;
	}

	/// @brief Preview再生ヘッドを描画する
	/// @param playbackTime 現在時刻
	/// @param duration Sequence時間
	void DrawPlaybackHead(float playbackTime, float duration) {
		const ImVec2 size((std::max)(ImGui::GetContentRegionAvail().x, 100.0f), 36.0f);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##EffectSequencePlaybackHead", size);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(start, { start.x + size.x, start.y + size.y }, IM_COL32(32, 35, 42, 255));
		for (int tick = 0; tick <= 10; ++tick) {
			const float x = start.x + size.x * static_cast<float>(tick) / 10.0f;
			drawList->AddLine({ x, start.y + 18.0f }, { x, start.y + size.y }, IM_COL32(90, 95, 105, 255));
		}
		const float normalizedTime = duration > 0.0f ? std::clamp(playbackTime / duration, 0.0f, 1.0f) : 0.0f;
		const float headX = start.x + size.x * normalizedTime;
		drawList->AddLine({ headX, start.y }, { headX, start.y + size.y }, IM_COL32(255, 185, 40, 255), 2.0f);
		const std::string label = std::to_string(playbackTime) + " / " + std::to_string(duration) + " 秒";
		drawList->AddText({ start.x + 6.0f, start.y + 2.0f }, IM_COL32_WHITE, label.c_str());
	}

	/// @brief 選択Nodeの詳細設定UIを描画する
	/// @param node 編集対象Node
	/// @return 値を変更した場合はtrue
	bool DrawNodeInspector(EffectSequenceNode& node) {
		bool changed = false;
		ImGui::SeparatorText("ローカルトランスフォーム");
		changed |= DrawTransformEditor(node.localTransform);
		if (auto* beam = std::get_if<BeamNodeSettings>(&node.settings)) {
			ImGui::SeparatorText("ビーム固有設定（ローカル空間）");
			changed |= ImGui::DragFloat3("始点", &beam->startPosition.x, 0.05f);
			changed |= ImGui::DragFloat3("終点", &beam->endPosition.x, 0.05f);
		} else if (auto* ribbon = std::get_if<RibbonNodeSettings>(&node.settings)) {
			ImGui::SeparatorText("リボン固有設定");
			changed |= ImGui::Checkbox("手動制御点を上書き", &ribbon->overrideManualControlPoints);
			if (ribbon->overrideManualControlPoints) {
				int removePointIndex = -1;
				for (int index = 0; index < static_cast<int>(ribbon->controlPoints.size()); ++index) {
					ImGui::PushID(index);
					changed |= ImGui::DragFloat3("制御点", &ribbon->controlPoints[index].x, 0.05f);
					ImGui::SameLine();
					if (ImGui::SmallButton("削除")) {
						removePointIndex = index;
					}
					ImGui::PopID();
				}
				if (removePointIndex >= 0) {
					ribbon->controlPoints.erase(ribbon->controlPoints.begin() + removePointIndex);
					changed = true;
				}
				ImGui::BeginDisabled(ribbon->controlPoints.size() >= MadoEngine::Ribbon::kMaximumRibbonPointCount);
				if (ImGui::Button("制御点を追加")) {
					ribbon->controlPoints.push_back({});
					changed = true;
				}
				ImGui::EndDisabled();
			}
		} else if (const auto* primitive = std::get_if<PrimitiveEffectNodeSettings>(&node.settings)) {
			(void)primitive;
			ImGui::TextDisabled("プリミティブエフェクト種別：円柱");
		}
		return changed;
	}

#endif // USE_IMGUI

} // namespace

namespace MadoEngine::Editor {

	void DrawEffectSequenceEditorUI() {
#ifdef USE_IMGUI
		EffectSequenceSystem& system = EffectSequenceSystem::GetInstance();
		static int selectedAssetIndex = 0;
		static uint32_t selectedNodeId = 0;
		static int addNodeTypeIndex = 0;
		static EffectSequenceHandle previewHandle;
		static std::string previewAssetName;
		static Transform3D previewRootTransform;
		static bool previewLoop = false;
		static float previewSpeed = 1.0f;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::string renameOriginalName;
		static std::unordered_map<std::string, std::string> savedSnapshots;
		static bool isNameInitialized = false;
		if (!isNameInitialized) {
			CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, "EffectSequence"));
			isNameInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(1120.0f, 820.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(800.0f, 560.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("エフェクトシーケンスエディター")) {
			ImGui::End();
			return;
		}
		if (!system.IsAlive(previewHandle)) {
			previewHandle = {};
			previewAssetName.clear();
		}

		std::vector<std::string> assetNames = system.GetAssetNames();
		std::string selectedAssetName;
		if (!assetNames.empty()) {
			selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
			selectedAssetName = assetNames[selectedAssetIndex];
		}

		if (ImGui::CollapsingHeader("アセット操作", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemWidth(260.0f);
			ImGui::InputText("新規アセット名", newAssetNameBuffer.data(), newAssetNameBuffer.size());
			const std::string newName = newAssetNameBuffer.data();
			const bool canCreate = system.IsAssetNameAvailable(newName);
			ImGui::SameLine();
			ImGui::BeginDisabled(!canCreate);
			if (ImGui::Button("新規作成")) {
				if (system.CreateAsset(newName)) {
					StopPreview(system, previewHandle);
					assetNames = system.GetAssetNames();
					selectedAssetIndex = static_cast<int>(std::distance(
						assetNames.begin(),
						std::find(assetNames.begin(), assetNames.end(), newName)
					));
					selectedNodeId = 0;
					CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, newName));
				}
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(selectedAssetName.empty() || !canCreate);
			if (ImGui::Button("複製")) {
				if (system.DuplicateAsset(selectedAssetName, newName)) {
					StopPreview(system, previewHandle);
					assetNames = system.GetAssetNames();
					selectedAssetIndex = static_cast<int>(std::distance(
						assetNames.begin(),
						std::find(assetNames.begin(), assetNames.end(), newName)
					));
					selectedNodeId = 0;
					CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, newName));
				}
			}
			ImGui::EndDisabled();
		}

		if (assetNames.empty()) {
			ImGui::TextDisabled("編集するエフェクトシーケンスアセットを作成してください。");
			ImGui::End();
			return;
		}

		selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
		selectedAssetName = assetNames[selectedAssetIndex];
		if (ImGui::BeginCombo("編集アセット", selectedAssetName.c_str())) {
			for (int index = 0; index < static_cast<int>(assetNames.size()); ++index) {
				const bool selected = index == selectedAssetIndex;
				if (ImGui::Selectable(assetNames[index].c_str(), selected)) {
					StopPreview(system, previewHandle);
					selectedAssetIndex = index;
					selectedAssetName = assetNames[index];
					selectedNodeId = 0;
					renameOriginalName.clear();
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (renameOriginalName != selectedAssetName) {
			CopyToBuffer(renameAssetNameBuffer, selectedAssetName);
			renameOriginalName = selectedAssetName;
		}
		ImGui::SetNextItemWidth(260.0f);
		ImGui::InputText("アセット名", renameAssetNameBuffer.data(), renameAssetNameBuffer.size());
		const std::string renameName = renameAssetNameBuffer.data();
		ImGui::SameLine();
		ImGui::BeginDisabled(renameName == selectedAssetName || !system.IsAssetNameAvailable(renameName));
		if (ImGui::Button("名前変更")) {
			const std::string oldName = selectedAssetName;
			StopPreview(system, previewHandle);
			if (system.RenameAsset(oldName, renameName)) {
				savedSnapshots.erase(oldName);
				assetNames = system.GetAssetNames();
				selectedAssetIndex = static_cast<int>(std::distance(
					assetNames.begin(),
					std::find(assetNames.begin(), assetNames.end(), renameName)
				));
				selectedAssetName = renameName;
				renameOriginalName.clear();
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("削除")) {
			ImGui::OpenPopup("EffectSequenceDeleteConfirmation");
		}
		if (ImGui::BeginPopupModal("EffectSequenceDeleteConfirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("「%s」を削除しますか？", selectedAssetName.c_str());
			ImGui::TextDisabled("JSONファイルは.trashへ退避されます。");
			if (ImGui::Button("削除する")) {
				StopPreview(system, previewHandle);
				savedSnapshots.erase(selectedAssetName);
				system.DeleteAsset(selectedAssetName);
				assetNames = system.GetAssetNames();
				selectedAssetIndex = assetNames.empty()
					? 0
					: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
				selectedNodeId = 0;
				renameOriginalName.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (assetNames.empty()) {
			ImGui::End();
			return;
		}

		selectedAssetName = assetNames[std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1)];
		EffectSequenceAsset* asset = system.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::End();
			return;
		}
		savedSnapshots.try_emplace(selectedAssetName, asset->ToJson().dump());
		bool isDirty = savedSnapshots[selectedAssetName] != asset->ToJson().dump();
		if (ImGui::Button("保存")) {
			asset->Validate();
			if (asset->SaveToFile({}, true)) {
				savedSnapshots[selectedAssetName] = asset->ToJson().dump();
				isDirty = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("再読み込み")) {
			StopPreview(system, previewHandle);
			if (system.ReloadAsset(selectedAssetName)) {
				asset = system.FindEditableAsset(selectedAssetName);
				if (asset) {
					savedSnapshots[selectedAssetName] = asset->ToJson().dump();
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("バックアップ読み込み")) {
			StopPreview(system, previewHandle);
			if (system.LoadAssetBackup(selectedAssetName)) {
				asset = system.FindEditableAsset(selectedAssetName);
			}
		}
		ImGui::SameLine();
		if (isDirty) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "未保存");
		} else {
			ImGui::TextDisabled("保存済み");
		}

		bool changed = false;
		EffectSequenceConfig& config = asset->GetConfig();
		ImGui::SeparatorText("シーケンス設定");
		changed |= ImGui::Checkbox("ループ", &config.isLoop);
		changed |= ImGui::DragFloat(
			"再生時間",
			&config.duration,
			0.01f,
			kMinimumEffectSequenceDuration,
			kMaximumEffectSequenceDuration,
			"%.3f 秒"
		);
		changed |= ImGui::DragFloat(
			"シーケンス再生速度",
			&config.playbackSpeed,
			0.01f,
			kMinimumEffectSequencePlaybackSpeed,
			kMaximumEffectSequencePlaybackSpeed,
			"%.2f倍"
		);

		ImGui::SeparatorText("プレビュー");
		if (ImGui::Button("再生")) {
			StopPreview(system, previewHandle);
			previewHandle = PlayPreview(system, selectedAssetName, previewRootTransform, previewLoop, previewSpeed);
			if (system.IsAlive(previewHandle)) {
				previewAssetName = selectedAssetName;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("一時停止")) {
			system.Pause(previewHandle);
		}
		ImGui::SameLine();
		if (ImGui::Button("再開")) {
			system.Resume(previewHandle);
		}
		ImGui::SameLine();
		if (ImGui::Button("終了待ち停止")) {
			system.Stop(previewHandle, EffectSequenceStopMode::Finish);
		}
		ImGui::SameLine();
		if (ImGui::Button("即時停止")) {
			StopPreview(system, previewHandle);
		}
		const bool previewLoopChanged = ImGui::Checkbox("プレビューをループ", &previewLoop);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f);
		const bool previewSpeedChanged = ImGui::DragFloat(
			"プレビュー速度",
			&previewSpeed,
			0.01f,
			kMinimumEffectSequencePlaybackSpeed,
			kMaximumEffectSequencePlaybackSpeed,
			"%.2f倍"
		);
		if (ImGui::TreeNodeEx("プレビュールートトランスフォーム", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (DrawTransformEditor(previewRootTransform) && system.IsAlive(previewHandle)) {
				system.SetTransform(previewHandle, previewRootTransform);
			}
			ImGui::TreePop();
		}
		if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
			if (previewLoopChanged) {
				StopPreview(system, previewHandle);
				previewHandle = PlayPreview(system, selectedAssetName, previewRootTransform, previewLoop, previewSpeed);
				previewAssetName = system.IsAlive(previewHandle) ? selectedAssetName : std::string{};
			} else if (previewSpeedChanged) {
				system.SetPlaybackSpeed(previewHandle, previewSpeed);
			}
			system.SetTransform(previewHandle, previewRootTransform);
		}

		const float previewTime = system.GetPlaybackTime(previewHandle).value_or(0.0f);
		DrawPlaybackHead(previewTime, config.duration);
		ImGui::TextDisabled(
			"プレビュー：%s / シーケンス再生数：%zu",
			system.IsAlive(previewHandle)
				? (system.IsPaused(previewHandle) ? "一時停止中" : "再生中")
				: "停止中",
			system.GetActiveSequenceCount()
		);

		ImGui::SeparatorText("タイムライン");
		const char* addNodeTypeNames[] = { "Particle", "Cylinder", "Ribbon", "Beam" };
		ImGui::SetNextItemWidth(160.0f);
		ImGui::Combo("追加ノードタイプ", &addNodeTypeIndex, addNodeTypeNames, static_cast<int>(std::size(addNodeTypeNames)));
		ImGui::SameLine();
		ImGui::BeginDisabled(config.nodes.size() >= kMaximumEffectSequenceNodeCount);
		if (ImGui::Button("ノード追加")) {
			EffectSequenceNode node;
			node.nodeId = asset->GenerateNodeId();
			node.nodeType = static_cast<EffectSequenceNodeType>(addNodeTypeIndex);
			node.displayName = GetNodeTypeName(node.nodeType);
			ResetNodeSettings(node);
			config.nodes.push_back(std::move(node));
			selectedNodeId = config.nodes.back().nodeId;
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		EffectSequenceNode* selectedNode = FindNode(config.nodes, selectedNodeId);
		ImGui::BeginDisabled(!selectedNode || config.nodes.size() >= kMaximumEffectSequenceNodeCount);
		if (ImGui::Button("選択ノード複製")) {
			EffectSequenceNode copy = *selectedNode;
			copy.nodeId = asset->GenerateNodeId();
			copy.displayName += " のコピー";
			config.nodes.push_back(std::move(copy));
			selectedNodeId = config.nodes.back().nodeId;
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!selectedNode);
		if (ImGui::Button("選択ノード削除")) {
			std::erase_if(config.nodes, [](const EffectSequenceNode& node) {
				return node.nodeId == selectedNodeId;
			});
			selectedNodeId = 0;
			changed = true;
		}
		ImGui::EndDisabled();

		int moveFrom = -1;
		int moveTo = -1;
		std::array<
			std::vector<std::string>,
			static_cast<std::size_t>(EffectSequenceNodeType::Count)
		> effectAssetNamesByType;
		for (std::size_t typeIndex = 0; typeIndex < effectAssetNamesByType.size(); ++typeIndex) {
			effectAssetNamesByType[typeIndex] = GetEffectAssetNames(
				static_cast<EffectSequenceNodeType>(typeIndex)
			);
		}
		if (ImGui::BeginTable(
			"EffectSequenceTimeline",
			9,
			ImGuiTableFlags_Borders |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_Resizable |
				ImGuiTableFlags_ScrollX |
				ImGuiTableFlags_ScrollY,
			ImVec2(0.0f, 260.0f),
			1350.0f)) {
			ImGui::TableSetupColumn("有効", ImGuiTableColumnFlags_WidthFixed, 45.0f);
			ImGui::TableSetupColumn("ノード", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("タイプ", ImGuiTableColumnFlags_WidthFixed, 125.0f);
			ImGui::TableSetupColumn("エフェクトアセット", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("タイムライン", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("再生速度", ImGuiTableColumnFlags_WidthFixed, 95.0f);
			ImGui::TableSetupColumn("描画レイヤー", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableSetupColumn("親ノード", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("順序", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			ImGui::TableHeadersRow();
			for (int index = 0; index < static_cast<int>(config.nodes.size()); ++index) {
				EffectSequenceNode& node = config.nodes[index];
				ImGui::PushID(static_cast<int>(node.nodeId));
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Checkbox("##Enabled", &node.isEnabled)) {
					selectedNodeId = node.nodeId;
					changed = true;
				}
				ImGui::TableSetColumnIndex(1);
				std::array<char, 128> displayNameBuffer{};
				CopyToBuffer(displayNameBuffer, node.displayName);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::InputText("##DisplayName", displayNameBuffer.data(), displayNameBuffer.size())) {
					node.displayName = displayNameBuffer.data();
					selectedNodeId = node.nodeId;
					changed = true;
				}
				if (ImGui::IsItemActivated()) {
					selectedNodeId = node.nodeId;
				}
				ImGui::TableSetColumnIndex(2);
				int nodeType = static_cast<int>(node.nodeType);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Combo(
					"##NodeType",
					&nodeType,
					addNodeTypeNames,
					static_cast<int>(std::size(addNodeTypeNames)))) {
					node.nodeType = static_cast<EffectSequenceNodeType>(nodeType);
					ResetNodeSettings(node);
					selectedNodeId = node.nodeId;
					changed = true;
				}
				if (ImGui::IsItemActivated()) {
					selectedNodeId = node.nodeId;
				}
				ImGui::TableSetColumnIndex(3);
				const std::vector<std::string>& effectAssetNames = effectAssetNamesByType[
					static_cast<std::size_t>(node.nodeType)
				];
				const char* selectedEffectAsset = node.effectAssetName.empty()
					? "未選択"
					: node.effectAssetName.c_str();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##EffectAsset", selectedEffectAsset)) {
					for (const std::string& assetName : effectAssetNames) {
						const bool selected = node.effectAssetName == assetName;
						if (ImGui::Selectable(assetName.c_str(), selected)) {
							node.effectAssetName = assetName;
							selectedNodeId = node.nodeId;
							changed = true;
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				if (ImGui::IsItemActivated()) {
					selectedNodeId = node.nodeId;
				}
				ImGui::TableSetColumnIndex(5);
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed |= ImGui::DragFloat(
					"##NodePlaybackSpeed",
					&node.playbackSpeed,
					0.01f,
					kMinimumEffectSequencePlaybackSpeed,
					kMaximumEffectSequencePlaybackSpeed,
					"%.2f倍"
				);
				if (ImGui::IsItemActivated()) {
					selectedNodeId = node.nodeId;
				}
				ImGui::TableSetColumnIndex(7);
				std::string parentLabel = "シーケンスルート";
				if (node.parentNodeId.has_value()) {
					if (const EffectSequenceNode* parent = FindNode(config.nodes, node.parentNodeId.value())) {
						parentLabel = parent->displayName + " [" + std::to_string(parent->nodeId) + "]";
					}
				}
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##ParentNode", parentLabel.c_str())) {
					selectedNodeId = node.nodeId;
					const bool isRoot = !node.parentNodeId.has_value();
					if (ImGui::Selectable("シーケンスルート", isRoot)) {
						node.parentNodeId.reset();
						changed = true;
					}
					for (const EffectSequenceNode& candidate : config.nodes) {
						if (
							candidate.nodeId == node.nodeId ||
							WouldCreateParentCycle(config.nodes, node.nodeId, candidate.nodeId)) {
							continue;
						}
						const bool selected = node.parentNodeId == candidate.nodeId;
						const std::string label = candidate.displayName +
							" [" + std::to_string(candidate.nodeId) + "]";
						if (ImGui::Selectable(label.c_str(), selected)) {
							node.parentNodeId = candidate.nodeId;
							changed = true;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::TableSetColumnIndex(6);
				const int renderLayerIndex = node.renderLayer.has_value()
					? static_cast<int>(node.renderLayer.value()) + 1
					: 0;
				const char* renderLayerLabel = renderLayerIndex == 0
					? "Sequence Default"
					: MadoEngine::Render::GetRenderLayerName(node.renderLayer.value());
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##RenderLayer", renderLayerLabel)) {
					selectedNodeId = node.nodeId;
					if (ImGui::Selectable("Sequence Default", renderLayerIndex == 0)) {
						node.renderLayer.reset();
						changed = true;
					}
					for (uint32_t layerIndex = 0; layerIndex < MadoEngine::Render::kRenderLayerCount; ++layerIndex) {
						const auto layer = MadoEngine::Render::GetRenderLayerByIndex(layerIndex);
						if (ImGui::Selectable(
							MadoEngine::Render::GetRenderLayerName(layer),
							node.renderLayer == layer)) {
							node.renderLayer = layer;
							changed = true;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::TableSetColumnIndex(4);
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed |= ImGui::SliderFloat("##TimelineDrag", &node.startTime, 0.0f, config.duration, "%.3f 秒");
				if (ImGui::IsItemActivated()) {
					selectedNodeId = node.nodeId;
				}
				ImGui::TableSetColumnIndex(8);
				if (ImGui::SmallButton("↑") && index > 0) {
					moveFrom = index;
					moveTo = index - 1;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("↓") && index + 1 < static_cast<int>(config.nodes.size())) {
					moveFrom = index;
					moveTo = index + 1;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (moveFrom >= 0 && moveTo >= 0) {
			std::swap(config.nodes[moveFrom], config.nodes[moveTo]);
			changed = true;
		}

		selectedNode = FindNode(config.nodes, selectedNodeId);
		if (selectedNode) {
			ImGui::SeparatorText("選択ノード設定");
			changed |= DrawNodeInspector(*selectedNode);
		}

		if (changed) {
			asset->Validate();
			if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
				StopPreview(system, previewHandle);
				previewHandle = PlayPreview(system, selectedAssetName, previewRootTransform, previewLoop, previewSpeed);
				previewAssetName = system.IsAlive(previewHandle) ? selectedAssetName : std::string{};
			}
		}
		ImGui::End();
#endif // USE_IMGUI
	}

} // namespace MadoEngine::Editor
