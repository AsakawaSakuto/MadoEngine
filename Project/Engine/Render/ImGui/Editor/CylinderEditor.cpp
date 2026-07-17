#include "CylinderEditor.h"
#include "ImGuiHeaders.h"
#include "Core/TextureManager/TextureManager.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectSystem3d.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::Effect;

	/// @brief 文字列を固定長Bufferへコピーする
	/// @tparam Size Bufferの要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 新規Cylinder Assetに使用できる名前を生成する
	/// @param system 名前の使用状況を確認するPrimitive Effect System
	/// @param baseName 名前の基準文字列
	/// @return 使用可能なCylinder Asset名
	std::string MakeAvailableCylinderAssetName(
		const PrimitiveEffectSystem3d& system,
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

	/// @brief 型付きエフェクトトラックを編集する
	/// @tparam T トラック値の型
	/// @tparam ValueDrawer 値編集UIを描画する関数型
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @param drawValue 値編集UIを描画する関数
	/// @return トラックを変更した場合はtrue
	template<class T, class ValueDrawer>
	bool DrawTrackEditor(
		const char* label,
		EffectTrack<T>& track,
		float duration,
		ValueDrawer drawValue) {
		bool changed = false;
		ImGui::PushID(label);
		const bool isOpen = ImGui::TreeNodeEx("トラック", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label);
		if (isOpen) {
			T defaultValue = track.GetDefaultValue();
			if (drawValue("既定値", defaultValue)) {
				track.SetDefaultValue(defaultValue);
				changed = true;
			}

			std::vector<EffectKeyframe<T>> keyframes = track.GetKeyframes();
			int removeIndex = -1;
			const ImGuiTableFlags tableFlags =
				ImGuiTableFlags_BordersInnerV |
				ImGuiTableFlags_BordersInnerH |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingStretchProp;
			if (ImGui::BeginTable("KeyframeTable", 4, tableFlags)) {
				ImGui::TableSetupColumn("時刻", ImGuiTableColumnFlags_WidthStretch, 0.8f);
				ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch, 1.4f);
				ImGui::TableSetupColumn("イージング", ImGuiTableColumnFlags_WidthStretch, 1.2f);
				ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 56.0f);
				ImGui::TableHeadersRow();

				for (int index = 0; index < static_cast<int>(keyframes.size()); ++index) {
					EffectKeyframe<T>& keyframe = keyframes[index];
					ImGui::PushID(index);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::SetNextItemWidth(-FLT_MIN);
					changed |= ImGui::DragFloat(
						"##Time",
						&keyframe.time,
						0.01f,
						0.0f,
						(std::max)(duration, 0.001f),
						"%.3f秒"
					);
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					changed |= drawValue("##Value", keyframe.value);
					ImGui::TableSetColumnIndex(2);
					ImGui::SetNextItemWidth(-FLT_MIN);
					changed |= DrawEaseTypeCombo("##Easing", keyframe.easing);
					ImGui::TableSetColumnIndex(3);
					if (ImGui::SmallButton("削除")) {
						removeIndex = index;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			if (removeIndex >= 0) {
				keyframes.erase(keyframes.begin() + removeIndex);
				changed = true;
			}

			if (ImGui::Button("キーフレームを追加")) {
				const float interval = (std::max)(duration * 0.1f, 0.01f);
				const float keyTime = keyframes.empty()
					? 0.0f
					: (std::min)(duration, keyframes.back().time + interval);
				EffectKeyframe<T> keyframe;
				keyframe.time = keyTime;
				keyframe.value = track.Evaluate(keyTime);
				keyframe.easing = EaseType::Linear;
				keyframes.push_back(keyframe);
				changed = true;
			}

			if (changed) {
				track.SetKeyframes(std::move(keyframes));
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return changed;
	}

	/// @brief floatトラックを編集する
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

	/// @brief Vector2トラックを編集する
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

	/// @brief 色トラックを編集する
	/// @param label UI表示名
	/// @param track 編集対象トラック
	/// @param duration エフェクトの再生時間
	/// @return トラックを変更した場合はtrue
	bool DrawColorTrack(
		const char* label,
		EffectTrack<Vector4>& track,
		float duration) {
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
			}
		);
	}

	/// @brief Cylinderの基本設定を編集する
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawBasicEditor(CylinderEffectConfig& config) {
		bool changed = false;
		changed |= ImGui::DragFloat("再生時間", &config.duration, 0.01f, 0.001f, 3600.0f, "%.3f秒");
		changed |= ImGui::Checkbox("ループ再生", &config.isLoop);
		return changed;
	}

	/// @brief Cylinderの形状設定を編集する
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

	/// @brief Cylinderのマテリアル設定を編集する
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawMaterialEditor(CylinderEffectConfig& config) {
		bool changed = false;
		CylinderMaterialModule& material = config.material;

		const std::vector<std::string> textureNames = MadoEngine::TextureManager::GetInstance().GetTextureNames();
		if (ImGui::BeginCombo("テクスチャ", material.textureName.c_str())) {
			for (const std::string& textureName : textureNames) {
				const bool isSelected = textureName == material.textureName;
				if (ImGui::Selectable(textureName.c_str(), isSelected)) {
					material.textureName = textureName;
					changed = true;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

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
		return changed;
	}

	/// @brief CylinderのUV設定を編集する
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

	/// @brief CylinderのGradient設定を編集する
	/// @param config 編集対象設定
	/// @return 設定を変更した場合はtrue
	bool DrawGradientEditor(CylinderEffectConfig& config) {
		bool changed = false;
		std::vector<CylinderColorStop>& gradient = config.material.gradient;
		int removeIndex = -1;

		for (int index = 0; index < static_cast<int>(gradient.size()); ++index) {
			CylinderColorStop& stop = gradient[index];
			ImGui::PushID(index);
			const bool isOpen = ImGui::TreeNodeEx(
				"GradientStop",
				ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
				"停止点 %d",
				index + 1
			);
			if (isOpen) {
				changed |= ImGui::DragFloat("位置", &stop.position, 0.01f, 0.0f, 1.0f, "%.3f");
				changed |= DrawColorTrack("色", stop.color, config.duration);
				ImGui::BeginDisabled(gradient.size() <= 1);
				if (ImGui::Button("停止点を削除")) {
					removeIndex = index;
				}
				ImGui::EndDisabled();
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		if (removeIndex >= 0) {
			gradient.erase(gradient.begin() + removeIndex);
			changed = true;
		}

		ImGui::BeginDisabled(gradient.size() >= kMaximumCylinderGradientStops);
		if (ImGui::Button("停止点を追加")) {
			CylinderColorStop stop;
			if (!gradient.empty()) {
				stop.position = (std::min)(1.0f, gradient.back().position + 0.1f);
				stop.color.SetDefaultValue(gradient.back().color.GetDefaultValue());
			} else {
				stop.position = 0.5f;
			}
			gradient.push_back(std::move(stop));
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled(
			"%zu / %u",
			gradient.size(),
			kMaximumCylinderGradientStops
		);
		return changed;
	}

	/// @brief Cylinder Assetの編集状態を比較するためのスナップショットを生成する
	/// @param asset スナップショットを生成するCylinder Asset
	/// @return JSON形式のスナップショット
	std::string CreateCylinderAssetSnapshot(const CylinderEffectAsset& asset) {
		return asset.ToJson().dump();
	}

	/// @brief Cylinder Editorのプレビューを再生する
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

	/// @brief Cylinder Editorのプレビューを即時停止して状態を消去する
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
		static int selectedAssetIndex = 0;
		static int selectedSettingPage = 0;
		static PrimitiveEffectHandle previewHandle;
		static std::string previewAssetName;
		static std::string previewAssetSnapshot;
		static Transform3D previewTransform;
		static bool previewLoop = true;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::string assetRenameOriginalName;
		static std::unordered_map<std::string, std::string> savedAssetSnapshots;
		static bool isNameBufferInitialized = false;
		if (!isNameBufferInitialized) {
			CopyToBuffer(
				newAssetNameBuffer,
				MakeAvailableCylinderAssetName(system, "新規Cylinder")
			);
			isNameBufferInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(980.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f, 520.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("Cylinderエディター")) {
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

		const ImGuiTreeNodeFlags assetCreationHeaderFlags = assetNames.empty()
			? ImGuiTreeNodeFlags_DefaultOpen
			: ImGuiTreeNodeFlags_None;
		if (ImGui::CollapsingHeader("アセットの作成・複製", assetCreationHeaderFlags)) {
			ImGui::Indent();
			ImGui::TextUnformatted("新規アセット名");
			ImGui::SetNextItemWidth((std::max)(180.0f, ImGui::GetContentRegionAvail().x - 220.0f));
			ImGui::InputText(
				"##NewCylinderAssetName",
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
					StopCylinderPreview(
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
					if (const CylinderEffectAsset* createdAsset = system.FindAsset(newAssetName)) {
						savedAssetSnapshots[newAssetName] = CreateCylinderAssetSnapshot(*createdAsset);
					}
					CopyToBuffer(
						newAssetNameBuffer,
						MakeAvailableCylinderAssetName(system, "新規Cylinder")
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
					StopCylinderPreview(
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
					if (const CylinderEffectAsset* duplicatedAsset = system.FindAsset(newAssetName)) {
						savedAssetSnapshots[newAssetName] = CreateCylinderAssetSnapshot(*duplicatedAsset);
					}
					CopyToBuffer(
						newAssetNameBuffer,
						MakeAvailableCylinderAssetName(system, "新規Cylinder")
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
			ImGui::TextDisabled("編集するCylinder Effect Assetを作成してください。");
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
		if (const CylinderEffectAsset* selectedAsset = system.FindAsset(selectedAssetName)) {
			const auto saved = savedAssetSnapshots.find(selectedAssetName);
			if (
				saved != savedAssetSnapshots.end() &&
				saved->second != CreateCylinderAssetSnapshot(*selectedAsset)) {
				assetComboPreview += " *";
			}
		}
		ImGui::SetNextItemWidth((std::max)(240.0f, ImGui::GetContentRegionAvail().x * 0.5f));
		if (ImGui::BeginCombo("アセット", assetComboPreview.c_str())) {
			for (int index = 0; index < static_cast<int>(assetNames.size()); ++index) {
				std::string displayName = assetNames[index];
				if (const CylinderEffectAsset* listedAsset = system.FindAsset(assetNames[index])) {
					const auto saved = savedAssetSnapshots.find(assetNames[index]);
					if (
						saved != savedAssetSnapshots.end() &&
						saved->second != CreateCylinderAssetSnapshot(*listedAsset)) {
						displayName += " *";
					}
				}

				const bool isSelected = index == selectedAssetIndex;
				if (ImGui::Selectable(displayName.c_str(), isSelected)) {
					selectedAssetIndex = index;
					selectedAssetName = assetNames[index];
					assetRenameOriginalName.clear();
					StopCylinderPreview(
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

		if (ImGui::CollapsingHeader("アセット名の変更・削除")) {
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
					selectedAssetName = renameAssetName;
					assetRenameOriginalName = renameAssetName;
					if (const CylinderEffectAsset* renamedAsset = system.FindAsset(renameAssetName)) {
						savedAssetSnapshots[renameAssetName] = CreateCylinderAssetSnapshot(*renamedAsset);
					}
				}
			}
			ImGui::EndDisabled();
			if (isAssetRenameChanged && !renameAssetName.empty() && !isAssetRenameAvailable) {
				ImGui::TextDisabled("変更後の名前は使用できません。");
			}

			if (ImGui::Button("アセットを削除")) {
				ImGui::OpenPopup("CylinderAssetDeleteConfirmation");
			}
			if (ImGui::BeginPopupModal(
				"CylinderAssetDeleteConfirmation",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("「%s」を削除しますか？", selectedAssetName.c_str());
				ImGui::TextDisabled("Jsonファイルは.trashディレクトリへ退避されます。");
				if (ImGui::Button("削除する")) {
					StopCylinderPreview(
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
			ImGui::TextDisabled("Cylinder Effect Assetがありません。新規作成してください。");
			ImGui::End();
			return;
		}

		CylinderEffectAsset* asset = system.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::TextDisabled("選択したCylinder Effect Assetを取得できませんでした。");
			ImGui::End();
			return;
		}
		savedAssetSnapshots.try_emplace(
			selectedAssetName,
			CreateCylinderAssetSnapshot(*asset)
		);
		bool isDirty =
			savedAssetSnapshots[selectedAssetName] != CreateCylinderAssetSnapshot(*asset);

		if (ImGui::Button("アセットを保存")) {
			asset->Validate();
			if (asset->SaveToFile()) {
				savedAssetSnapshots[selectedAssetName] = CreateCylinderAssetSnapshot(*asset);
				isDirty = false;
			}
		}
		ImGui::SameLine();
		bool reloadRequested = false;
		if (ImGui::Button("再読み込み")) {
			if (isDirty) {
				ImGui::OpenPopup("CylinderAssetReloadConfirmation");
			} else {
				reloadRequested = true;
			}
		}
		ImGui::SameLine();
		if (isDirty) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "未保存");
		} else {
			ImGui::TextDisabled("保存済み");
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
		CylinderEffectConfig& config = asset->GetConfig();
		ImGui::SeparatorText("設定");
		const char* settingPageNames[] = { "基本", "形状", "マテリアル", "UV", "グラデーション" };
		const float settingPageButtonWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 4.0f) /
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

		ImGui::BeginChild("CylinderSettingPane", ImVec2(0.0f, 0.0f), true);
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
		case 4:
			assetChanged |= DrawGradientEditor(config);
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
