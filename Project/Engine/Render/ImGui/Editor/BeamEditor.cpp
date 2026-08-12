#include "BeamEditor.h"
#include "EffectAssetEditorCommon.h"
#include "EffectEmitterEditorCommon.h"
#include "ImGuiHeaders.h"
#include "Render/Object/3d/BeamEffect/BeamEffectSystem3d.h"
#include "TextureSelector.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::Beam;
	using MadoEngine::Effect::EffectKeyframe;
	using MadoEngine::Effect::EffectTrack;

	constexpr std::array<const char*, 42> kEaseTypeNames = {
		"Linear", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutInQuad",
		"EaseInCubic", "EaseOutCubic", "EaseInOutCubic", "EaseOutInCubic",
		"EaseInQuart", "EaseOutQuart", "EaseInOutQuart", "EaseOutInQuart",
		"EaseInQuint", "EaseOutQuint", "EaseInOutQuint", "EaseOutInQuint",
		"EaseInSine", "EaseOutSine", "EaseInOutSine", "EaseOutInSine",
		"EaseInExpo", "EaseOutExpo", "EaseInOutExpo", "EaseOutInExpo",
		"EaseInCirc", "EaseOutCirc", "EaseInOutCirc", "EaseOutInCirc",
		"EaseInBack", "EaseOutBack", "EaseInOutBack", "EaseOutInBack",
		"EaseInElastic", "EaseOutElastic", "EaseInOutElastic", "EaseOutInElastic",
		"EaseInBounce", "EaseOutBounce", "EaseInOutBounce", "EaseOutInBounce", "None",
	};

	/// @brief 文字列を固定長Bufferへコピー
	/// @tparam Size Buffer要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 使用可能なBeam Asset名を生成
	/// @param system 名前を確認するSystem
	/// @param createdName 初期名または直前に作成した名前
	/// @return 使用可能な名前
	std::string MakeAvailableAssetName(
		const BeamEffectSystem3d& system,
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

	/// @brief イージング種類を選択するComboを描画
	/// @param label UI表示名
	/// @param easing 編集対象イージング
	/// @return 値を変更した場合はtrue
	bool DrawEaseTypeCombo(const char* label, EaseType& easing) {
		int index = std::clamp(
			static_cast<int>(easing),
			0,
			static_cast<int>(kEaseTypeNames.size()) - 1
		);
		if (!ImGui::Combo(label, &index, kEaseTypeNames.data(), static_cast<int>(kEaseTypeNames.size()))) {
			return false;
		}
		easing = static_cast<EaseType>(index);
		return true;
	}

	/// @brief Keyframe追加に使う空き時刻を探索
	/// @tparam T Track値型
	/// @param keys 時刻順Keyframe
	/// @return 追加時刻、空きがない場合はstd::nullopt
	template<class T>
	std::optional<float> FindInsertionTime(const std::vector<EffectKeyframe<T>>& keys) {
		constexpr float minimumGap = 0.0001f;
		if (keys.empty()) {
			return 0.0f;
		}
		float gapStart = 0.0f;
		float gapEnd = keys.front().time;
		float largestGap = gapEnd - gapStart;
		for (std::size_t index = 0; index + 1 < keys.size(); ++index) {
			const float gap = keys[index + 1].time - keys[index].time;
			if (gap > largestGap) {
				gapStart = keys[index].time;
				gapEnd = keys[index + 1].time;
				largestGap = gap;
			}
		}
		if (1.0f - keys.back().time > largestGap) {
			gapStart = keys.back().time;
			gapEnd = 1.0f;
			largestGap = gapEnd - gapStart;
		}
		return largestGap > minimumGap
			? std::optional<float>{ (gapStart + gapEnd) * 0.5f }
			: std::nullopt;
	}

	/// @brief 操作しやすい表形式のEffect Track Editorを描画
	/// @tparam T Track値型
	/// @tparam ValueDrawer 値編集関数型
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @param drawValue 値編集関数
	/// @return 値を変更した場合はtrue
	template<class T, class ValueDrawer>
	bool DrawTrackEditor(
		const char* label,
		EffectTrack<T>& track,
		ValueDrawer drawValue) {
		bool changed = false;
		static std::unordered_map<const void*, int> selectedIndices;
		int& selectedIndex = selectedIndices[static_cast<const void*>(&track)];
		ImGui::PushID(label);
		if (ImGui::TreeNodeEx("トラック", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label)) {
			T defaultValue = track.GetDefaultValue();
			if (drawValue("既定値", defaultValue)) {
				track.SetDefaultValue(defaultValue);
				changed = true;
			}
			std::vector<EffectKeyframe<T>> keys = track.GetKeyframes();
			selectedIndex = keys.empty()
				? -1
				: std::clamp(selectedIndex, 0, static_cast<int>(keys.size()) - 1);
			const std::optional<float> insertionTime = FindInsertionTime(keys);
			ImGui::BeginDisabled(!insertionTime.has_value());
			if (ImGui::Button("追加", ImVec2(72.0f, 0.0f))) {
				const float time = insertionTime.value();
				keys.push_back({ time, track.Evaluate(time), EaseType::Linear });
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(selectedIndex < 0 || !insertionTime.has_value());
			if (ImGui::Button("複製", ImVec2(72.0f, 0.0f))) {
				EffectKeyframe<T> copy = keys[selectedIndex];
				copy.time = insertionTime.value();
				keys.push_back(copy);
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(selectedIndex < 0);
			if (ImGui::Button("削除", ImVec2(72.0f, 0.0f))) {
				keys.erase(keys.begin() + selectedIndex);
				selectedIndex = keys.empty()
					? -1
					: (std::min)(selectedIndex, static_cast<int>(keys.size()) - 1);
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextDisabled("%zu個", keys.size());
			ImGui::TextDisabled("時刻は0.0～1.0で、前後のキーフレームを越えません。");

			if (changed) {
				std::stable_sort(keys.begin(), keys.end(), [](const auto& lhs, const auto& rhs) {
					return lhs.time < rhs.time;
				});
			}
			const float tableHeight = std::clamp(
				(static_cast<float>(keys.size()) + 1.0f) * ImGui::GetTextLineHeightWithSpacing() + 8.0f,
				100.0f,
				260.0f
			);
			if (ImGui::BeginTable(
				"KeyframeTable",
				4,
				ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
				ImVec2(0.0f, tableHeight))) {
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("キーフレーム", ImGuiTableColumnFlags_WidthFixed, 112.0f);
				ImGui::TableSetupColumn("時刻", ImGuiTableColumnFlags_WidthStretch, 0.8f);
				ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch, 1.4f);
				ImGui::TableSetupColumn("イージング", ImGuiTableColumnFlags_WidthStretch, 1.2f);
				ImGui::TableHeadersRow();
				for (int index = 0; index < static_cast<int>(keys.size()); ++index) {
					EffectKeyframe<T>& key = keys[index];
					const float minimumTime = index > 0 ? keys[index - 1].time : 0.0f;
					const float maximumTime = index + 1 < static_cast<int>(keys.size())
						? keys[index + 1].time
						: 1.0f;
					ImGui::PushID(index);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					const std::string keyLabel = "キーフレーム " + std::to_string(index + 1);
					if (ImGui::Selectable(keyLabel.c_str(), selectedIndex == index)) {
						selectedIndex = index;
					}
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::DragFloat("##Time", &key.time, 0.01f, minimumTime, maximumTime, "%.3f")) {
						key.time = std::clamp(
							std::isfinite(key.time) ? key.time : minimumTime,
							minimumTime,
							maximumTime
						);
						selectedIndex = index;
						changed = true;
					}
					ImGui::TableSetColumnIndex(2);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (drawValue("##Value", key.value)) {
						selectedIndex = index;
						changed = true;
					}
					ImGui::TableSetColumnIndex(3);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (DrawEaseTypeCombo("##Easing", key.easing)) {
						selectedIndex = index;
						changed = true;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			if (changed) {
				track.SetKeyframes(std::move(keys));
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return changed;
	}

	/// @brief float Track編集UIを描画
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @param minimumValue 値下限
	/// @param maximumValue 値上限
	/// @return 値を変更した場合はtrue
	bool DrawFloatTrack(
		const char* label,
		EffectTrack<float>& track,
		float minimumValue,
		float maximumValue) {
		return DrawTrackEditor(label, track, [minimumValue, maximumValue](const char* valueLabel, float& value) {
			return ImGui::DragFloat(valueLabel, &value, 0.01f, minimumValue, maximumValue, "%.3f");
		});
	}

	/// @brief Color Track編集UIを描画
	/// @param label UI表示名
	/// @param track 編集対象Track
	/// @return 値を変更した場合はtrue
	bool DrawColorTrack(const char* label, EffectTrack<Vector4>& track) {
		return DrawTrackEditor(label, track, [](const char* valueLabel, Vector4& value) {
			return ImGui::ColorEdit4(
				valueLabel,
				&value.x,
				ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR
			);
		});
	}

	/// @brief 基本設定を編集
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawBasicEditor(BeamEffectConfig& config) {
		bool changed = false;
		changed |= ImGui::DragFloat3(
			"エミッター位置オフセット",
			&config.translateOffset.x,
			0.01f
		);
		changed |= ImGui::DragFloat("再生時間", &config.playback.duration, 0.01f, 0.001f, 3600.0f, "%.3f秒");
		changed |= ImGui::Checkbox("ループ再生", &config.playback.isLoop);
		ImGui::SeparatorText("再生時間トラック");
		changed |= DrawFloatTrack(
			"始点から終点への伸長率",
			config.playback.extensionOverTime,
			0.0f,
			1.0f
		);
		changed |= DrawFloatTrack("Beam幅", config.geometry.widthOverTime, 0.0f, 100000.0f);
		ImGui::TextDisabled("伸長率0で非表示、1で終点まで表示します。時刻は正規化再生時間です。");
		return changed;
	}

	/// @brief 形状設定を編集
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawGeometryEditor(BeamEffectConfig& config) {
		bool changed = false;
		int segmentCount = static_cast<int>(config.geometry.segmentCount);
		if (ImGui::DragInt("分割数", &segmentCount, 1.0f, 1, 1024)) {
			config.geometry.segmentCount = static_cast<uint32_t>(std::clamp(segmentCount, 1, 1024));
			changed = true;
		}
		changed |= ImGui::Checkbox("カメラへ正対", &config.geometry.cameraFacing);
		changed |= ImGui::SliderFloat("始端フェード", &config.geometry.startFade, 0.0f, 1.0f, "%.3f");
		changed |= ImGui::SliderFloat("終端フェード", &config.geometry.endFade, 0.0f, 1.0f, "%.3f");
		ImGui::TextDisabled("フェード値はBeam全長に対する割合です。");
		return changed;
	}

	/// @brief Noise設定を編集
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawNoiseEditor(BeamEffectConfig& config) {
		bool changed = false;
		changed |= ImGui::DragFloat("振幅", &config.noise.amplitude, 0.01f, 0.0f, 100000.0f, "%.3f");
		changed |= ImGui::DragFloat("周波数", &config.noise.frequency, 0.01f, 0.0f, 10000.0f, "%.3f");
		changed |= ImGui::DragFloat("スクロール速度", &config.noise.scrollSpeed, 0.01f, -10000.0f, 10000.0f, "%.3f");
		int seed = static_cast<int>((std::min)(config.noise.seed, static_cast<uint32_t>((std::numeric_limits<int>::max)())));
		if (ImGui::DragInt("シード", &seed, 1.0f, 0, (std::numeric_limits<int>::max)())) {
			config.noise.seed = static_cast<uint32_t>((std::max)(seed, 0));
			changed = true;
		}
		ImGui::TextDisabled("振幅0では直線になります。始点と終点はNoiseの影響を受けません。");
		return changed;
	}

	/// @brief Material設定を編集
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawMaterialEditor(BeamEffectConfig& config) {
		bool changed = false;
		const MadoEngine::Editor::TextureSelector textureSelector;
		changed |= textureSelector.Draw("テクスチャ", config.material.textureName);
		const char* blendModeNames[] = { "通常", "加算", "減算", "乗算", "ブレンドなし" };
		int blendMode = static_cast<int>(config.material.blendMode);
		if (ImGui::Combo("ブレンドモード", &blendMode, blendModeNames, static_cast<int>(std::size(blendModeNames)))) {
			config.material.blendMode = static_cast<MadoEngine::Render::BlendMode>(blendMode);
			changed = true;
		}
		const char* cullModeNames[] = { "なし", "前面", "背面" };
		int cullMode = static_cast<int>(config.material.cullMode);
		if (ImGui::Combo("カリングモード", &cullMode, cullModeNames, static_cast<int>(std::size(cullModeNames)))) {
			config.material.cullMode = static_cast<MadoEngine::Render::CullMode>(cullMode);
			changed = true;
		}
		ImGui::SeparatorText("アニメーショントラック");
		changed |= DrawColorTrack("再生時間による全体色", config.material.colorOverTime);
		changed |= DrawColorTrack("始点から終点への色", config.material.colorOverLength);
		changed |= DrawFloatTrack("全体の不透明度", config.material.globalAlphaOverTime, 0.0f, 1.0f);
		ImGui::TextDisabled("長さ方向色は時刻0が始点、時刻1が終点です。全体色と乗算されます。");
		return changed;
	}

	/// @brief UV設定を編集
	/// @param config 編集対象設定
	/// @return 値を変更した場合はtrue
	bool DrawUvEditor(BeamEffectConfig& config) {
		bool changed = false;
		const char* uvModeNames[] = { "全体へ伸長", "距離でタイル" };
		int uvMode = static_cast<int>(config.material.uvMode);
		if (ImGui::Combo("UV配置方式", &uvMode, uvModeNames, static_cast<int>(std::size(uvModeNames)))) {
			config.material.uvMode = static_cast<MadoEngine::Ribbon::RibbonUvMode>(uvMode);
			changed = true;
		}
		changed |= ImGui::DragFloat2("UVスケール", &config.material.uvScale.x, 0.01f);
		changed |= ImGui::DragFloat2("UVオフセット", &config.material.uvOffset.x, 0.01f);
		changed |= ImGui::DragFloat2("UVスクロール速度", &config.material.uvScroll.x, 0.01f);
		if (config.material.uvMode == MadoEngine::Ribbon::RibbonUvMode::Tile) {
			changed |= ImGui::DragFloat("タイル1枚の長さ", &config.material.tileLength, 0.01f, 0.001f, 100000.0f, "%.3f");
		}
		return changed;
	}

	/// @brief Beam Previewを即時停止
	/// @param system 停止に使用するSystem
	/// @param handle 停止するHandle
	void StopPreview(BeamEffectSystem3d& system, BeamEffectHandle& handle) {
		if (system.IsAlive(handle)) {
			system.Stop(handle, BeamStopMode::Immediate);
		}
		handle = {};
	}

#endif // USE_IMGUI

} // namespace

namespace MadoEngine::Editor {

	void DrawBeamEffectEditorUI() {
#ifdef USE_IMGUI
		BeamEffectSystem3d& system = BeamEffectSystem3d::GetInstance();

		// 選択、Preview、未保存SnapshotをFrame間で維持するEditor Session状態
		static int selectedAssetIndex = 0;
		static int selectedEmitterIndex = 0;
		static int selectedPage = 0;
		static BeamEffectHandle previewHandle;
		static std::string previewAssetName;
		static Vector3 previewStart = { -3.0f, 1.0f, 0.0f };
		static Vector3 previewEnd = { 3.0f, 1.0f, 0.0f };
		static bool previewLoop = true;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::array<char, 128> newEmitterNameBuffer{};
		static std::array<char, 128> renameEmitterNameBuffer{};
		static std::string emitterCreateAssetName;
		static std::string emitterRenameIdentity;
		static const BeamEffectAsset* emitterSelectedAsset = nullptr;
		static std::string renameOriginalName;
		static std::unordered_map<std::string, std::string> savedSnapshots;
		static bool isNameInitialized = false;
		if (!isNameInitialized) {
			CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, "Beam"));
			isNameInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(980.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f, 520.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("BeamEditor")) {
			ImGui::End();
			return;
		}
		if (!system.IsAlive(previewHandle)) {

			// Runtime側で終了したPreview HandleにEditor状態を結び付けたままにしない同期
			previewHandle = {};
			previewAssetName.clear();
		}

		std::vector<std::string> assetNames = system.GetAssetNames();
		std::string selectedAssetName;
		if (!assetNames.empty()) {
			selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
			selectedAssetName = assetNames[selectedAssetIndex];
		}

		if (!selectedAssetName.empty() && renameOriginalName != selectedAssetName) {
			CopyToBuffer(renameAssetNameBuffer, selectedAssetName);
			renameOriginalName = selectedAssetName;
		}
		BeamEffectAsset* asset = selectedAssetName.empty()
			? nullptr
			: system.FindEditableAsset(selectedAssetName);
		if (asset) {
			savedSnapshots.try_emplace(selectedAssetName, asset->ToJson().dump());
		}
		bool isDirty = asset && savedSnapshots[selectedAssetName] != asset->ToJson().dump();
		const std::string newName = newAssetNameBuffer.data();
		const std::string renameName = renameAssetNameBuffer.data();
		const MadoEngine::Editor::Detail::EffectAssetManagementActions actions =
			MadoEngine::Editor::Detail::DrawEffectAssetManagement(
				"BeamAssets",
				assetNames,
				selectedAssetIndex,
				newAssetNameBuffer,
				renameAssetNameBuffer,
				newName.empty(),
				system.IsAssetNameAvailable(newName),
				renameName.empty(),
				renameName != selectedAssetName,
				system.IsAssetNameAvailable(renameName),
				isDirty
			);

		if (actions.isSelectionChanged) {

			// Asset切替時に旧AssetのPreviewを停止して編集対象との不一致を防止
			StopPreview(system, previewHandle);
			selectedAssetIndex = actions.selectedAssetIndex;
			renameOriginalName.clear();
		}
		if (actions.isCreateRequested && system.CreateAsset(newName)) {
			StopPreview(system, previewHandle);
			assetNames = system.GetAssetNames();
			selectedAssetIndex = static_cast<int>(std::distance(
				assetNames.begin(),
				std::find(assetNames.begin(), assetNames.end(), newName)
			));
			renameOriginalName.clear();
			CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, newName));
		} else if (actions.isDuplicateRequested && system.DuplicateAsset(selectedAssetName, newName)) {
			StopPreview(system, previewHandle);
			assetNames = system.GetAssetNames();
			selectedAssetIndex = static_cast<int>(std::distance(
				assetNames.begin(),
				std::find(assetNames.begin(), assetNames.end(), newName)
			));
			renameOriginalName.clear();
			CopyToBuffer(newAssetNameBuffer, MakeAvailableAssetName(system, newName));
		} else if (actions.isRenameRequested) {
			const std::string oldName = selectedAssetName;
			StopPreview(system, previewHandle);
			if (system.RenameAsset(oldName, renameName)) {
				savedSnapshots.erase(oldName);
				assetNames = system.GetAssetNames();
				selectedAssetIndex = static_cast<int>(std::distance(
					assetNames.begin(),
					std::find(assetNames.begin(), assetNames.end(), renameName)
				));
				renameOriginalName.clear();
			}
		}

		if (actions.isDeleteRequested && !assetNames.empty()) {
			selectedAssetName = assetNames[std::clamp(
				actions.selectedAssetIndex,
				0,
				static_cast<int>(assetNames.size()) - 1
			)];
			StopPreview(system, previewHandle);
			savedSnapshots.erase(selectedAssetName);
			system.DeleteAsset(selectedAssetName);
			assetNames = system.GetAssetNames();
			selectedAssetIndex = assetNames.empty()
				? 0
				: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
			renameOriginalName.clear();
		}
		if (assetNames.empty()) {
			ImGui::End();
			return;
		}

		selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
		selectedAssetName = assetNames[selectedAssetIndex];
		asset = system.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::End();
			return;
		}
		savedSnapshots.try_emplace(selectedAssetName, asset->ToJson().dump());
		isDirty = savedSnapshots[selectedAssetName] != asset->ToJson().dump();
		if (actions.isSaveRequested) {
			asset->Validate();
			if (asset->SaveToFile({}, true)) {
				savedSnapshots[selectedAssetName] = asset->ToJson().dump();
				isDirty = false;
			}
		}
		if (actions.isLoadRequested) {
			StopPreview(system, previewHandle);
			if (system.ReloadAsset(selectedAssetName)) {
				asset = system.FindEditableAsset(selectedAssetName);
				if (asset) {
					savedSnapshots[selectedAssetName] = asset->ToJson().dump();
				}
			}
		}
		ImGui::SeparatorText("プレビュー");
		if (ImGui::BeginTable("BeamPreviewSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("始点", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("終点", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("##PreviewStart", &previewStart.x, 0.05f);
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("##PreviewEnd", &previewEnd.x, 0.05f);
			ImGui::EndTable();
		}
		const bool previewLoopChanged = ImGui::Checkbox("プレビューをループ", &previewLoop);
		if (ImGui::Button("プレビューを再生")) {
			StopPreview(system, previewHandle);
			BeamEffectPlayDesc desc;
			desc.startPosition = previewStart;
			desc.endPosition = previewEnd;
			desc.loopOverride = previewLoop;
			desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
			previewHandle = system.Play(selectedAssetName, desc);
			if (system.IsAlive(previewHandle)) {
				previewAssetName = selectedAssetName;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("プレビューを停止")) {
			StopPreview(system, previewHandle);
			previewAssetName.clear();
		}
		ImGui::SameLine();
		if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "再生中");
		} else {
			ImGui::TextDisabled("停止中");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("再生中: %zu", system.GetActiveEffectCount());

		bool changed = false;
		if (emitterSelectedAsset != asset) {
			selectedEmitterIndex = 0;
			emitterCreateAssetName.clear();
			emitterRenameIdentity.clear();
			emitterSelectedAsset = asset;
		}
		std::vector<BeamEmitterConfig>& emitters = asset->GetEmitters();
		changed |= Detail::DrawEffectEmitterListPane(
			"BeamEmitters",
			selectedAssetName,
			emitters,
			kMaximumBeamEmitterCount,
			selectedEmitterIndex,
			newEmitterNameBuffer,
			emitterCreateAssetName,
			renameEmitterNameBuffer,
			emitterRenameIdentity
		);
		BeamEmitterConfig& config = emitters[selectedEmitterIndex];
		ImGui::SameLine();
		ImGui::BeginChild("BeamEmitterSettingPane", ImVec2(0.0f, 0.0f), true);
		ImGui::Text("設定: %s", config.name.c_str());
		ImGui::Separator();
		const char* pageNames[] = { "基本", "形状", "ノイズ", "マテリアル", "UV" };
		const float pageWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 4.0f) /
			static_cast<float>(std::size(pageNames));
		for (int index = 0; index < static_cast<int>(std::size(pageNames)); ++index) {
			ImGui::PushID(index);
			if (index > 0) {
				ImGui::SameLine();
			}
			const bool selected = selectedPage == index;
			if (selected) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			if (ImGui::Button(pageNames[index], ImVec2(pageWidth, 0.0f))) {
				selectedPage = index;
			}
			if (selected) {
				ImGui::PopStyleColor();
			}
			ImGui::PopID();
		}
		ImGui::BeginChild("BeamSettingScrollPane", ImVec2(0.0f, 0.0f), false);
		switch (selectedPage) {
		case 1: changed |= DrawGeometryEditor(config); break;
		case 2: changed |= DrawNoiseEditor(config); break;
		case 3: changed |= DrawMaterialEditor(config); break;
		case 4: changed |= DrawUvEditor(config); break;
		case 0:
		default: changed |= DrawBasicEditor(config); break;
		}
		ImGui::EndChild();
		ImGui::EndChild();
		if (changed) {
			asset->Validate();
		}
		if (system.IsAlive(previewHandle) && previewAssetName == selectedAssetName) {
			if (changed || previewLoopChanged) {
				StopPreview(system, previewHandle);
				BeamEffectPlayDesc desc;
				desc.startPosition = previewStart;
				desc.endPosition = previewEnd;
				desc.loopOverride = previewLoop;
				desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
				previewHandle = system.Play(selectedAssetName, desc);
				if (system.IsAlive(previewHandle)) {
					previewAssetName = selectedAssetName;
				}
			}
			system.SetEndpoints(previewHandle, previewStart, previewEnd);
		}
		ImGui::End();
#endif // USE_IMGUI
	}

} // namespace MadoEngine::Editor
