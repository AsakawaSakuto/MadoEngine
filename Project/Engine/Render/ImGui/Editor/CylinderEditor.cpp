#include "CylinderEditor.h"
#include "EffectAssetEditorCommon.h"
#include "EffectEmitterEditorCommon.h"
#include "TextureSelector.h"
#include "ImGuiHeaders.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectSystem3d.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::Effect;

	/// @brief Track Editor内でKeyframeを識別するIDを発行
	/// @return 新しいEditor用ID
	uint32_t AllocateKeyframeEditorId() {
		static uint32_t nextId = 1;
		const uint32_t id = nextId++;
		if (nextId == 0) {
			nextId = 1;
		}
		return id;
	}

	/// @brief 文字列を固定長Bufferへコピー
	/// @tparam Size Bufferの要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 新規Cylinder Assetに使用できる名前を生成
	/// @param system 名前の使用状況を確認するPrimitive Effect System
	/// @param createdName 初期名または直前に作成した名前
	/// @return 使用可能なCylinder Asset名
	std::string MakeAvailableCylinderAssetName(
		const PrimitiveEffectSystem3d& system,
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
			const char* suffixBegin = createdName.data() + suffixStart;
			const char* suffixEnd = createdName.data() + createdName.size();
			const std::from_chars_result result = std::from_chars(suffixBegin, suffixEnd, suffix);
			if (result.ec == std::errc{} && result.ptr == suffixEnd) {
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

	/// @brief イージング種類を選択するComboを描画
	/// @param label UI表示名
	/// @param easing 編集対象のイージング種類
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

	/// @brief Keyframe追加に使用できる空き時刻を算出
	/// @tparam T Track値型
	/// @param keyframes 時刻順のKeyframe一覧
	/// @param maximumTime Track時刻上限
	/// @param preferredIndex 優先して近傍を検索するKeyframe Index
	/// @return 追加可能な時刻、空きがない場合はstd::nullopt
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

	/// @brief 型付きエフェクトトラックを編集
	/// @tparam T トラック値の型
	/// @tparam ValueDrawer 値編集UIを描画する関数型
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @param drawValue 値編集UIを描画する関数
	/// @param treeNodeFlags トラック見出しへ追加するTreeNodeフラグ
	/// @return トラックを変更した場合はtrue
	template<class T, class ValueDrawer>
	bool DrawTrackEditor(
		const char* label,
		EffectTrack<T>& track,
		float duration,
		ValueDrawer drawValue,
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_None) {
		bool changed = false;
		static std::unordered_map<const void*, int> selectedKeyframeIndices;
		static std::unordered_map<const void*, std::vector<uint32_t>> keyframeIds;
		const void* trackKey = static_cast<const void*>(&track);
		int& selectedKeyframeIndex = selectedKeyframeIndices[trackKey];
		ImGui::PushID(label);
		const bool isOpen = ImGui::TreeNodeEx(
			"トラック",
			ImGuiTreeNodeFlags_SpanAvailWidth | treeNodeFlags,
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
			std::vector<uint32_t>& editorIds = keyframeIds[trackKey];
			while (editorIds.size() < keyframes.size()) {
				editorIds.push_back(AllocateKeyframeEditorId());
			}
			if (editorIds.size() > keyframes.size()) {
				editorIds.resize(keyframes.size());
			}
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
				duration,
				selectedKeyframeIndex
			);
			std::optional<int> requestedSelectionIndex;
			constexpr float operationButtonWidth = 72.0f;

			ImGui::BeginDisabled(!insertionTime.has_value());
			if (ImGui::Button("追加", ImVec2(operationButtonWidth, 0.0f))) {
				EffectKeyframe<T> keyframe;
				keyframe.time = insertionTime.value();
				keyframe.value = track.Evaluate(keyframe.time);
				keyframe.easing = EaseType::Linear;
				requestedSelectionIndex = static_cast<int>(keyframes.size());
				keyframes.push_back(keyframe);
				editorIds.push_back(AllocateKeyframeEditorId());
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!hasSelection || !insertionTime.has_value());
			if (ImGui::Button("複製", ImVec2(operationButtonWidth, 0.0f))) {
				EffectKeyframe<T> keyframe = keyframes[selectedKeyframeIndex];
				keyframe.time = insertionTime.value();
				requestedSelectionIndex = static_cast<int>(keyframes.size());
				keyframes.push_back(keyframe);
				editorIds.push_back(AllocateKeyframeEditorId());
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!hasSelection);
			if (ImGui::Button("削除", ImVec2(operationButtonWidth, 0.0f))) {
				keyframes.erase(keyframes.begin() + selectedKeyframeIndex);
				editorIds.erase(editorIds.begin() + selectedKeyframeIndex);
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
			ImGui::TextDisabled("時刻は前後のキーフレームの範囲内に制限されます。");

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
					const float minimumKeyframeTime = index > 0
						? keyframes[index - 1].time
						: 0.0f;
					const float requestedMaximumKeyframeTime = index + 1 < static_cast<int>(keyframes.size())
						? keyframes[index + 1].time
						: (std::max)(duration, 0.001f);
					const float maximumKeyframeTime = (std::max)(
						minimumKeyframeTime,
						requestedMaximumKeyframeTime
					);
					ImGui::PushID(static_cast<int>(editorIds[index]));
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
						minimumKeyframeTime,
						maximumKeyframeTime,
						"%.3f秒"
					)) {
						keyframe.time = std::clamp(
							std::isfinite(keyframe.time) ? keyframe.time : minimumKeyframeTime,
							minimumKeyframeTime,
							maximumKeyframeTime
						);
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
					editorIds.clear();
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
				if (
					requestedSelectionIndex.has_value() &&
					requestedSelectionIndex.value() >= 0 &&
					requestedSelectionIndex.value() < static_cast<int>(keyframes.size())) {
					EffectKeyframe<T> selectedKeyframe = std::move(
						keyframes[requestedSelectionIndex.value()]
					);
					const uint32_t selectedEditorId = editorIds[requestedSelectionIndex.value()];
					keyframes.erase(keyframes.begin() + requestedSelectionIndex.value());
					editorIds.erase(editorIds.begin() + requestedSelectionIndex.value());
					const auto insertion = std::upper_bound(
						keyframes.begin(),
						keyframes.end(),
						selectedKeyframe.time,
						[](float time, const EffectKeyframe<T>& keyframe) {
							return time < keyframe.time;
						}
					);
					selectedKeyframeIndex = static_cast<int>(std::distance(keyframes.begin(), insertion));
					keyframes.insert(insertion, std::move(selectedKeyframe));
					editorIds.insert(
						editorIds.begin() + selectedKeyframeIndex,
						selectedEditorId
					);
				}
				track.SetKeyframes(std::move(keyframes));
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return changed;
	}

	/// @brief floatトラックを編集
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @param speed Drag操作速度
	/// @param minimum 最小値
	/// @param maximum 最大値
	/// @return トラックを変更した場合はtrue
	bool DrawFloatTrack(
		const char* label,
		EffectTrack<float>& track,
		float duration,
		float speed,
		float minimum,
		float maximum) {
		return DrawTrackEditor(
			label,
			track,
			duration,
			[speed, minimum, maximum](const char* valueLabel, float& value) {
				return ImGui::DragFloat(valueLabel, &value, speed, minimum, maximum, "%.3f");
			}
		);
	}

	/// @brief Vector2トラックを編集
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @param speed Drag操作速度
	/// @param minimum 最小値
	/// @param maximum 最大値
	/// @return トラックを変更した場合はtrue
	bool DrawVector2Track(
		const char* label,
		EffectTrack<Vector2>& track,
		float duration,
		float speed,
		float minimum,
		float maximum) {
		return DrawTrackEditor(
			label,
			track,
			duration,
			[speed, minimum, maximum](const char* valueLabel, Vector2& value) {
				return ImGui::DragFloat2(valueLabel, &value.x, speed, minimum, maximum, "%.3f");
			}
		);
	}

	/// @brief 色トラックを編集
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @param defaultOpen 初回表示時にトラックを展開する場合はtrue
	/// @return トラックを変更した場合はtrue
	bool DrawColorTrack(
		const char* label,
		EffectTrack<Vector4>& track,
		float duration,
		bool defaultOpen = false) {
		return DrawTrackEditor(
			label,
			track,
			duration,
			[](const char* valueLabel, Vector4& value) {
				return ImGui::ColorEdit4(
					valueLabel,
					&value.x,
					ImGuiColorEditFlags_AlphaBar |
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_HDR
				);
			},
			defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None
		);
	}

	/// @brief Cylinderの基本設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawBasicEditor(CylinderEffectConfig& config) {
		bool changed = false;
		changed |= ImGui::DragFloat3(
			"エミッター位置オフセット",
			&config.translateOffset.x,
			0.01f
		);
		changed |= ImGui::DragFloat("再生時間", &config.duration, 0.01f, 0.001f, 3600.0f, "%.3f秒");
		changed |= ImGui::Checkbox("ループ再生", &config.isLoop);
		return changed;
	}

	/// @brief Cylinderの形状設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawGeometryEditor(CylinderEffectConfig& config) {
		bool changed = false;
		CylinderGeometryModule& geometry = config.geometry;

		int radialSegments = static_cast<int>(geometry.radialSegments);
		if (ImGui::DragInt("円周分割数", &radialSegments, 1.0f, 3, 256)) {
			geometry.radialSegments = static_cast<uint32_t>(std::clamp(radialSegments, 3, 256));
			changed = true;
		}

		int heightSegments = static_cast<int>(geometry.heightSegments);
		if (ImGui::DragInt("高さ分割数", &heightSegments, 1.0f, 1, 64)) {
			geometry.heightSegments = static_cast<uint32_t>(std::clamp(heightSegments, 1, 64));
			changed = true;
		}

		const char* pivotNames[] = { "下端", "中央", "上端" };
		int pivotIndex = static_cast<int>(geometry.pivot);
		if (ImGui::Combo("基準位置", &pivotIndex, pivotNames, static_cast<int>(std::size(pivotNames)))) {
			geometry.pivot = static_cast<CylinderPivot>(pivotIndex);
			changed = true;
		}

		ImGui::SeparatorText("アニメーショントラック");
		changed |= DrawVector2Track("下端半径", geometry.bottomRadii, config.duration, 0.01f, 0.0f, 10000.0f);
		changed |= DrawVector2Track("上端半径", geometry.topRadii, config.duration, 0.01f, 0.0f, 10000.0f);
		changed |= DrawFloatTrack("高さ", geometry.height, config.duration, 0.01f, 0.001f, 10000.0f);
		changed |= DrawFloatTrack("開始角度", geometry.startAngleDegrees, config.duration, 0.1f, -36000.0f, 36000.0f);
		changed |= DrawFloatTrack("円弧角度", geometry.arcAngleDegrees, config.duration, 0.1f, -360.0f, 360.0f);
		return changed;
	}

	/// @brief Cylinderのグラデーション設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawGradientEditor(CylinderEffectConfig& config);

	/// @brief Cylinderのマテリアル設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawMaterialEditor(CylinderEffectConfig& config) {
		bool changed = false;
		CylinderMaterialModule& material = config.material;

		const MadoEngine::Editor::TextureSelector textureSelector;
		changed |= textureSelector.Draw("テクスチャ", material.textureName);

		const char* blendModeNames[] = { "通常", "加算", "減算", "乗算", "ブレンドなし" };
		int blendModeIndex = static_cast<int>(material.blendMode);
		if (ImGui::Combo(
			"ブレンドモード",
			&blendModeIndex,
			blendModeNames,
			static_cast<int>(std::size(blendModeNames)))) {
			material.blendMode = static_cast<MadoEngine::Render::BlendMode>(blendModeIndex);
			changed = true;
		}

		const char* cullModeNames[] = { "なし", "前面", "背面" };
		int cullModeIndex = static_cast<int>(material.cullMode);
		if (ImGui::Combo(
			"カリングモード",
			&cullModeIndex,
			cullModeNames,
			static_cast<int>(std::size(cullModeNames)))) {
			material.cullMode = static_cast<MadoEngine::Render::CullMode>(cullModeIndex);
			changed = true;
		}

		ImGui::SeparatorText("アニメーショントラック");
		changed |= DrawFloatTrack("全体の不透明度", material.globalAlpha, config.duration, 0.01f, 0.0f, 1.0f);
		changed |= DrawFloatTrack("下端フェード範囲", material.bottomFadeRange, config.duration, 0.01f, 0.0f, 1.0f);
		changed |= DrawFloatTrack("上端フェード範囲", material.topFadeRange, config.duration, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("グラデーション");
		changed |= DrawGradientEditor(config);
		return changed;
	}

	/// @brief CylinderのUV設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawUvEditor(CylinderEffectConfig& config) {
		bool changed = false;
		CylinderUvModule& uv = config.material.uv;

		const char* directionNames[] = { "上から下", "下から上", "時計回り", "反時計回り" };
		int directionIndex = static_cast<int>(uv.direction);
		if (ImGui::Combo(
			"UV方向",
			&directionIndex,
			directionNames,
			static_cast<int>(std::size(directionNames)))) {
			uv.direction = static_cast<CylinderUvDirection>(directionIndex);
			changed = true;
		}

		ImGui::SeparatorText("アニメーショントラック");
		changed |= DrawVector2Track("UVスケール", uv.scale, config.duration, 0.01f, -100.0f, 100.0f);
		changed |= DrawVector2Track("UVオフセット", uv.offset, config.duration, 0.01f, -100.0f, 100.0f);
		changed |= DrawFloatTrack("UV回転角度", uv.rotationDegrees, config.duration, 0.1f, -36000.0f, 36000.0f);
		return changed;
	}

	/// @brief グラデーション停止点を追加できる位置を算出
	/// @param gradient 位置順に並んだグラデーション停止点
	/// @param preferredIndex 優先して隣接位置を探す停止点番号
	/// @return 追加可能な位置、空きがない場合はstd::nullopt
	std::optional<float> FindGradientStopInsertionPosition(
		const std::vector<CylinderColorStop>& gradient,
		int preferredIndex) {
		constexpr float minimumGap = 0.001f;
		if (gradient.empty()) {
			return 0.5f;
		}

		const auto midpointIfAvailable = [](float begin, float end) -> std::optional<float> {
			constexpr float requiredGap = 0.001f;
			if (end - begin <= requiredGap) {
				return std::nullopt;
			}
			return (begin + end) * 0.5f;
		};

		if (preferredIndex >= 0 && preferredIndex < static_cast<int>(gradient.size())) {
			const float selectedPosition = std::clamp(gradient[preferredIndex].position, 0.0f, 1.0f);
			const float nextPosition = preferredIndex + 1 < static_cast<int>(gradient.size())
				? std::clamp(gradient[preferredIndex + 1].position, 0.0f, 1.0f)
				: 1.0f;
			if (const std::optional<float> position = midpointIfAvailable(selectedPosition, nextPosition)) {
				return position;
			}

			const float previousPosition = preferredIndex > 0
				? std::clamp(gradient[preferredIndex - 1].position, 0.0f, 1.0f)
				: 0.0f;
			if (const std::optional<float> position = midpointIfAvailable(previousPosition, selectedPosition)) {
				return position;
			}
		}

		float largestGapBegin = 0.0f;
		float largestGapEnd = 0.0f;
		float previousPosition = 0.0f;
		for (const CylinderColorStop& stop : gradient) {
			const float position = std::clamp(stop.position, 0.0f, 1.0f);
			if (position - previousPosition > largestGapEnd - largestGapBegin) {
				largestGapBegin = previousPosition;
				largestGapEnd = position;
			}
			previousPosition = position;
		}
		if (1.0f - previousPosition > largestGapEnd - largestGapBegin) {
			largestGapBegin = previousPosition;
			largestGapEnd = 1.0f;
		}
		if (largestGapEnd - largestGapBegin <= minimumGap) {
			return std::nullopt;
		}
		return (largestGapBegin + largestGapEnd) * 0.5f;
	}

	/// @brief HDR色をグラデーションプレビュー用の色へ変換
	/// @param color 変換するHDR色
	/// @return 0から1へ制限したImGui描画色
	ImU32 ToGradientPreviewColor(const Vector4& color) {
		const auto normalize = [](float value) {
			return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
		};
		return ImGui::ColorConvertFloat4ToU32(ImVec4{
			normalize(color.x),
			normalize(color.y),
			normalize(color.z),
			normalize(color.w),
		});
	}

	/// @brief 指定時刻のCylinderグラデーションを描画
	/// @param gradient 描画するグラデーション停止点
	/// @param previewTime 色トラックを評価する時刻
	void DrawGradientPreview(
		const std::vector<CylinderColorStop>& gradient,
		float previewTime) {
		const float width = (std::max)(ImGui::GetContentRegionAvail().x, 1.0f);
		ImGui::InvisibleButton("##GradientPreview", ImVec2(width, 30.0f));
		const ImVec2 previewMinimum = ImGui::GetItemRectMin();
		const ImVec2 previewMaximum = ImGui::GetItemRectMax();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(
			previewMinimum,
			previewMaximum,
			ImGui::GetColorU32(ImGuiCol_FrameBg)
		);
		if (gradient.empty()) {
			drawList->AddRect(
				previewMinimum,
				previewMaximum,
				ImGui::GetColorU32(ImGuiCol_Border)
			);
			return;
		}

		const float previewWidth = previewMaximum.x - previewMinimum.x;
		const auto positionToX = [previewMinimum, previewWidth](float position) {
			return previewMinimum.x + std::clamp(position, 0.0f, 1.0f) * previewWidth;
		};
		const ImU32 firstColor = ToGradientPreviewColor(gradient.front().color.Evaluate(previewTime));
		const float firstX = positionToX(gradient.front().position);
		if (firstX > previewMinimum.x) {
			drawList->AddRectFilled(
				previewMinimum,
				ImVec2(firstX, previewMaximum.y),
				firstColor
			);
		}

		for (std::size_t index = 0; index + 1 < gradient.size(); ++index) {
			const float leftX = positionToX(gradient[index].position);
			const float rightX = positionToX(gradient[index + 1].position);
			if (rightX <= leftX) {
				continue;
			}
			const ImU32 leftColor = ToGradientPreviewColor(gradient[index].color.Evaluate(previewTime));
			const ImU32 rightColor = ToGradientPreviewColor(gradient[index + 1].color.Evaluate(previewTime));
			drawList->AddRectFilledMultiColor(
				ImVec2(leftX, previewMinimum.y),
				ImVec2(rightX, previewMaximum.y),
				leftColor,
				rightColor,
				rightColor,
				leftColor
			);
		}

		const ImU32 lastColor = ToGradientPreviewColor(gradient.back().color.Evaluate(previewTime));
		const float lastX = positionToX(gradient.back().position);
		if (lastX < previewMaximum.x) {
			drawList->AddRectFilled(
				ImVec2(lastX, previewMinimum.y),
				previewMaximum,
				lastColor
			);
		}
		drawList->AddRect(
			previewMinimum,
			previewMaximum,
			ImGui::GetColorU32(ImGuiCol_Border)
		);
	}

	/// @brief CylinderのGradient設定を編集
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawGradientEditor(CylinderEffectConfig& config) {
		bool changed = false;
		std::vector<CylinderColorStop>& gradient = config.material.gradient;
		static std::unordered_map<const void*, int> selectedStopIndices;
		static std::unordered_map<const void*, float> previewTimes;
		const void* gradientKey = static_cast<const void*>(&gradient);
		int& selectedStopIndex = selectedStopIndices[gradientKey];
		float& previewTime = previewTimes[gradientKey];
		if (gradient.empty()) {
			gradient.emplace_back();
			selectedStopIndex = 0;
			changed = true;
		}
		selectedStopIndex = std::clamp(
			selectedStopIndex,
			0,
			static_cast<int>(gradient.size()) - 1
		);
		const float maximumPreviewTime = (std::max)(config.duration, 0.001f);
		previewTime = std::clamp(
			std::isfinite(previewTime) ? previewTime : 0.0f,
			0.0f,
			maximumPreviewTime
		);
		const std::optional<float> insertionPosition = FindGradientStopInsertionPosition(
			gradient,
			selectedStopIndex
		);
		const bool canAdd = gradient.size() < kMaximumCylinderGradientStops && insertionPosition.has_value();
		const auto insertStop = [&gradient, &selectedStopIndex, &changed](CylinderColorStop stop) {
			const auto insertion = std::upper_bound(
				gradient.begin(),
				gradient.end(),
				stop.position,
				[](float position, const CylinderColorStop& existingStop) {
					return position < existingStop.position;
				}
			);
			selectedStopIndex = static_cast<int>(std::distance(gradient.begin(), insertion));
			gradient.insert(insertion, std::move(stop));
			changed = true;
		};

		ImGui::PushID(gradientKey);
		constexpr float operationButtonWidth = 72.0f;
		ImGui::BeginDisabled(!canAdd);
		if (ImGui::Button("追加", ImVec2(operationButtonWidth, 0.0f))) {
			CylinderColorStop stop;
			stop.position = insertionPosition.value();
			insertStop(std::move(stop));
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!canAdd);
		if (ImGui::Button("複製", ImVec2(operationButtonWidth, 0.0f))) {
			CylinderColorStop stop = gradient[selectedStopIndex];
			stop.position = insertionPosition.value();
			insertStop(std::move(stop));
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(gradient.size() <= 1);
		if (ImGui::Button("削除", ImVec2(operationButtonWidth, 0.0f))) {
			gradient.erase(gradient.begin() + selectedStopIndex);
			selectedStopIndex = (std::min)(
				selectedStopIndex,
				static_cast<int>(gradient.size()) - 1
			);
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("%zu / %u", gradient.size(), kMaximumCylinderGradientStops);

		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_ScrollY;
		const float tableHeight = std::clamp(
			(static_cast<float>(gradient.size()) + 1.0f) * ImGui::GetTextLineHeightWithSpacing() + 12.0f,
			100.0f,
			230.0f
		);
		if (ImGui::BeginTable("グラデーション停止点", 3, tableFlags, ImVec2(0.0f, tableHeight))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("停止点", ImGuiTableColumnFlags_WidthFixed, 105.0f);
			ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("既定色", ImGuiTableColumnFlags_WidthStretch, 1.6f);
			ImGui::TableHeadersRow();

			for (int index = 0; index < static_cast<int>(gradient.size()); ++index) {
				CylinderColorStop& stop = gradient[index];
				const float minimumPosition = index > 0 ? gradient[index - 1].position : 0.0f;
				const float maximumPosition = index + 1 < static_cast<int>(gradient.size())
					? gradient[index + 1].position
					: 1.0f;
				ImGui::PushID(index);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const std::string stopLabel = "停止点 " + std::to_string(index + 1);
				if (ImGui::Selectable(stopLabel.c_str(), selectedStopIndex == index)) {
					selectedStopIndex = index;
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat(
					"##Position",
					&stop.position,
					0.005f,
					minimumPosition,
					maximumPosition,
					"%.3f"
				)) {
					stop.position = std::clamp(
						std::isfinite(stop.position) ? stop.position : minimumPosition,
						minimumPosition,
						maximumPosition
					);
					selectedStopIndex = index;
					changed = true;
				}
				ImGui::TableSetColumnIndex(2);
				ImGui::SetNextItemWidth(-FLT_MIN);
				Vector4 defaultColor = stop.color.GetDefaultValue();
				if (ImGui::ColorEdit4(
					"##DefaultColor",
					&defaultColor.x,
					ImGuiColorEditFlags_AlphaBar |
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_HDR
				)) {
					stop.color.SetDefaultValue(defaultColor);
					selectedStopIndex = index;
					changed = true;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		ImGui::SeparatorText("時刻プレビュー");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat(
			"##GradientPreviewTime",
			&previewTime,
			0.0f,
			maximumPreviewTime,
			"%.3f秒"
		);
		DrawGradientPreview(gradient, previewTime);
		ImGui::TextDisabled("グラデーションは現在のプレビュー時刻で評価されます。");

		if (selectedStopIndex >= 0 && selectedStopIndex < static_cast<int>(gradient.size())) {
			ImGui::SeparatorText("選択中の停止点");
			ImGui::Text("停止点 %d / 位置 %.3f", selectedStopIndex + 1, gradient[selectedStopIndex].position);
			changed |= DrawColorTrack(
				"色アニメーション",
				gradient[selectedStopIndex].color,
				config.duration,
				true
			);
		}
		ImGui::PopID();
		return changed;
	}

	/// @brief Cylinder Assetの編集状態を比較するためのスナップショットを生成
	/// @param asset スナップショットを生成するCylinder Asset
	/// @return JSON形式のスナップショット
	std::string CreateCylinderAssetSnapshot(const CylinderEffectAsset& asset) {
		return asset.ToJson().dump();
	}

	/// @brief Cylinder Editorのプレビューを再生
	/// @param system 再生に使用するPrimitive Effect System
	/// @param assetName 再生するAsset名
	/// @param transform プレビューのTransform
	/// @param isLoop ループ再生する場合はtrue
	/// @return 再生したCylinder EffectのHandle
	PrimitiveEffectHandle PlayCylinderPreview(
		PrimitiveEffectSystem3d& system,
		const std::string& assetName,
		const Transform3D& transform,
		bool isLoop) {
		PrimitiveEffectPlayDesc desc;
		desc.transform = transform;
		desc.sceneType = SceneType::None;
		desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
		desc.loopOverride = isLoop;
		return system.Play(assetName, desc);
	}

	/// @brief Cylinder Editorのプレビューを即時停止して状態を消去
	/// @param system 停止に使用するPrimitive Effect System
	/// @param handle 停止するHandle
	/// @param assetName プレビュー中Asset名
	/// @param assetSnapshot プレビュー開始時のAssetスナップショット
	void StopCylinderPreview(
		PrimitiveEffectSystem3d& system,
		PrimitiveEffectHandle& handle,
		std::string& assetName,
		std::string& assetSnapshot) {
		if (system.IsAlive(handle)) {
			system.Stop(handle, PrimitiveEffectStopMode::Immediate);
		}
		handle = {};
		assetName.clear();
		assetSnapshot.clear();
	}

#endif // USE_IMGUI

} // namespace

namespace MadoEngine::Editor {

	void DrawCylinderEffectEditorUI() {
#ifdef USE_IMGUI
		PrimitiveEffectSystem3d& system = PrimitiveEffectSystem3d::GetInstance();

		// 選択、Preview、未保存SnapshotをFrame間で維持するEditor Session状態
		static int selectedAssetIndex = 0;
		static int selectedEmitterIndex = 0;
		static int selectedSettingPage = 0;
		static PrimitiveEffectHandle previewHandle;
		static std::string previewAssetName;
		static std::string previewAssetSnapshot;
		static Transform3D previewTransform;
		static bool previewLoop = true;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::array<char, 128> newEmitterNameBuffer{};
		static std::array<char, 128> renameEmitterNameBuffer{};
		static std::string emitterCreateAssetName;
		static std::string emitterRenameIdentity;
		static const CylinderEffectAsset* emitterSelectedAsset = nullptr;
		static std::string assetRenameOriginalName;
		static std::unordered_map<std::string, std::string> savedAssetSnapshots;
		static bool isNameBufferInitialized = false;
		if (!isNameBufferInitialized) {
			CopyToBuffer(
				newAssetNameBuffer,
				MakeAvailableCylinderAssetName(system, "Cylinder")
			);
			isNameBufferInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(980.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f, 520.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("CylinderEditor")) {
			ImGui::End();
			return;
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

		if (!selectedAssetName.empty() && assetRenameOriginalName != selectedAssetName) {
			CopyToBuffer(renameAssetNameBuffer, selectedAssetName);
			assetRenameOriginalName = selectedAssetName;
		}
		CylinderEffectAsset* asset = selectedAssetName.empty()
			? nullptr
			: system.FindEditableAsset(selectedAssetName);
		if (asset) {
			savedAssetSnapshots.try_emplace(
				selectedAssetName,
				CreateCylinderAssetSnapshot(*asset)
			);
		}
		bool isDirty = asset &&
			savedAssetSnapshots[selectedAssetName] != CreateCylinderAssetSnapshot(*asset);
		const std::string newAssetName = newAssetNameBuffer.data();
		const std::string renameAssetName = renameAssetNameBuffer.data();
		const Detail::EffectAssetManagementActions actions = Detail::DrawEffectAssetManagement(
			"CylinderAssets",
			assetNames,
			selectedAssetIndex,
			newAssetNameBuffer,
			renameAssetNameBuffer,
			newAssetName.empty(),
			system.IsAssetNameAvailable(newAssetName),
			renameAssetName.empty(),
			renameAssetName != selectedAssetName,
			system.IsAssetNameAvailable(renameAssetName),
			isDirty
		);

		if (actions.isSelectionChanged) {

			// Asset切替時に旧AssetのPreviewを停止して編集対象との不一致を防止
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			selectedAssetIndex = actions.selectedAssetIndex;
			selectedEmitterIndex = 0;
			assetRenameOriginalName.clear();
		}
		if (actions.isCreateRequested && system.CreateAsset(newAssetName)) {
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			assetNames = system.GetAssetNames();
			const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
			selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
			assetRenameOriginalName.clear();
			if (const CylinderEffectAsset* createdAsset = system.FindAsset(newAssetName)) {
				savedAssetSnapshots[newAssetName] = CreateCylinderAssetSnapshot(*createdAsset);
			}
			CopyToBuffer(
				newAssetNameBuffer,
				MakeAvailableCylinderAssetName(system, newAssetName)
			);
		} else if (
			actions.isDuplicateRequested &&
			system.DuplicateAsset(selectedAssetName, newAssetName)) {
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			assetNames = system.GetAssetNames();
			const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
			selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
			assetRenameOriginalName.clear();
			if (const CylinderEffectAsset* duplicatedAsset = system.FindAsset(newAssetName)) {
				savedAssetSnapshots[newAssetName] = CreateCylinderAssetSnapshot(*duplicatedAsset);
			}
			CopyToBuffer(
				newAssetNameBuffer,
				MakeAvailableCylinderAssetName(system, newAssetName)
			);
		} else if (actions.isRenameRequested) {
				const std::string oldAssetName = selectedAssetName;
				StopCylinderPreview(
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
					assetRenameOriginalName.clear();
					if (const CylinderEffectAsset* renamedAsset = system.FindAsset(renameAssetName)) {
						savedAssetSnapshots[renameAssetName] = CreateCylinderAssetSnapshot(*renamedAsset);
					}
				}
		}

		if (actions.isDeleteRequested && !assetNames.empty()) {
			selectedAssetName = assetNames[std::clamp(
				actions.selectedAssetIndex,
				0,
				static_cast<int>(assetNames.size()) - 1
			)];
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			if (system.DeleteAsset(selectedAssetName)) {
				savedAssetSnapshots.erase(selectedAssetName);
				assetNames = system.GetAssetNames();
				selectedAssetIndex = assetNames.empty()
					? 0
					: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
				assetRenameOriginalName.clear();
			}
		}
		if (assetNames.empty()) {
			ImGui::End();
			return;
		}

		selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
		selectedAssetName = assetNames[selectedAssetIndex];
		asset = system.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::TextDisabled("選択したCylinder Effect Assetを取得できませんでした。");
			ImGui::End();
			return;
		}
		savedAssetSnapshots.try_emplace(
			selectedAssetName,
			CreateCylinderAssetSnapshot(*asset)
		);
		isDirty = savedAssetSnapshots[selectedAssetName] != CreateCylinderAssetSnapshot(*asset);
		if (actions.isSaveRequested) {
			asset->Validate();
			if (asset->SaveToFile()) {
				savedAssetSnapshots[selectedAssetName] = CreateCylinderAssetSnapshot(*asset);
				isDirty = false;
			}
		}
		bool reloadRequested = false;
		if (actions.isLoadRequested) {
			if (isDirty) {
				ImGui::OpenPopup("CylinderAssetReloadConfirmation");
			} else {
				reloadRequested = true;
			}
		}

		if (ImGui::BeginPopupModal(
			"CylinderAssetReloadConfirmation",
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

		if (reloadRequested) {
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			if (system.ReloadAsset(selectedAssetName)) {
				asset = system.FindEditableAsset(selectedAssetName);
				if (asset) {
					savedAssetSnapshots[selectedAssetName] = CreateCylinderAssetSnapshot(*asset);
				}
			}
		}

		if (!asset) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("プレビュー");
		const ImGuiTableFlags previewTableFlags = ImGuiTableFlags_SizingStretchProp;
		bool previewLoopChanged = false;
		if (ImGui::BeginTable("CylinderPreviewSettings", 2, previewTableFlags)) {
			ImGui::TableSetupColumn("Transform", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableSetupColumn("再生", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("位置", &previewTransform.translate.x, 0.05f);
			ImGui::TableSetColumnIndex(1);
			previewLoopChanged = ImGui::Checkbox("プレビューをループ", &previewLoop);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			float rotationDegrees[3] = {
				previewTransform.rotate.x * 180.0f / std::numbers::pi_v<float>,
				previewTransform.rotate.y * 180.0f / std::numbers::pi_v<float>,
				previewTransform.rotate.z * 180.0f / std::numbers::pi_v<float>,
			};
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("回転", rotationDegrees, 0.5f, -360.0f, 360.0f, "%.1f度")) {
				previewTransform.rotate = {
					rotationDegrees[0] * std::numbers::pi_v<float> / 180.0f,
					rotationDegrees[1] * std::numbers::pi_v<float> / 180.0f,
					rotationDegrees[2] * std::numbers::pi_v<float> / 180.0f,
				};
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("スケール", &previewTransform.scale.x, 0.01f, 0.001f, 1000.0f, "%.3f");
			ImGui::EndTable();
		}

		if (ImGui::Button("プレビューを再生")) {
			StopCylinderPreview(
				system,
				previewHandle,
				previewAssetName,
				previewAssetSnapshot
			);
			previewHandle = PlayCylinderPreview(system, selectedAssetName, previewTransform, previewLoop);
			if (system.IsAlive(previewHandle)) {
				previewAssetName = selectedAssetName;
				previewAssetSnapshot = CreateCylinderAssetSnapshot(*asset);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("プレビューを停止")) {
			StopCylinderPreview(
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
		if (emitterSelectedAsset != asset) {
			selectedEmitterIndex = 0;
			emitterCreateAssetName.clear();
			emitterRenameIdentity.clear();
			emitterSelectedAsset = asset;
		}
		std::vector<CylinderEmitterConfig>& emitters = asset->GetEmitters();
		assetChanged |= Detail::DrawEffectEmitterListPane(
			"CylinderEmitters",
			selectedAssetName,
			emitters,
			kMaximumCylinderEmitterCount,
			selectedEmitterIndex,
			newEmitterNameBuffer,
			emitterCreateAssetName,
			renameEmitterNameBuffer,
			emitterRenameIdentity
		);
		CylinderEmitterConfig& config = emitters[selectedEmitterIndex];
		ImGui::SameLine();
		ImGui::BeginChild("CylinderEmitterSettingPane", ImVec2(0.0f, 0.0f), true);
		ImGui::Text("設定: %s", config.name.c_str());
		ImGui::Separator();
		const char* settingPageNames[] = { "基本", "形状", "マテリアル", "UV" };
		selectedSettingPage = std::clamp(
			selectedSettingPage,
			0,
			static_cast<int>(std::size(settingPageNames)) - 1
		);
		const float settingPageButtonWidth =
			(ImGui::GetContentRegionAvail().x -
				ImGui::GetStyle().ItemSpacing.x * static_cast<float>(std::size(settingPageNames) - 1)) /
			static_cast<float>(std::size(settingPageNames));
		ImGui::PushID("CylinderSettingPageButtons");
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

		ImGui::BeginChild("CylinderSettingScrollPane", ImVec2(0.0f, 0.0f), false);
		switch (selectedSettingPage) {
		case 1:
			assetChanged |= DrawGeometryEditor(config);
			break;
		case 2:
			assetChanged |= DrawMaterialEditor(config);
			break;
		case 3:
			assetChanged |= DrawUvEditor(config);
			break;
		case 0:
		default:
			assetChanged |= DrawBasicEditor(config);
			break;
		}
		ImGui::EndChild();
		ImGui::EndChild();

		if (assetChanged) {
			asset->Validate();
		}

		const std::string currentAssetSnapshot = CreateCylinderAssetSnapshot(*asset);
		if (
			system.IsAlive(previewHandle) &&
			previewAssetName == selectedAssetName) {
			const bool isAssetChanged = previewAssetSnapshot != currentAssetSnapshot;
			if (isAssetChanged || previewLoopChanged) {
				StopCylinderPreview(
					system,
					previewHandle,
					previewAssetName,
					previewAssetSnapshot
				);
				previewHandle = PlayCylinderPreview(system, selectedAssetName, previewTransform, previewLoop);
				if (system.IsAlive(previewHandle)) {
					previewAssetName = selectedAssetName;
					previewAssetSnapshot = currentAssetSnapshot;
				}
			} else {
				system.SetTransform(previewHandle, previewTransform);
			}
		}
		ImGui::End();
#endif // USE_IMGUI
	}

} // namespace MadoEngine::Editor
