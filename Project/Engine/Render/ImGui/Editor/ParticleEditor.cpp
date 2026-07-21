#include "ParticleEditor.h"
#include "TextureSelector.h"
#include "ImGuiHeaders.h"
#include "Render/Object/3d/Particle/ParticleEmitterDebugDrawer3d.h"
#include "Render/Object/3d/Particle/ParticleSystem3d.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace {

#ifdef USE_IMGUI

	using namespace MadoEngine::Particle;

	/// @brief 指定したEmitter名が既に使用されているか確認する
	/// @param emitters 確認対象のEmitter一覧
	/// @param name 確認するEmitter名
	/// @param ignoredIndex 確認対象から除外するIndex。負数の場合は除外しない
	/// @return 使用済みの場合はtrue
	bool IsEmitterNameUsed(
		const std::vector<EmitterConfig>& emitters,
		const std::string& name,
		int ignoredIndex = -1) {
		for (int index = 0; index < static_cast<int>(emitters.size()); ++index) {
			if (index != ignoredIndex && emitters[index].name == name) {
				return true;
			}
		}

		return false;
	}

	/// @brief Emitter一覧内で重複しない名前を生成する
	/// @param emitters 名前の重複を確認するEmitter一覧
	/// @param baseName 名前の基準文字列
	/// @return 重複しないEmitter名
	std::string MakeUniqueEmitterName(
		const std::vector<EmitterConfig>& emitters,
		const std::string& baseName) {
		if (!IsEmitterNameUsed(emitters, baseName)) {
			return baseName;
		}

		for (uint32_t suffix = 2; ; ++suffix) {
			const std::string candidate = baseName + " (" + std::to_string(suffix) + ")";
			if (!IsEmitterNameUsed(emitters, candidate)) {
				return candidate;
			}
		}
	}

	/// @brief 新規Particle Assetに使用できる名前を生成する
	/// @param particleSystem 名前の使用状況を確認するParticleSystem
	/// @param baseName 名前の基準文字列
	/// @return 使用可能なParticle Asset名
	std::string MakeAvailableParticleAssetName(
		const ParticleSystem3d& particleSystem,
		const std::string& baseName) {
		if (particleSystem.IsAssetNameAvailable(baseName)) {
			return baseName;
		}

		for (uint32_t suffix = 2; suffix < 10000; ++suffix) {
			const std::string candidate = baseName + " (" + std::to_string(suffix) + ")";
			if (particleSystem.IsAssetNameAvailable(candidate)) {
				return candidate;
			}
		}

		return baseName;
	}

	/// @brief 文字列を固定長Bufferへコピーする
	/// @tparam Size Bufferの要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief float範囲を編集する
	/// @param label UI表示名
	/// @param range 編集対象範囲
	/// @param speed Drag操作速度
	/// @param minimum 最小値
	/// @param maximum 最大値
	void DrawFloatRange(
		const char* label,
		ValueRange<float>& range,
		float speed,
		float minimum = 0.0f,
		float maximum = 0.0f) {
		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::DragFloat("最小", &range.min, speed, minimum, maximum);
		ImGui::DragFloat("最大", &range.max, speed, minimum, maximum);
		ImGui::PopID();
	}

	/// @brief Vector2範囲を編集する
	/// @param label UI表示名
	/// @param range 編集対象範囲
	/// @param speed Drag操作速度
	void DrawVector2Range(const char* label, ValueRange<Vector2>& range, float speed) {
		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::DragFloat2("最小", &range.min.x, speed);
		ImGui::DragFloat2("最大", &range.max.x, speed);
		ImGui::PopID();
	}

	/// @brief Vector3範囲を編集する
	/// @param label UI表示名
	/// @param range 編集対象範囲
	/// @param speed Drag操作速度
	void DrawVector3Range(const char* label, ValueRange<Vector3>& range, float speed) {
		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::DragFloat3("最小", &range.min.x, speed);
		ImGui::DragFloat3("最大", &range.max.x, speed);
		ImGui::PopID();
	}

	/// @brief Vector4色範囲を編集する
	/// @param label UI表示名
	/// @param range 編集対象範囲
	void DrawColorRange(const char* label, ValueRange<Vector4>& range) {
		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::ColorEdit4("最小", &range.min.x);
		ImGui::ColorEdit4("最大", &range.max.x);
		ImGui::PopID();
	}

	/// @brief Emitter発生形状を編集する
	/// @param shape 編集対象形状
	void DrawShapeEditor(ParticleShape& shape) {
		const char* shapeNames[] = { "点", "線", "球", "ボックス", "平面", "リング" };
		int shapeIndex = 0;
		if (std::holds_alternative<LineShape>(shape)) { shapeIndex = 1; }
		if (std::holds_alternative<SphereShape>(shape)) { shapeIndex = 2; }
		if (std::holds_alternative<BoxShape>(shape)) { shapeIndex = 3; }
		if (std::holds_alternative<PlaneShape>(shape)) { shapeIndex = 4; }
		if (std::holds_alternative<RingShape>(shape)) { shapeIndex = 5; }

		if (ImGui::Combo("形状", &shapeIndex, shapeNames, static_cast<int>(std::size(shapeNames)))) {
			switch (shapeIndex) {
			case 1:
				shape = LineShape{};
				break;
			case 2:
				shape = SphereShape{};
				break;
			case 3:
				shape = BoxShape{};
				break;
			case 4:
				shape = PlaneShape{};
				break;
			case 5:
				shape = RingShape{};
				break;
			case 0:
			default:
				shape = PointShape{};
				break;
			}
		}

		std::visit([](auto& value) {
			using ShapeType = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<ShapeType, PointShape>) {
				ImGui::DragFloat3("オフセット", &value.offset.x, 0.01f);
			} else if constexpr (std::is_same_v<ShapeType, LineShape>) {
				ImGui::DragFloat3("開始位置", &value.start.x, 0.01f);
				ImGui::DragFloat3("終了位置", &value.end.x, 0.01f);
			} else if constexpr (std::is_same_v<ShapeType, SphereShape>) {
				ImGui::DragFloat("半径", &value.radius, 0.01f, 0.0f, FLT_MAX);
				ImGui::Checkbox("表面から発生", &value.emitFromSurface);
			} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
				ImGui::DragFloat3("半サイズ", &value.halfExtents.x, 0.01f, 0.0f, FLT_MAX);
				ImGui::Checkbox("表面から発生", &value.emitFromSurface);
			} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
				ImGui::DragFloat2("半サイズ", &value.halfExtents.x, 0.01f, 0.0f, FLT_MAX);
				ImGui::DragFloat3("法線", &value.normal.x, 0.01f);
			} else {
				ImGui::DragFloat("内側の半径", &value.innerRadius, 0.01f, 0.0f, FLT_MAX);
				ImGui::DragFloat("外側の半径", &value.outerRadius, 0.01f, 0.0f, FLT_MAX);
				ImGui::DragFloat3("法線", &value.normal.x, 0.01f);
				ImGui::Checkbox("外周から発生", &value.emitFromEdge);
			}
		}, shape);
	}

	/// @brief Emitter設定を編集する
	/// @param emitter 編集対象Emitter
	void DrawEmitterEditor(EmitterConfig& emitter) {
		static int selectedSettingPage = 0;
		const char* settingPageNames[] = { "発生", "形状", "動き", "見た目" };
		const float settingPageButtonWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 3.0f) /
			static_cast<float>(std::size(settingPageNames));
		ImGui::PushID("EmitterSettingPageButtons");
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
		ImGui::Separator();

		if (selectedSettingPage == 0) {
			ImGui::SeparatorText("発生数と時間");
			int maxParticles = static_cast<int>(emitter.emission.maxParticles);
			if (ImGui::DragInt("最大パーティクル数", &maxParticles, 1.0f, 1, 100000)) {
				emitter.emission.maxParticles = static_cast<uint32_t>((std::max)(1, maxParticles));
			}
			ImGui::DragFloat("1秒あたりの発生数", &emitter.emission.ratePerSecond, 0.1f, 0.0f, FLT_MAX);
			ImGui::DragFloat("発生時間", &emitter.emission.duration, 0.01f, 0.0f, FLT_MAX);
			ImGui::DragFloat("開始遅延", &emitter.emission.startDelay, 0.01f, 0.0f, FLT_MAX);
			ImGui::Checkbox("ループ", &emitter.emission.isLoop);

			ImGui::SeparatorText("バースト");
			int removeBurstIndex = -1;
			const ImGuiTableFlags burstTableFlags =
				ImGuiTableFlags_BordersInnerV |
				ImGuiTableFlags_BordersInnerH |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingStretchProp;
			if (ImGui::BeginTable("BurstTable", 3, burstTableFlags)) {
				ImGui::TableSetupColumn("発生時刻", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("発生数", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 64.0f);
				ImGui::TableHeadersRow();
				for (int index = 0; index < static_cast<int>(emitter.emission.bursts.size()); ++index) {
					BurstConfig& burst = emitter.emission.bursts[index];
					ImGui::PushID(index);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::SetNextItemWidth(-FLT_MIN);
					ImGui::DragFloat("##BurstTime", &burst.time, 0.01f, 0.0f, FLT_MAX);
					ImGui::TableSetColumnIndex(1);
					int burstCount = static_cast<int>(burst.count);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::DragInt("##BurstCount", &burstCount, 1.0f, 1, 100000)) {
						burst.count = static_cast<uint32_t>((std::max)(1, burstCount));
					}
					ImGui::TableSetColumnIndex(2);
					if (ImGui::SmallButton("削除")) {
						removeBurstIndex = index;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			if (removeBurstIndex >= 0) {
				emitter.emission.bursts.erase(emitter.emission.bursts.begin() + removeBurstIndex);
			}
			if (ImGui::Button("バーストを追加")) {
				emitter.emission.bursts.push_back({});
			}
		}

		if (selectedSettingPage == 1) {
			ImGui::SeparatorText("発生形状");
			DrawShapeEditor(emitter.shape);
			const char* spaces[] = { "ローカル", "ワールド" };
			int spaceIndex = emitter.simulationSpace == SimulationSpace::Local ? 0 : 1;
			if (ImGui::Combo("シミュレーション空間", &spaceIndex, spaces, static_cast<int>(std::size(spaces)))) {
				emitter.simulationSpace = spaceIndex == 0 ? SimulationSpace::Local : SimulationSpace::World;
			}
		}

		if (selectedSettingPage == 2) {
			ImGui::SeparatorText("初期値");
			DrawFloatRange("寿命", emitter.initial.lifeTime, 0.01f, 0.001f, FLT_MAX);
			DrawFloatRange("速度", emitter.initial.speed, 0.01f);
			DrawVector3Range("進行方向", emitter.initial.direction, 0.01f);
			DrawFloatRange("回転", emitter.initial.rotation, 0.01f);
			DrawFloatRange("回転速度", emitter.initial.angularVelocity, 0.01f);
			const char* directionModes[] = { "設定した方向", "形状の外向き" };
			int directionMode = emitter.initial.directionMode == DirectionMode::Configured ? 0 : 1;
			if (ImGui::Combo("進行方向の決定方法", &directionMode, directionModes, static_cast<int>(std::size(directionModes)))) {
				emitter.initial.directionMode = directionMode == 0 ? DirectionMode::Configured : DirectionMode::ShapeOutward;
			}

			ImGui::SeparatorText("移動");
			ImGui::DragFloat3("重力", &emitter.motion.gravity.x, 0.01f);
			ImGui::DragFloat3("加速度", &emitter.motion.acceleration.x, 0.01f);
			ImGui::DragFloat("抵抗", &emitter.motion.drag, 0.01f, 0.0f, FLT_MAX);
		}

		if (selectedSettingPage == 3) {
			ImGui::SeparatorText("生存期間による変化");
			DrawVector2Range("開始サイズ", emitter.sizeOverLifetime.start, 0.01f);
			DrawVector2Range("終了サイズ", emitter.sizeOverLifetime.end, 0.01f);
			DrawColorRange("開始色", emitter.colorOverLifetime.start);
			DrawColorRange("終了色", emitter.colorOverLifetime.end);

			ImGui::SeparatorText("描画");
			const MadoEngine::Editor::TextureSelector textureSelector;
			textureSelector.Draw("テクスチャ", emitter.renderer.textureName);

			const char* blendModes[] = { "通常", "加算", "減算", "乗算", "ブレンドなし" };
			int blendMode = static_cast<int>(emitter.renderer.blendMode);
			if (ImGui::Combo("ブレンドモード", &blendMode, blendModes, static_cast<int>(std::size(blendModes)))) {
				emitter.renderer.blendMode = static_cast<MadoEngine::Render::BlendMode>(blendMode);
			}

			const char* sortModes[] = { "なし", "奥から手前" };
			int sortMode = emitter.renderer.sortMode == SortMode::None ? 0 : 1;
			if (ImGui::Combo("並び替え", &sortMode, sortModes, static_cast<int>(std::size(sortModes)))) {
				emitter.renderer.sortMode = sortMode == 0 ? SortMode::None : SortMode::BackToFront;
			}
		}
	}

	/// @brief Particleアセットの編集状態を比較するためのスナップショットを生成する
	/// @param asset スナップショットを生成するParticleアセット
	/// @return JSON形式のスナップショット
	std::string CreateParticleAssetSnapshot(const ParticleEffectAsset& asset) {
		return asset.ToJson().dump();
	}

	/// @brief Particle Editorのプレビューを再生する
	/// @param particleSystem 再生に使用するParticleSystem
	/// @param assetName 再生するParticleアセット名
	/// @param position プレビューの再生位置
	/// @param isLoop ループ再生する場合はtrue
	/// @return 再生したEffectのHandle
	EffectHandle PlayParticlePreview(
		ParticleSystem3d& particleSystem,
		const std::string& assetName,
		const Vector3& position,
		bool isLoop) {
		PlayDesc desc;
		desc.transform.translate = position;
		desc.sceneType = SceneType::None;
		desc.renderLayer = MadoEngine::Render::RenderLayer::Effect;
		desc.loopOverride = isLoop;
		return particleSystem.Play(assetName, desc);
	}

#endif // USE_IMGUI

} // namespace

namespace MadoEngine::Editor {

	void DrawParticleSystemEditorUI() {
#ifdef USE_IMGUI
		ParticleSystem3d& particleSystem = ParticleSystem3d::GetInstance();
		static int selectedAssetIndex = 0;
		static int selectedEmitterIndex = 0;
		static Vector3 previewPosition{};
		static bool previewLoop = false;
		static bool showEmitterShape = true;
		static Vector4 emitterShapeColor = { 0.0f, 1.0f, 1.0f, 1.0f };
		static EffectHandle previewHandle;
		static std::string previewAssetName;
		static std::string previewAssetSnapshot;
		static bool previewLoopSnapshot = false;
		static std::array<char, 128> newAssetNameBuffer{};
		static std::array<char, 128> renameAssetNameBuffer{};
		static std::string assetRenameOriginalName;
		static std::array<char, 128> newEmitterNameBuffer{};
		static std::array<char, 128> renameEmitterNameBuffer{};
		static std::string emitterRenameAssetName;
		static std::string emitterRenameOriginalName;
		static int renameEmitterIndex = -1;
		static bool isNameBufferInitialized = false;
		if (!isNameBufferInitialized) {
			CopyToBuffer(
				newAssetNameBuffer,
				MakeAvailableParticleAssetName(particleSystem, "新規パーティクル")
			);
			CopyToBuffer(newEmitterNameBuffer, "新規エミッター");
			isNameBufferInitialized = true;
		}

		ImGui::SetNextWindowSize(ImVec2(980.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(
			ImVec2(760.0f, 520.0f),
			ImVec2(FLT_MAX, FLT_MAX)
		);
		if (!ImGui::Begin("パーティクルエディター")) {
			ImGui::End();
			return;
		}

		std::vector<std::string> assetNames = particleSystem.GetAssetNames();
		if (!assetNames.empty()) {
			selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
		}
		std::string selectedAssetName = assetNames.empty() ? std::string{} : assetNames[selectedAssetIndex];

		const ImGuiTreeNodeFlags assetCreationHeaderFlags = assetNames.empty()
			? ImGuiTreeNodeFlags_DefaultOpen
			: ImGuiTreeNodeFlags_None;
		if (ImGui::CollapsingHeader("アセットの作成・複製", assetCreationHeaderFlags)) {
			ImGui::Indent();
			ImGui::TextUnformatted("新規アセット名");
			ImGui::SetNextItemWidth((std::max)(180.0f, ImGui::GetContentRegionAvail().x - 210.0f));
		ImGui::InputText(
			"##NewParticleAssetName",
			newAssetNameBuffer.data(),
			newAssetNameBuffer.size()
		);
		const std::string newAssetName = newAssetNameBuffer.data();
		const bool isNewAssetNameEmpty = newAssetName.empty();
		const bool isNewAssetNameAvailable = particleSystem.IsAssetNameAvailable(newAssetName);
		ImGui::SameLine();
		ImGui::BeginDisabled(isNewAssetNameEmpty || !isNewAssetNameAvailable);
		if (ImGui::Button("新規作成")) {
			if (particleSystem.CreateAsset(newAssetName)) {
				assetNames = particleSystem.GetAssetNames();
				const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
				selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
				selectedAssetName = newAssetName;
				assetRenameOriginalName.clear();
				renameEmitterIndex = -1;
				CopyToBuffer(
					newAssetNameBuffer,
					MakeAvailableParticleAssetName(particleSystem, "新規パーティクル")
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
			if (particleSystem.DuplicateAsset(selectedAssetName, newAssetName)) {
				assetNames = particleSystem.GetAssetNames();
				const auto selected = std::find(assetNames.begin(), assetNames.end(), newAssetName);
				selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
				selectedAssetName = newAssetName;
				assetRenameOriginalName.clear();
				renameEmitterIndex = -1;
				CopyToBuffer(
					newAssetNameBuffer,
					MakeAvailableParticleAssetName(particleSystem, "新規パーティクル")
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
			ImGui::TextDisabled("編集するパーティクルアセットを作成してください。");
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("編集中のアセット");
		selectedAssetIndex = std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
		ImGui::SetNextItemWidth((std::max)(240.0f, ImGui::GetContentRegionAvail().x * 0.5f));
		selectedAssetName = assetNames[selectedAssetIndex];
		if (ImGui::BeginCombo("アセット", assetNames[selectedAssetIndex].c_str())) {
			for (int index = 0; index < static_cast<int>(assetNames.size()); ++index) {
				const bool isSelected = index == selectedAssetIndex;
				if (ImGui::Selectable(assetNames[index].c_str(), isSelected)) {
					selectedAssetIndex = index;
					selectedEmitterIndex = 0;
					selectedAssetName = assetNames[index];
					assetRenameOriginalName.clear();
					renameEmitterIndex = -1;
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
		ImGui::SetNextItemWidth(240.0f);
		ImGui::InputText(
			"アセット名",
			renameAssetNameBuffer.data(),
			renameAssetNameBuffer.size()
		);
		const std::string renameAssetName = renameAssetNameBuffer.data();
		const bool isAssetRenameChanged = renameAssetName != selectedAssetName;
		const bool isAssetRenameAvailable = particleSystem.IsAssetNameAvailable(renameAssetName);
		ImGui::SameLine();
		ImGui::BeginDisabled(
			renameAssetName.empty() ||
			!isAssetRenameChanged ||
			!isAssetRenameAvailable
		);
		if (ImGui::Button("アセット名を変更")) {
			const std::string oldAssetName = selectedAssetName;
			if (particleSystem.RenameAsset(oldAssetName, renameAssetName)) {
				if (previewAssetName == oldAssetName) {
					previewAssetName = renameAssetName;
				}
				assetNames = particleSystem.GetAssetNames();
				const auto selected = std::find(assetNames.begin(), assetNames.end(), renameAssetName);
				selectedAssetIndex = static_cast<int>(std::distance(assetNames.begin(), selected));
				selectedAssetName = renameAssetName;
				assetRenameOriginalName = renameAssetName;
				renameEmitterIndex = -1;
			}
		}
		ImGui::EndDisabled();
		if (isAssetRenameChanged && !renameAssetName.empty() && !isAssetRenameAvailable) {
			ImGui::TextDisabled("変更後の名前は使用できません。");
		}

		if (ImGui::Button("アセットを削除")) {
			ImGui::OpenPopup("アセット削除確認");
		}
		if (ImGui::BeginPopupModal("アセット削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("「%s」を削除しますか？", selectedAssetName.c_str());
			ImGui::TextDisabled("Jsonファイルは.trashディレクトリへ退避されます。");
			if (ImGui::Button("削除する##Asset")) {
				if (particleSystem.IsAlive(previewHandle)) {
					particleSystem.Stop(previewHandle, StopMode::Immediate);
				}
				previewAssetName.clear();
				previewAssetSnapshot.clear();
				if (particleSystem.DeleteAsset(selectedAssetName)) {
					assetNames = particleSystem.GetAssetNames();
					selectedAssetIndex = assetNames.empty()
						? 0
						: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);
					selectedAssetName = assetNames.empty() ? std::string{} : assetNames[selectedAssetIndex];
					assetRenameOriginalName.clear();
					renameEmitterIndex = -1;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル##Asset")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
			ImGui::Unindent();
		}

		if (assetNames.empty()) {
			ImGui::TextDisabled("パーティクルアセットがありません。新規作成してください。");
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("プレビュー");
		const ImGuiTableFlags previewTableFlags = ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("PreviewSettings", 2, previewTableFlags)) {
			ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableSetupColumn("表示", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("プレビュー位置", &previewPosition.x, 0.05f);
			ImGui::TableSetColumnIndex(1);
			ImGui::Checkbox("プレビューをループ", &previewLoop);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);
			ImGui::Checkbox("発生形状を表示", &showEmitterShape);
			if (showEmitterShape) {
				ImGui::SameLine();
				ImGui::ColorEdit4("##EmitterShapeColor", &emitterShapeColor.x, ImGuiColorEditFlags_NoInputs);
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("発生形状の表示色");
				}
			}
			ImGui::EndTable();
		}
		if (ImGui::Button("プレビューを再生")) {
			if (particleSystem.IsAlive(previewHandle)) {
				particleSystem.Stop(previewHandle, StopMode::Immediate);
			}
			previewHandle = PlayParticlePreview(
				particleSystem,
				selectedAssetName,
				previewPosition,
				previewLoop
			);
			if (particleSystem.IsAlive(previewHandle)) {
				previewAssetName = selectedAssetName;
				if (const ParticleEffectAsset* previewAsset = particleSystem.FindAsset(selectedAssetName)) {
					previewAssetSnapshot = CreateParticleAssetSnapshot(*previewAsset);
				}
				previewLoopSnapshot = previewLoop;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("プレビューを停止")) {
			if (particleSystem.IsAlive(previewHandle)) {
				particleSystem.Stop(previewHandle, StopMode::Immediate);
			}
			previewAssetName.clear();
			previewAssetSnapshot.clear();
		}
		ImGui::SameLine();
		if (ImGui::Button("再読み込み")) {
			particleSystem.ReloadAsset(selectedAssetName);
			renameEmitterIndex = -1;
		}

		ImGui::Text(
			"再生中: %zu エフェクト    生存中: %zu パーティクル",
			particleSystem.GetActiveEffectCount(),
			particleSystem.GetAliveParticleCount()
		);
		ImGui::Separator();

		ParticleEffectAsset* asset = particleSystem.FindEditableAsset(selectedAssetName);
		if (!asset) {
			ImGui::TextDisabled("編集可能なパーティクルアセットがありません。");
			ImGui::End();
			return;
		}

		if (ImGui::Button("アセットを保存")) {
			asset->Validate();
			asset->SaveToFile();
		}

		std::vector<EmitterConfig>& emitters = asset->GetEmitters();
		ImGui::SameLine();
		ImGui::TextDisabled("%zu エミッター", emitters.size());
		const float emitterListWidth = std::clamp(
			ImGui::GetContentRegionAvail().x * 0.28f,
			240.0f,
			300.0f
		);
		ImGui::BeginChild("EmitterListPane", ImVec2(emitterListWidth, 0.0f), true);
		ImGui::SeparatorText("エミッター一覧");
		ImGui::TextUnformatted("新規エミッター名");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText(
			"##NewEmitterName",
			newEmitterNameBuffer.data(),
			newEmitterNameBuffer.size()
		);
		const std::string newEmitterName = newEmitterNameBuffer.data();
		const bool isNewNameEmpty = newEmitterName.empty();
		const bool isNewNameDuplicated = IsEmitterNameUsed(emitters, newEmitterName);
		ImGui::BeginDisabled(isNewNameEmpty || isNewNameDuplicated);
		if (ImGui::Button("追加", ImVec2(-FLT_MIN, 0.0f))) {
			EmitterConfig emitter;
			emitter.name = newEmitterName;
			emitters.push_back(std::move(emitter));
			selectedEmitterIndex = static_cast<int>(emitters.size()) - 1;
			renameEmitterIndex = -1;
			CopyToBuffer(
				newEmitterNameBuffer,
				MakeUniqueEmitterName(emitters, "新規エミッター")
			);
		}
		ImGui::EndDisabled();

		if (isNewNameEmpty) {
			ImGui::TextDisabled("新規エミッター名を入力してください。");
		} else if (isNewNameDuplicated) {
			ImGui::TextDisabled("同じ名前のエミッターが存在します。");
		}

		if (emitters.empty()) {
			ImGui::TextDisabled("編集するエミッターを追加してください。");
			ImGui::EndChild();
			ImGui::End();
			return;
		}

		selectedEmitterIndex = std::clamp(selectedEmitterIndex, 0, static_cast<int>(emitters.size()) - 1);
		ImGui::TextUnformatted("登録済みエミッター");
		const float emitterListHeight = std::clamp(
			static_cast<float>(emitters.size()) * ImGui::GetTextLineHeightWithSpacing() + 8.0f,
			96.0f,
			220.0f
		);
		ImGui::BeginChild("EmitterSelectionList", ImVec2(0.0f, emitterListHeight), true);
		for (int index = 0; index < static_cast<int>(emitters.size()); ++index) {
			const bool isSelected = index == selectedEmitterIndex;
			if (ImGui::Selectable(emitters[index].name.c_str(), isSelected)) {
				selectedEmitterIndex = index;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndChild();

		const float emitterOperationButtonWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
		if (ImGui::Button("複製", ImVec2(emitterOperationButtonWidth, 0.0f))) {
			EmitterConfig emitter = emitters[selectedEmitterIndex];
			emitter.name = MakeUniqueEmitterName(emitters, emitter.name + " のコピー");
			emitters.insert(emitters.begin() + selectedEmitterIndex + 1, std::move(emitter));
			++selectedEmitterIndex;
			renameEmitterIndex = -1;
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(emitters.size() <= 1);
		if (ImGui::Button("削除", ImVec2(emitterOperationButtonWidth, 0.0f))) {
			ImGui::OpenPopup("エミッター削除確認");
		}
		ImGui::EndDisabled();

		if (ImGui::BeginPopupModal("エミッター削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("「%s」を削除しますか？", emitters[selectedEmitterIndex].name.c_str());
			ImGui::TextDisabled("保存するまでは再読み込みで元に戻せます。");
			if (ImGui::Button("削除する")) {
				emitters.erase(emitters.begin() + selectedEmitterIndex);
				selectedEmitterIndex = std::clamp(
					selectedEmitterIndex,
					0,
					static_cast<int>(emitters.size()) - 1
				);
				renameEmitterIndex = -1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		EmitterConfig& selectedEmitter = emitters[selectedEmitterIndex];
		if (
			emitterRenameAssetName != selectedAssetName ||
			renameEmitterIndex != selectedEmitterIndex ||
			emitterRenameOriginalName != selectedEmitter.name) {
			CopyToBuffer(renameEmitterNameBuffer, selectedEmitter.name);
			emitterRenameAssetName = selectedAssetName;
			renameEmitterIndex = selectedEmitterIndex;
			emitterRenameOriginalName = selectedEmitter.name;
		}

		ImGui::SeparatorText("名前変更");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText(
			"##RenameEmitterName",
			renameEmitterNameBuffer.data(),
			renameEmitterNameBuffer.size()
		);
		const std::string renameEmitterName = renameEmitterNameBuffer.data();
		const bool isRenameNameEmpty = renameEmitterName.empty();
		const bool isRenameNameDuplicated = IsEmitterNameUsed(
			emitters,
			renameEmitterName,
			selectedEmitterIndex
		);
		const bool isRenameNameChanged = renameEmitterName != selectedEmitter.name;
		ImGui::BeginDisabled(
			isRenameNameEmpty ||
			isRenameNameDuplicated ||
			!isRenameNameChanged
		);
		if (ImGui::Button("名前を変更", ImVec2(-FLT_MIN, 0.0f))) {
			selectedEmitter.name = renameEmitterName;
			emitterRenameOriginalName = selectedEmitter.name;
		}
		ImGui::EndDisabled();

		if (isRenameNameEmpty) {
			ImGui::TextDisabled("エミッター名を入力してください。");
		} else if (isRenameNameDuplicated) {
			ImGui::TextDisabled("同じ名前のエミッターが存在します。");
		}

		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("EmitterSettingPane", ImVec2(0.0f, 0.0f), true);
		ImGui::Text("設定: %s", emitters[selectedEmitterIndex].name.c_str());
		ImGui::Separator();
		DrawEmitterEditor(emitters[selectedEmitterIndex]);
		ImGui::EndChild();

		const std::string currentAssetSnapshot = CreateParticleAssetSnapshot(*asset);
		if (
			particleSystem.IsAlive(previewHandle) &&
			previewAssetName == selectedAssetName) {
			const bool isAssetChanged = previewAssetSnapshot != currentAssetSnapshot;
			const bool isLoopChanged = previewLoopSnapshot != previewLoop;
			if (isAssetChanged || isLoopChanged) {
				particleSystem.Stop(previewHandle, StopMode::Immediate);
				previewHandle = PlayParticlePreview(
					particleSystem,
					selectedAssetName,
					previewPosition,
					previewLoop
				);
				previewAssetSnapshot = currentAssetSnapshot;
				previewLoopSnapshot = previewLoop;
			} else {
				Transform3D previewTransform;
				previewTransform.translate = previewPosition;
				particleSystem.SetTransform(previewHandle, previewTransform);
			}
		}

		if (showEmitterShape) {
			Transform3D emitterTransform;
			emitterTransform.translate = previewPosition;
			ParticleEmitterDebugDrawer3d::Submit(
				emitters[selectedEmitterIndex],
				emitterTransform,
				emitterShapeColor
			);
		}
		ImGui::End();
#endif // USE_IMGUI
	}

} // namespace MadoEngine::Editor
