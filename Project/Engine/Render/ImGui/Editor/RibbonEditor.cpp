#include "RibbonEditor.h"
#include "TextureSelector.h"
#include "ImGuiHeaders.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectSystem3d.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::Ribbon;
	using MadoEngine::Effect::EffectKeyframe;
	using MadoEngine::Effect::EffectTrack;

	/// @brief 文字列を固定長Bufferへコピーする
	/// @tparam Size Buffer要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 使用可能なRibbon Asset名を生成する
	/// @param system 名前を確認するSystem
	/// @param baseName 基準名
	/// @return 使用可能な名前
	std::string MakeAvailableAssetName(
		const RibbonEffectSystem3d& system,
		const std::string& baseName) {
		if (system.IsAssetNameAvailable(baseName)) {
			return baseName;
		}
		for (uint32_t suffix = 2; suffix < 10000; ++suffix) {
			const std::string candidate = baseName + " (" + std::to_string(suffix) + ")";
			if (system.IsAssetNameAvailable(candidate)) {
				return candidate;
			}
		}
		return baseName;
	}

	constexpr std::array<const char*, 42> kEaseTypeNames = {
		"Linear",
		"EaseInQuad",
		"EaseOutQuad",
		"EaseInOutQuad",
		"EaseOutInQuad",
		"EaseInCubic",
		"EaseOutCubic",
		"EaseInOutCubic",
		"EaseOutInCubic",
		"EaseInQuart",
		"EaseOutQuart",
		"EaseInOutQuart",
		"EaseOutInQuart",
		"EaseInQuint",
		"EaseOutQuint",
		"EaseInOutQuint",
		"EaseOutInQuint",
		"EaseInSine",
		"EaseOutSine",
		"EaseInOutSine",
		"EaseOutInSine",
		"EaseInExpo",
		"EaseOutExpo",
		"EaseInOutExpo",
		"EaseOutInExpo",
		"EaseInCirc",
		"EaseOutCirc",
		"EaseInOutCirc",
		"EaseOutInCirc",
		"EaseInBack",
		"EaseOutBack",
		"EaseInOutBack",
		"EaseOutInBack",
		"EaseInElastic",
		"EaseOutElastic",
		"EaseInOutElastic",
		"EaseOutInElastic",
		"EaseInBounce",
		"EaseOutBounce",
		"EaseInOutBounce",
		"EaseOutInBounce",
		"None",
	};

	/// @brief イージング種類を選択するComboを描画する
	/// @param label UI表示名
	/// @param easing 編集対象イージング
	/// @return 値を変更した場合はtrue
	bool DrawEaseTypeCombo(const char* label, EaseType& easing) {
		int easingIndex = std::clamp(
			static_cast<int>(easing),
			0,
			static_cast<int>(kEaseTypeNames.size()) - 1
		);
		if (!ImGui::Combo(
			label,
			&easingIndex,
			kEaseTypeNames.data(),
			static_cast<int>(kEaseTypeNames.size()))) {
			return false;
		}
		easing = static_cast<EaseType>(easingIndex);
		return true;
	}

	/// @brief Keyframe追加に使用できる空き時刻を求める
	/// @tparam T Track値型
	/// @param keyframes 時刻順のKeyframe一覧
	/// @param maximumTime Track時刻上限
	/// @param preferredIndex 優先して近傍を検索するKeyframe Index
	/// @return 追加可能な時刻。空きがない場合はstd::nullopt
	template<class T>
	std::optional<float> FindKeyframeInsertionTime(
		const std::vector<EffectKeyframe<T>>& keyframes,
		float maximumTime,
		int preferredIndex) {
		constexpr float minimumGap = 0.0001f;
		const float safeMaximumTime = (std::max)(maximumTime, 0.001f);
		if (keyframes.empty()) {
			return 0.0f;
		}

		if (preferredIndex >= 0 && preferredIndex < static_cast<int>(keyframes.size())) {
			const float preferredTime = keyframes[preferredIndex].time;
			const float nextTime = preferredIndex + 1 < static_cast<int>(keyframes.size())
				? keyframes[preferredIndex + 1].time
				: safeMaximumTime;
			if (nextTime - preferredTime > minimumGap) {
				return (preferredTime + nextTime) * 0.5f;
			}
			const float previousTime = preferredIndex > 0
				? keyframes[preferredIndex - 1].time
				: 0.0f;
			if (preferredTime - previousTime > minimumGap) {
				return (previousTime + preferredTime) * 0.5f;
			}
		}

		float largestGapStart = 0.0f;
		float largestGapEnd = keyframes.front().time;
		float largestGap = largestGapEnd - largestGapStart;
		for (std::size_t index = 0; index + 1 < keyframes.size(); ++index) {
			const float gapStart = keyframes[index].time;
			const float gapEnd = keyframes[index + 1].time;
			if (gapEnd - gapStart > largestGap) {
				largestGapStart = gapStart;
				largestGapEnd = gapEnd;
				largestGap = gapEnd - gapStart;
			}
		}
		const float endGap = safeMaximumTime - keyframes.back().time;
		if (endGap > largestGap) {
			largestGapStart = keyframes.back().time;
			largestGapEnd = safeMaximumTime;
			largestGap = endGap;
		}
		if (largestGap <= minimumGap) {
			return std::nullopt;
		}
		return (largestGapStart + largestGapEnd) * 0.5f;
	}

	/// @brief 型付きEffect Track編集UIを描画する
	/// @tparam T Track値型
	/// @tparam ValueDrawer 値編集関数型
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @param maximumTime Key時刻上限
	/// @param timeFormat 時刻表示Format
	/// @param drawValue 値編集関数
	/// @return 値を変更した場合はtrue
	template<class T, class ValueDrawer>
	bool DrawTrackEditor(
		const char* label,
		EffectTrack<T>& track,
		float maximumTime,
		const char* timeFormat,
		ValueDrawer drawValue) {
		bool changed = false;
		static std::unordered_map<std::string, int> selectedKeyframeIndices;
		int& selectedKeyframeIndex = selectedKeyframeIndices[label];
		ImGui::PushID(label);
		const bool isOpen = ImGui::TreeNodeEx(
			"トラック",
			ImGuiTreeNodeFlags_SpanAvailWidth,
			"%s",
			label
		);
		if (isOpen) {
			T defaultValue = track.GetDefaultValue();
			if (drawValue("既定値", defaultValue)) {
				track.SetDefaultValue(defaultValue);
				changed = true;
			}

			std::vector<EffectKeyframe<T>> keyframes = track.GetKeyframes();
			if (keyframes.empty()) {
				selectedKeyframeIndex = -1;
			} else {
				selectedKeyframeIndex = std::clamp(
					selectedKeyframeIndex,
					0,
					static_cast<int>(keyframes.size()) - 1
				);
			}
			const bool hasSelection = selectedKeyframeIndex >= 0;
			const std::optional<float> insertionTime = FindKeyframeInsertionTime(
				keyframes,
				maximumTime,
				selectedKeyframeIndex
			);
			std::optional<float> requestedSelectionTime;
			constexpr float operationButtonWidth = 72.0f;

			ImGui::BeginDisabled(!insertionTime.has_value());
			if (ImGui::Button("追加", ImVec2(operationButtonWidth, 0.0f))) {
				EffectKeyframe<T> keyframe;
				keyframe.time = insertionTime.value();
				keyframe.value = track.Evaluate(keyframe.time);
				keyframe.easing = EaseType::Linear;
				requestedSelectionTime = keyframe.time;
				keyframes.push_back(keyframe);
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!hasSelection || !insertionTime.has_value());
			if (ImGui::Button("複製", ImVec2(operationButtonWidth, 0.0f))) {
				EffectKeyframe<T> keyframe = keyframes[selectedKeyframeIndex];
				keyframe.time = insertionTime.value();
				requestedSelectionTime = keyframe.time;
				keyframes.push_back(keyframe);
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!hasSelection);
			if (ImGui::Button("削除", ImVec2(operationButtonWidth, 0.0f))) {
				keyframes.erase(keyframes.begin() + selectedKeyframeIndex);
				if (keyframes.empty()) {
					selectedKeyframeIndex = -1;
				} else {
					selectedKeyframeIndex = (std::min)(
						selectedKeyframeIndex,
						static_cast<int>(keyframes.size()) - 1
					);
				}
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(keyframes.empty());
			if (ImGui::Button("全消去", ImVec2(operationButtonWidth, 0.0f))) {
				ImGui::OpenPopup("キーフレームをすべて消去##KeyframeClearConfirmation");
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextDisabled("%zu個", keyframes.size());

			const float tableHeight = std::clamp(
				(static_cast<float>(keyframes.size()) + 1.0f) * ImGui::GetTextLineHeightWithSpacing() + 8.0f,
				100.0f,
				260.0f
			);
			const ImGuiTableFlags tableFlags =
				ImGuiTableFlags_Borders |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingStretchProp |
				ImGuiTableFlags_ScrollY;
			if (ImGui::BeginTable("KeyframeTable", 4, tableFlags, ImVec2(0.0f, tableHeight))) {
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("キーフレーム", ImGuiTableColumnFlags_WidthFixed, 112.0f);
				ImGui::TableSetupColumn("時刻", ImGuiTableColumnFlags_WidthStretch, 0.8f);
				ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch, 1.4f);
				ImGui::TableSetupColumn("イージング", ImGuiTableColumnFlags_WidthStretch, 1.2f);
				ImGui::TableHeadersRow();
				for (int index = 0; index < static_cast<int>(keyframes.size()); ++index) {
					EffectKeyframe<T>& keyframe = keyframes[index];
					ImGui::PushID(index);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					const std::string keyframeLabel = "キーフレーム " + std::to_string(index + 1);
					if (ImGui::Selectable(keyframeLabel.c_str(), selectedKeyframeIndex == index)) {
						selectedKeyframeIndex = index;
					}
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::DragFloat(
						"##Time",
						&keyframe.time,
						0.01f,
						0.0f,
						(std::max)(maximumTime, 0.001f),
						timeFormat
					)) {
						selectedKeyframeIndex = index;
						changed = true;
					}
					ImGui::TableSetColumnIndex(2);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (drawValue("##Value", keyframe.value)) {
						selectedKeyframeIndex = index;
						changed = true;
					}
					ImGui::TableSetColumnIndex(3);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (DrawEaseTypeCombo("##Easing", keyframe.easing)) {
						selectedKeyframeIndex = index;
						changed = true;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			if (ImGui::BeginPopupModal(
				"キーフレームをすべて消去##KeyframeClearConfirmation",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::TextUnformatted("すべてのキーフレームを消去しますか？");
				ImGui::TextDisabled("トラックは既定値を使用する状態へ戻ります。");
				if (ImGui::Button("消去する")) {
					keyframes.clear();
					selectedKeyframeIndex = -1;
					changed = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("キャンセル")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (changed) {
				track.SetKeyframes(std::move(keyframes));
				if (requestedSelectionTime.has_value()) {
					const std::vector<EffectKeyframe<T>>& normalizedKeyframes = track.GetKeyframes();
					const auto selected = std::lower_bound(
						normalizedKeyframes.begin(),
						normalizedKeyframes.end(),
						requestedSelectionTime.value(),
						[](const EffectKeyframe<T>& keyframe, float time) {
							return keyframe.time < time;
						}
					);
					selectedKeyframeIndex = selected == normalizedKeyframes.end()
						? static_cast<int>(normalizedKeyframes.size()) - 1
						: static_cast<int>(std::distance(normalizedKeyframes.begin(), selected));
				}
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return changed;
	}

	/// @brief float Track編集UIを描画する
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @param maximumTime Key時刻上限
	/// @param timeFormat 時刻表示Format
	/// @param speed Drag操作速度
	/// @param minimumValue 値下限
	/// @param maximumValue 値上限
	/// @return 値を変更した場合はtrue
	bool DrawFloatTrack(
		const char* label,
		EffectTrack<float>& track,
		float maximumTime,
		const char* timeFormat,
		float speed,
		float minimumValue,
		float maximumValue) {
		return DrawTrackEditor(
			label,
			track,
			maximumTime,
			timeFormat,
			[speed, minimumValue, maximumValue](const char* valueLabel, float& value) {
				return ImGui::DragFloat(
					valueLabel,
					&value,
					speed,
					minimumValue,
					maximumValue,
					"%.3f"
				);
			}
		);
	}

	/// @brief Color Track編集UIを描画する
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @param maximumTime Key時刻上限
	/// @return 値を変更した場合はtrue
	bool DrawColorTrack(
		const char* label,
		EffectTrack<Vector4>& track,
		float maximumTime) {
		return DrawTrackEditor(
			label,
			track,
			maximumTime,
			"%.3f",
			[](const char* valueLabel, Vector4& value) {
				return ImGui::ColorEdit4(
					valueLabel,
					&value.x,
					ImGuiColorEditFlags_AlphaBar |
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_HDR
				);
			}
		);
	}

	/// @brief Ribbonの基本設定を編集する
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawBasicEditor(RibbonEffectConfig& config) {
		bool changed = false;
		changed |= ImGui::DragFloat(
			"再生時間",
			&config.playback.duration,
			0.01f,
			0.001f,
			3600.0f,
			"%.3f秒"
		);
		changed |= ImGui::Checkbox("ループ再生", &config.playback.isLoop);

		ImGui::SeparatorText("表示範囲の再生");
		const char* playbackModeNames[] = {
			"全体表示 (Full)",
			"先頭から表示 (Reveal)",
			"指定区間を移動 (Sweep)",
		};
		int playbackModeIndex = static_cast<int>(config.playback.mode);
		if (ImGui::Combo(
			"再生方法",
			&playbackModeIndex,
			playbackModeNames,
			static_cast<int>(std::size(playbackModeNames)))) {
			config.playback.mode = static_cast<RibbonPlaybackMode>(playbackModeIndex);
			changed = true;
		}

		switch (config.playback.mode) {
		case RibbonPlaybackMode::Reveal:
			ImGui::TextDisabled("再生率に応じて、最初の制御点から進行位置まで表示します。");
			break;
		case RibbonPlaybackMode::Sweep:
			ImGui::TextDisabled("再生率に応じて、指定した長さの区間を経路上で移動します。");
			changed |= ImGui::DragFloat(
				"表示区間の長さ",
				&config.playback.sweepLength,
				0.01f,
				0.001f,
				100000.0f,
				"%.3f"
			);
			break;
		case RibbonPlaybackMode::Full:
		default:
			ImGui::TextDisabled("再生中は常にRibbon全体を表示します。");
			break;
		}

		if (config.playback.mode != RibbonPlaybackMode::Full) {
			changed |= DrawFloatTrack(
				"再生進行率",
				config.playback.progress,
				1.0f,
				"%.3f",
				0.01f,
				0.0f,
				1.0f
			);
			ImGui::TextDisabled("トラック時刻0.0が再生開始、1.0が再生終了です。");
		}
		return changed;
	}

	/// @brief Manual Ribbonの既定制御点を編集する
	/// @param trail 編集対象Trail設定
	/// @param selectedControlPointIndex 選択中制御点Index
	/// @return 値を変更した場合はtrue
	bool DrawManualControlPointEditor(
		RibbonTrailModule& trail,
		int& selectedControlPointIndex) {
		bool changed = false;
		std::vector<Vector3>& controlPoints = trail.defaultControlPoints;
		ImGui::TextDisabled(
			"Assetの既定形状です。ゲーム実行中はSetControlPoints()で上書きできます。"
		);
		if (trail.simulationSpace == RibbonSimulationSpace::Local) {
			ImGui::TextDisabled("ローカル空間: 座標へRibbonのTransformが適用されます。");
		} else {
			ImGui::TextDisabled("ワールド空間: 座標をワールド座標としてそのまま使用します。");
		}

		if (controlPoints.empty()) {
			selectedControlPointIndex = -1;
		} else {
			selectedControlPointIndex = std::clamp(
				selectedControlPointIndex,
				0,
				static_cast<int>(controlPoints.size()) - 1
			);
		}
		const bool hasSelection = selectedControlPointIndex >= 0;
		constexpr float operationButtonWidth = 72.0f;

		ImGui::BeginDisabled(controlPoints.size() >= trail.maxPointCount);
		if (ImGui::Button("追加", ImVec2(operationButtonWidth, 0.0f))) {
			const Vector3 newPoint = hasSelection
				? controlPoints[selectedControlPointIndex] + Vector3{ 1.0f, 0.0f, 0.0f }
				: controlPoints.empty()
				? Vector3{}
				: controlPoints.back() + Vector3{ 1.0f, 0.0f, 0.0f };
			selectedControlPointIndex = hasSelection
				? selectedControlPointIndex + 1
				: static_cast<int>(controlPoints.size());
			controlPoints.insert(
				controlPoints.begin() + selectedControlPointIndex,
				newPoint
			);
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!hasSelection || controlPoints.size() >= trail.maxPointCount);
		if (ImGui::Button("複製", ImVec2(operationButtonWidth, 0.0f))) {
			const Vector3 duplicatedPoint = controlPoints[selectedControlPointIndex];
			++selectedControlPointIndex;
			controlPoints.insert(
				controlPoints.begin() + selectedControlPointIndex,
				duplicatedPoint
			);
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!hasSelection || selectedControlPointIndex == 0);
		if (ImGui::Button("上へ", ImVec2(operationButtonWidth, 0.0f))) {
			std::swap(
				controlPoints[selectedControlPointIndex],
				controlPoints[selectedControlPointIndex - 1]
			);
			--selectedControlPointIndex;
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(
			!hasSelection ||
			selectedControlPointIndex + 1 >= static_cast<int>(controlPoints.size())
		);
		if (ImGui::Button("下へ", ImVec2(operationButtonWidth, 0.0f))) {
			std::swap(
				controlPoints[selectedControlPointIndex],
				controlPoints[selectedControlPointIndex + 1]
			);
			++selectedControlPointIndex;
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!hasSelection);
		if (ImGui::Button("削除", ImVec2(operationButtonWidth, 0.0f))) {
			controlPoints.erase(controlPoints.begin() + selectedControlPointIndex);
			if (controlPoints.empty()) {
				selectedControlPointIndex = -1;
			} else {
				selectedControlPointIndex = (std::min)(
					selectedControlPointIndex,
					static_cast<int>(controlPoints.size()) - 1
				);
			}
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(controlPoints.empty());
		if (ImGui::Button("全消去", ImVec2(operationButtonWidth, 0.0f))) {
			ImGui::OpenPopup("制御点をすべて消去##ManualControlPointClearConfirmation");
		}
		ImGui::EndDisabled();

		const char* selectedPointLabel = selectedControlPointIndex >= 0
			? "一覧から座標を直接編集できます。"
			: "制御点を追加してください。";
		ImGui::TextDisabled(
			"制御点数: %zu / %u  |  %s",
			controlPoints.size(),
			trail.maxPointCount,
			selectedPointLabel
		);

		const float tableHeight = std::clamp(
			(static_cast<float>(controlPoints.size()) + 1.0f) * ImGui::GetTextLineHeightWithSpacing() + 8.0f,
			100.0f,
			300.0f
		);
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_ScrollY;
		if (ImGui::BeginTable("ManualControlPointTable", 4, tableFlags, ImVec2(0.0f, tableHeight))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("制御点", ImGuiTableColumnFlags_WidthFixed, 96.0f);
			ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			for (int index = 0; index < static_cast<int>(controlPoints.size()); ++index) {
				ImGui::PushID(index);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const std::string pointLabel = "制御点 " + std::to_string(index + 1);
				if (ImGui::Selectable(pointLabel.c_str(), selectedControlPointIndex == index)) {
					selectedControlPointIndex = index;
				}
				float* components[] = {
					&controlPoints[index].x,
					&controlPoints[index].y,
					&controlPoints[index].z,
				};
				for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
					ImGui::TableSetColumnIndex(componentIndex + 1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					ImGui::PushID(componentIndex);
					if (ImGui::DragFloat("##Value", components[componentIndex], 0.01f, 0.0f, 0.0f, "%.3f")) {
						selectedControlPointIndex = index;
						changed = true;
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (ImGui::BeginPopupModal(
			"制御点をすべて消去##ManualControlPointClearConfirmation",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("既定制御点をすべて消去しますか？");
			if (ImGui::Button("消去する")) {
				controlPoints.clear();
				selectedControlPointIndex = -1;
				changed = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (controlPoints.size() < kMinimumRibbonPointCount) {
			ImGui::TextColored(
				ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
				"描画には2点以上の制御点が必要です。"
			);
		}
		return changed;
	}

	/// @brief RibbonのPoint生成設定を編集する
	/// @param config 編集対象設定
	/// @param selectedControlPointIndex 選択中制御点Index
	/// @return 値を変更した場合はtrue
	bool DrawTrailEditor(RibbonEffectConfig& config, int& selectedControlPointIndex) {
		bool changed = false;
		const char* generationModeNames[] = { "Transform履歴", "手動制御点" };
		int generationModeIndex = static_cast<int>(config.trail.generationMode);
		if (ImGui::Combo(
			"制御点の生成方式",
			&generationModeIndex,
			generationModeNames,
			static_cast<int>(std::size(generationModeNames)))) {
			config.trail.generationMode = static_cast<RibbonPointGenerationMode>(generationModeIndex);
			changed = true;
		}

		const char* simulationSpaceNames[] = { "ワールド", "ローカル" };
		int simulationSpaceIndex = static_cast<int>(config.trail.simulationSpace);
		if (ImGui::Combo(
			"シミュレーション空間",
			&simulationSpaceIndex,
			simulationSpaceNames,
			static_cast<int>(std::size(simulationSpaceNames)))) {
			config.trail.simulationSpace = static_cast<RibbonSimulationSpace>(simulationSpaceIndex);
			changed = true;
		}

		changed |= ImGui::DragFloat(
			"制御点の生存時間",
			&config.trail.pointLifetime,
			0.01f,
			0.001f,
			3600.0f,
			"%.3f秒"
		);
		changed |= ImGui::DragFloat(
			"最小生成距離",
			&config.trail.minPointDistance,
			0.001f,
			0.0f,
			100000.0f,
			"%.3f"
		);
		int maxPointCount = static_cast<int>(config.trail.maxPointCount);
		if (ImGui::DragInt("最大制御点数", &maxPointCount, 1.0f, 2, 4096)) {
			config.trail.maxPointCount = static_cast<uint32_t>(std::clamp(maxPointCount, 2, 4096));
			changed = true;
		}
		if (config.trail.generationMode == RibbonPointGenerationMode::Manual) {
			ImGui::SeparatorText("既定制御点");
			changed |= DrawManualControlPointEditor(config.trail, selectedControlPointIndex);
		}
		return changed;
	}

	/// @brief Ribbonの形状設定を編集する
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawGeometryEditor(RibbonEffectConfig& config) {
		bool changed = false;
		const char* interpolationModeNames[] = { "線形", "Catmull-Rom" };
		int interpolationModeIndex = static_cast<int>(config.geometry.interpolation);
		if (ImGui::Combo(
			"補間方式",
			&interpolationModeIndex,
			interpolationModeNames,
			static_cast<int>(std::size(interpolationModeNames)))) {
			config.geometry.interpolation = static_cast<RibbonInterpolationMode>(interpolationModeIndex);
			changed = true;
		}

		int smoothingSubdivision = static_cast<int>(config.geometry.smoothingSubdivision);
		if (ImGui::DragInt("補間分割数", &smoothingSubdivision, 1.0f, 0, 32)) {
			config.geometry.smoothingSubdivision = static_cast<uint32_t>(
				std::clamp(smoothingSubdivision, 0, 32)
			);
			changed = true;
		}
		changed |= ImGui::Checkbox("カメラへ正対", &config.geometry.cameraFacing);
		ImGui::SeparatorText("寿命トラック");
		changed |= DrawFloatTrack(
			"Ribbon幅",
			config.geometry.widthOverLifetime,
			1.0f,
			"%.3f",
			0.01f,
			0.0f,
			100000.0f
		);
		ImGui::TextDisabled("時刻0が生成直後、時刻1が寿命終了時です。");
		return changed;
	}

	/// @brief RibbonのMaterial設定を編集する
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawMaterialEditor(RibbonEffectConfig& config) {
		bool changed = false;
		const MadoEngine::Editor::TextureSelector textureSelector;
		changed |= textureSelector.Draw("テクスチャ", config.material.textureName);

		const char* blendModeNames[] = { "通常", "加算", "減算", "乗算", "ブレンドなし" };
		int blendModeIndex = static_cast<int>(config.material.blendMode);
		if (ImGui::Combo(
			"ブレンドモード",
			&blendModeIndex,
			blendModeNames,
			static_cast<int>(std::size(blendModeNames)))) {
			config.material.blendMode = static_cast<MadoEngine::Render::BlendMode>(blendModeIndex);
			changed = true;
		}

		const char* cullModeNames[] = { "なし", "前面", "背面" };
		int cullModeIndex = static_cast<int>(config.material.cullMode);
		if (ImGui::Combo(
			"カリングモード",
			&cullModeIndex,
			cullModeNames,
			static_cast<int>(std::size(cullModeNames)))) {
			config.material.cullMode = static_cast<MadoEngine::Render::CullMode>(cullModeIndex);
			changed = true;
		}

		ImGui::SeparatorText("アニメーショントラック");
		changed |= DrawColorTrack("寿命による色", config.material.colorOverLifetime, 1.0f);
		changed |= DrawFloatTrack(
			"全体の不透明度",
			config.material.globalAlpha,
			(std::max)(config.playback.duration, 0.001f),
			"%.3f秒",
			0.01f,
			0.0f,
			1.0f
		);
		return changed;
	}

	/// @brief RibbonのUV設定を編集する
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawUvEditor(RibbonEffectConfig& config) {
		bool changed = false;
		const char* uvModeNames[] = { "全体へ伸長", "距離でタイル" };
		int uvModeIndex = static_cast<int>(config.material.uvMode);
		if (ImGui::Combo(
			"UV配置方式",
			&uvModeIndex,
			uvModeNames,
			static_cast<int>(std::size(uvModeNames)))) {
			config.material.uvMode = static_cast<RibbonUvMode>(uvModeIndex);
			changed = true;
		}
		changed |= ImGui::DragFloat2("UVスケール", &config.material.uvScale.x, 0.01f);
		changed |= ImGui::DragFloat2("UVオフセット", &config.material.uvOffset.x, 0.01f);
		changed |= ImGui::DragFloat2("UVスクロール速度", &config.material.uvScroll.x, 0.01f);
		if (config.material.uvMode == RibbonUvMode::Tile) {
			changed |= ImGui::DragFloat(
				"タイル1枚の長さ",
				&config.material.tileLength,
				0.01f,
				0.001f,
				100000.0f,
				"%.3f"
			);
		}
		return changed;
	}

	/// @brief Ribbon Assetの編集状態比較用Snapshotを生成する
	/// @param asset Snapshotを生成するAsset
	/// @return JSON形式Snapshot
	std::string CreateRibbonAssetSnapshot(const RibbonEffectAsset& asset) {
		return asset.ToJson().dump();
	}

	/// @brief Ribbon Previewを再生する
	/// @param system 再生に使用するSystem
	/// @param assetName 再生するAsset名
	/// @param previewPosition Preview基準位置
	/// @param isLoop Loop再生する場合はtrue
	/// @return 再生したRibbon Handle
	RibbonEffectHandle PlayRibbonPreview(
		RibbonEffectSystem3d& system,
		const std::string& assetName,
		const Vector3& previewPosition,
		bool isLoop) {
		RibbonEffectPlayDesc desc;
		desc.sceneType = SceneType::None;
		desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
		desc.loopOverride = isLoop;
		desc.transform.translate = previewPosition;
		return system.Play(assetName, desc);
	}

	/// @brief Ribbon Previewを即時停止して状態を消去する
	/// @param system 停止に使用するSystem
	/// @param handle 停止するHandle
	/// @param assetName Preview中Asset名
	/// @param assetSnapshot Preview開始時Snapshot
	void StopRibbonPreview(
		RibbonEffectSystem3d& system,
		RibbonEffectHandle& handle,
		std::string& assetName,
		std::string& assetSnapshot) {
		if (system.IsAlive(handle)) {
			system.Stop(handle, RibbonStopMode::Immediate);
		}
		handle = {};
		assetName.clear();
		assetSnapshot.clear();
	}

#endif // USE_IMGUI

} // namespace

namespace MadoEngine::Editor {

	void DrawRibbonEffectEditorUI() {
#ifdef USE_IMGUI
		RibbonEffectSystem3d& system = RibbonEffectSystem3d::GetInstance();
		static int selectedAssetIndex = 0;
		static int selectedSettingPage = 0;
		static int selectedManualControlPointIndex = 0;
		static RibbonEffectHandle previewHandle;
		static std::string previewAssetName;
		static std::string previewAssetSnapshot;
		static Vector3 previewPosition = { 0.0f, 1.0f, 0.0f };
		static bool previewLoop = true;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::string assetRenameOriginalName;
		static std::unordered_map<std::string, std::string> savedAssetSnapshots;
		static bool isNameBufferInitialized = false;
		if (!isNameBufferInitialized) {
			CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, "新規Ribbon"));
			isNameBufferInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(980.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f, 520.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("Ribbonエディター")) {
			ImGui::End();
			return;
		}

		if (!system.IsAlive(previewHandle)) {
			previewHandle = {};
			previewAssetName.clear();
			previewAssetSnapshot.clear();
		}

		std::vector<std::string> assetNames = system.GetAssetNames();
		if (!assetNames.empty()) {
			selectedAssetIndex = std::clamp(
				selectedAssetIndex,
				0,
				static_cast<int>(assetNames.size()) - 1
			);
		}
		std::string selectedAssetName = assetNames.empty()
			? std::string{}
			: assetNames[selectedAssetIndex];

		if (ImGui::CollapsingHeader("アセットの作成・複製", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent();
			ImGui::TextUnformatted("新規アセット名");
			ImGui::SetNextItemWidth((std::max)(180.0f, ImGui::GetContentRegionAvail().x - 220.0f));
			ImGui::InputText(
				"##NewRibbonAssetName",
				newAssetNameBuffer.data(),
				newAssetNameBuffer.size()
			);
			const std::string newAssetName = newAssetNameBuffer.data();
			const bool isNewAssetNameEmpty = newAssetName.empty();
			const bool isNewAssetNameAvailable = system.IsAssetNameAvailable(newAssetName);

			ImGui::SameLine();
			ImGui::BeginDisabled(isNewAssetNameEmpty || !isNewAssetNameAvailable);
			if (ImGui::Button("新規作成")) {
				if (system.CreateAsset(newAssetName)) {
					StopRibbonPreview(
						system,
						previewHandle,
						previewAssetName,
						previewAssetSnapshot
					);
					assetNames = system.GetAssetNames();
					const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
					selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
					selectedAssetName = newAssetName;
					assetRenameOriginalName.clear();
					if (const RibbonEffectAsset* createdAsset = system.FindAsset(newAssetName)) {
						savedAssetSnapshots[newAssetName] = CreateRibbonAssetSnapshot(*createdAsset);
					}
					CopyToBuffer(
						newAssetNameBuffer,
						MakeAvailableAssetName(system, "新規Ribbon")
					);
				}
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(
				selectedAssetName.empty() ||
				isNewAssetNameEmpty ||
				!isNewAssetNameAvailable
			);
			if (ImGui::Button("選択中を複製")) {
				if (system.DuplicateAsset(selectedAssetName, newAssetName)) {
					StopRibbonPreview(
						system,
						previewHandle,
						previewAssetName,
						previewAssetSnapshot
					);
					assetNames = system.GetAssetNames();
					const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
					selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
					selectedAssetName = newAssetName;
					assetRenameOriginalName.clear();
					if (const RibbonEffectAsset* duplicatedAsset = system.FindAsset(newAssetName)) {
						savedAssetSnapshots[newAssetName] = CreateRibbonAssetSnapshot(*duplicatedAsset);
					}
					CopyToBuffer(
						newAssetNameBuffer,
						MakeAvailableAssetName(system, "新規Ribbon")
					);
				}
			}
			ImGui::EndDisabled();

			if (isNewAssetNameEmpty) {
				ImGui::TextDisabled("新規アセット名を入力してください。");
			} else if (!isNewAssetNameAvailable) {
				ImGui::TextDisabled("同名のアセットが存在するか、ファイル名に使用できない文字が含まれています。");
			}
			ImGui::Unindent();
		}

		if (assetNames.empty()) {
			ImGui::TextDisabled("編集するRibbonエフェクトアセットを作成してください。");
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("編集中のアセット");
		selectedAssetIndex = std::clamp(
			selectedAssetIndex,
			0,
			static_cast<int>(assetNames.size()) - 1
		);
		selectedAssetName = assetNames[selectedAssetIndex];
		std::string assetComboPreview = selectedAssetName;
		if (const RibbonEffectAsset* selectedAsset = system.FindAsset(selectedAssetName)) {
			const auto saved = savedAssetSnapshots.find(selectedAssetName);
			if (
				saved != savedAssetSnapshots.end() &&
				saved->second != CreateRibbonAssetSnapshot(*selectedAsset)) {
				assetComboPreview += " *";
			}
		}
		ImGui::SetNextItemWidth((std::max)(240.0f, ImGui::GetContentRegionAvail().x * 0.5f));
		if (ImGui::BeginCombo("アセット", assetComboPreview.c_str())) {
			for (int index = 0; index < static_cast<int>(assetNames.size()); ++index) {
				std::string displayName = assetNames[index];
				if (const RibbonEffectAsset* listedAsset = system.FindAsset(assetNames[index])) {
					const auto saved = savedAssetSnapshots.find(assetNames[index]);
					if (
						saved != savedAssetSnapshots.end() &&
						saved->second != CreateRibbonAssetSnapshot(*listedAsset)) {
						displayName += " *";
					}
				}
				const bool isSelected = index == selectedAssetIndex;
				if (ImGui::Selectable(displayName.c_str(), isSelected)) {
					selectedAssetIndex = index;
					selectedAssetName = assetNames[index];
					assetRenameOriginalName.clear();
					StopRibbonPreview(
						system,
						previewHandle,
						previewAssetName,
						previewAssetSnapshot
					);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::CollapsingHeader("アセット名の変更・削除", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent();
			if (assetRenameOriginalName != selectedAssetName) {
				CopyToBuffer(renameAssetNameBuffer, selectedAssetName);
				assetRenameOriginalName = selectedAssetName;
			}
			ImGui::SetNextItemWidth(260.0f);
			ImGui::InputText(
				"アセット名",
				renameAssetNameBuffer.data(),
				renameAssetNameBuffer.size()
			);
			const std::string renameAssetName = renameAssetNameBuffer.data();
			const bool isAssetRenameChanged = renameAssetName != selectedAssetName;
			const bool isAssetRenameAvailable = system.IsAssetNameAvailable(renameAssetName);
			ImGui::SameLine();
			ImGui::BeginDisabled(
				renameAssetName.empty() ||
				!isAssetRenameChanged ||
				!isAssetRenameAvailable
			);
			if (ImGui::Button("アセット名を変更")) {
				const std::string oldAssetName = selectedAssetName;
				StopRibbonPreview(
					system,
					previewHandle,
					previewAssetName,
					previewAssetSnapshot
				);
				if (system.RenameAsset(oldAssetName, renameAssetName)) {
					savedAssetSnapshots.erase(oldAssetName);
					assetNames = system.GetAssetNames();
					const auto selected = std::find(assetNames.begin(), assetNames.end(), renameAssetName);
					selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
					selectedAssetName = renameAssetName;
					assetRenameOriginalName = renameAssetName;
					if (const RibbonEffectAsset* renamedAsset = system.FindAsset(renameAssetName)) {
						savedAssetSnapshots[renameAssetName] = CreateRibbonAssetSnapshot(*renamedAsset);
					}
				}
			}
			ImGui::EndDisabled();
			if (isAssetRenameChanged && !renameAssetName.empty() && !isAssetRenameAvailable) {
				ImGui::TextDisabled("変更後の名前は使用できません。");
			}

			if (ImGui::Button("アセットを削除")) {
				ImGui::OpenPopup("RibbonAssetDeleteConfirmation");
			}
			if (ImGui::BeginPopupModal(
				"RibbonAssetDeleteConfirmation",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("「%s」を削除しますか？", selectedAssetName.c_str());
				ImGui::TextDisabled("JSONファイルは.trashディレクトリへ退避されます。");
				if (ImGui::Button("削除する")) {
					StopRibbonPreview(
						system,
						previewHandle,
						previewAssetName,
						previewAssetSnapshot
					);
					const std::string deletedAssetName = selectedAssetName;
					if (system.DeleteAsset(deletedAssetName)) {
						savedAssetSnapshots.erase(deletedAssetName);
						assetNames = system.GetAssetNames();
						selectedAssetIndex = assetNames.empty()
							? 0
							: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
						selectedAssetName = assetNames.empty()
							? std::string{}
							: assetNames[selectedAssetIndex];
						assetRenameOriginalName.clear();
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("キャンセル")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::Unindent();
		}

		if (assetNames.empty()) {
			ImGui::TextDisabled("Ribbonエフェクトアセットがありません。新規作成してください。");
			ImGui::End();
			return;
		}

		RibbonEffectAsset* asset = system.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::TextDisabled("選択したRibbonエフェクトアセットを取得できませんでした。");
			ImGui::End();
			return;
		}
		savedAssetSnapshots.try_emplace(
			selectedAssetName,
			CreateRibbonAssetSnapshot(*asset)
		);
		bool isDirty = savedAssetSnapshots[selectedAssetName] != CreateRibbonAssetSnapshot(*asset);

		if (ImGui::Button("アセットを保存")) {
			asset->Validate();
			if (asset->SaveToFile({}, true)) {
				savedAssetSnapshots[selectedAssetName] = CreateRibbonAssetSnapshot(*asset);
				isDirty = false;
			}
		}
		ImGui::SameLine();
		bool reloadRequested = false;
		if (ImGui::Button("再読み込み")) {
			if (isDirty) {
				ImGui::OpenPopup("RibbonAssetReloadConfirmation");
			} else {
				reloadRequested = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("バックアップを読み込み")) {
			ImGui::OpenPopup("RibbonAssetBackupConfirmation");
		}
		ImGui::SameLine();
		if (isDirty) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "未保存");
		} else {
			ImGui::TextDisabled("保存済み");
		}

		if (ImGui::BeginPopupModal(
			"RibbonAssetReloadConfirmation",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("未保存の変更を破棄して再読み込みしますか？");
			if (ImGui::Button("再読み込みする")) {
				reloadRequested = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		bool backupRequested = false;
		if (ImGui::BeginPopupModal(
			"RibbonAssetBackupConfirmation",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("現在の編集内容を破棄してバックアップを読み込みますか？");
			ImGui::TextDisabled("読み込んだ内容は保存するまでJSONへ反映されません。");
			if (ImGui::Button("バックアップを読み込む")) {
				backupRequested = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (reloadRequested || backupRequested) {
			StopRibbonPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			const bool loaded = backupRequested
				? system.LoadAssetBackup(selectedAssetName)
				: system.ReloadAsset(selectedAssetName);
			if (loaded) {
				asset = system.FindEditableAsset(selectedAssetName);
				if (asset && reloadRequested) {
					savedAssetSnapshots[selectedAssetName] = CreateRibbonAssetSnapshot(*asset);
				}
			}
		}

		if (!asset) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("プレビュー");
		bool previewLoopChanged = false;
		if (ImGui::BeginTable("RibbonPreviewSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableSetupColumn("再生", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3(
				"プレビュー基準位置",
				&previewPosition.x,
				0.05f
			);
			ImGui::TableSetColumnIndex(1);
			previewLoopChanged = ImGui::Checkbox("プレビューをループ", &previewLoop);
			ImGui::EndTable();
		}

		if (ImGui::Button("プレビューを再生")) {
			StopRibbonPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			previewHandle = PlayRibbonPreview(
				system,
				selectedAssetName,
				previewPosition,
				previewLoop
			);
			if (system.IsAlive(previewHandle)) {
				previewAssetName = selectedAssetName;
				previewAssetSnapshot = CreateRibbonAssetSnapshot(*asset);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("プレビューを停止")) {
			StopRibbonPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
		}
		ImGui::SameLine();
		if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "再生中");
		} else {
			ImGui::TextDisabled("停止中");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("再生中: %zu", system.GetActiveEffectCount());

		bool assetChanged = false;
		RibbonEffectConfig& config = asset->GetConfig();
		ImGui::SeparatorText("設定");
		const char* settingPageNames[] = { "基本", "トレイル", "形状", "マテリアル", "UV" };
		const float settingPageButtonWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 4.0f) /
			static_cast<float>(std::size(settingPageNames));
		ImGui::PushID("RibbonSettingPageButtons");
		for (int pageIndex = 0; pageIndex < static_cast<int>(std::size(settingPageNames)); ++pageIndex) {
			ImGui::PushID(pageIndex);
			if (pageIndex > 0) {
				ImGui::SameLine();
			}
			const bool isSelectedPage = selectedSettingPage == pageIndex;
			if (isSelectedPage) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			if (ImGui::Button(settingPageNames[pageIndex], ImVec2(settingPageButtonWidth, 0.0f))) {
				selectedSettingPage = pageIndex;
			}
			if (isSelectedPage) {
				ImGui::PopStyleColor();
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::BeginChild("RibbonSettingPane", ImVec2(0.0f, 0.0f), true);
		switch (selectedSettingPage) {
		case 1:
			assetChanged |= DrawTrailEditor(config, selectedManualControlPointIndex);
			break;
		case 2:
			assetChanged |= DrawGeometryEditor(config);
			break;
		case 3:
			assetChanged |= DrawMaterialEditor(config);
			break;
		case 4:
			assetChanged |= DrawUvEditor(config);
			break;
		case 0:
		default:
			assetChanged |= DrawBasicEditor(config);
			break;
		}
		ImGui::EndChild();

		if (assetChanged) {
			asset->Validate();
		}

		const std::string currentAssetSnapshot = CreateRibbonAssetSnapshot(*asset);
		if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
			const bool isAssetChanged = previewAssetSnapshot != currentAssetSnapshot;
			if (isAssetChanged || previewLoopChanged) {
				StopRibbonPreview(
					system,
					previewHandle,
					previewAssetName,
					previewAssetSnapshot
				);
				previewHandle = PlayRibbonPreview(
					system,
					selectedAssetName,
					previewPosition,
					previewLoop
				);
				if (system.IsAlive(previewHandle)) {
					previewAssetName = selectedAssetName;
					previewAssetSnapshot = currentAssetSnapshot;
				}
			} else if (config.trail.generationMode == RibbonPointGenerationMode::TransformHistory) {
				const float time = static_cast<float>(ImGui::GetTime());
				Transform3D previewTransform;
				previewTransform.translate = {
					previewPosition.x + std::sin(time * 1.7f) * 3.0f,
					previewPosition.y + std::sin(time * 2.3f) * 0.75f,
					previewPosition.z + std::cos(time * 1.7f) * 1.5f,
				};
				system.SetTransform(previewHandle, previewTransform);
			} else {
				Transform3D previewTransform;
				previewTransform.translate = previewPosition;
				system.SetTransform(previewHandle, previewTransform);
			}
		}
		ImGui::End();
#endif // USE_IMGUI
	}

} // namespace MadoEngine::Editor
