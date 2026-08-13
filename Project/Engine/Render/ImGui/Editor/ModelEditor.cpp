#include "ModelEditor.h"
#include "TextureSelector.h"
#include "Render/Object/3d/Line/MyDebugLine.h"
#include "Render/Object/3d/Model/ModelManager.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <vector>

namespace MadoEngine::Editor {

	namespace {

		const std::filesystem::path kModelEditorJsonPath = "Assets/Json/ModelObjects.json";
		const std::filesystem::path kModelDirectoryPath = "Assets/Model";

#ifdef USE_IMGUI

		constexpr float kRadiansToDegrees = 57.29577951308232f;
		constexpr float kDegreesToRadians = 0.017453292519943295f;
		constexpr float kDefaultVertexMarkerRadius = 0.1f;
		constexpr Vector4 kVertexMarkerColor = { 1.0f, 0.0f, 0.0f, 1.0f };

		/// @brief Modelアセット選択TreeのFile情報
		struct ModelAssetItem {
			std::string assetPath;
			std::string fileName;
		};

		/// @brief Modelアセット選択TreeのDirectory情報
		struct ModelAssetDirectory {
			std::string name;
			std::vector<ModelAssetDirectory> directories;
			std::vector<ModelAssetItem> assets;
		};

		/// @brief Modelアセットパスから表示用のModel名を取得
		/// @param modelName Modelアセットパス
		/// @return Editor表示用のModelアセット名
		std::string GetModelDisplayName(const std::string& modelName) {
			const std::string displayName = std::filesystem::path(modelName).filename().string();
			if (!displayName.empty()) {
				return displayName;
			}

			return modelName;
		}

		/// @brief 文字列を固定長バッファへコピー
		/// @tparam Size バッファサイズ
		/// @param buffer コピー先バッファ
		/// @param text コピー元文字列
		template<size_t Size>
		void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
			buffer.fill('\0');
			strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
		}

		/// @brief 追加したModel名を基準に次の未使用名を生成
		/// @param manager Model名の使用状況を確認するManager
		/// @param createdName 直前に追加したModel名
		/// @return 末尾の番号を繰り上げた未使用のModel名
		std::string MakeNextAvailableModelName(
			const ModelManager& manager,
			const std::string& createdName) {
			size_t suffixStart = createdName.size();
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

			const std::vector<std::string> names = manager.GetNames();
			for (;;) {
				const std::string candidate = baseName + std::to_string(suffix);
				if (std::find(names.begin(), names.end(), candidate) == names.end()) {
					return candidate;
				}
				++suffix;
			}
		}

		/// @brief Modelアセット階層内の子Directoryを取得または追加
		/// @param parent 親Directory
		/// @param directoryName 子Directory名
		/// @return 取得または追加した子Directory
		ModelAssetDirectory& FindOrAddModelAssetDirectory(
			ModelAssetDirectory& parent,
			const std::string& directoryName) {
			const auto found = std::find_if(
				parent.directories.begin(),
				parent.directories.end(),
				[&directoryName](const ModelAssetDirectory& directory) {
					return directory.name == directoryName;
				}
			);
			if (found != parent.directories.end()) {
				return *found;
			}

			parent.directories.push_back(ModelAssetDirectory{ directoryName });
			return parent.directories.back();
		}

		/// @brief Modelアセット階層をDirectory名とFile名で再帰的に整列
		/// @param directory 整列対象Directory
		void SortModelAssetDirectory(ModelAssetDirectory& directory) {
			std::sort(
				directory.directories.begin(),
				directory.directories.end(),
				[](const ModelAssetDirectory& left, const ModelAssetDirectory& right) {
					return left.name < right.name;
				}
			);
			std::sort(
				directory.assets.begin(),
				directory.assets.end(),
				[](const ModelAssetItem& left, const ModelAssetItem& right) {
					return left.fileName < right.fileName;
				}
			);
			for (ModelAssetDirectory& child : directory.directories) {
				SortModelAssetDirectory(child);
			}
		}

		/// @brief ModelManagerのAsset Path一覧から選択Treeを構築
		/// @param modelNames Modelアセットパス一覧
		/// @return Assets/Modelからの相対階層へ変換した選択Tree
		ModelAssetDirectory CreateModelAssetTree(
			const std::vector<std::string>& modelNames) {
			ModelAssetDirectory root;
			root.name = kModelDirectoryPath.generic_string();
			for (const std::string& modelName : modelNames) {
				const std::filesystem::path modelPath(modelName);
				const std::filesystem::path relativePath =
					modelPath.lexically_relative(kModelDirectoryPath);
				const std::filesystem::path displayPath = relativePath.empty()
					? modelPath.filename()
					: relativePath;

				ModelAssetDirectory* directory = &root;
				for (const std::filesystem::path& component : displayPath.parent_path()) {
					directory = &FindOrAddModelAssetDirectory(*directory, component.string());
				}
				directory->assets.push_back(ModelAssetItem{
					modelName,
					displayPath.filename().string(),
				});
			}
			SortModelAssetDirectory(root);
			return root;
		}

		/// @brief Directory配下に選択中Modelアセットが存在するか確認
		/// @param directory 確認対象Directory
		/// @param selectedName 選択中Modelアセットパス
		/// @return 選択中Modelアセットが存在する場合はtrue
		bool ContainsSelectedModelAsset(
			const ModelAssetDirectory& directory,
			const std::string& selectedName) {
			if (std::any_of(
				directory.assets.begin(),
				directory.assets.end(),
				[&selectedName](const ModelAssetItem& asset) {
					return asset.assetPath == selectedName;
				})) {
				return true;
			}
			return std::any_of(
				directory.directories.begin(),
				directory.directories.end(),
				[&selectedName](const ModelAssetDirectory& child) {
					return ContainsSelectedModelAsset(child, selectedName);
				}
			);
		}

		/// @brief ModelアセットDirectory配下の選択項目を再帰的に描画
		/// @param directory 描画対象Directory
		/// @param selectedName 現在選択中のModelアセットパス
		/// @return 選択が変更された場合はtrue
		bool DrawModelAssetDirectory(
			const ModelAssetDirectory& directory,
			std::string& selectedName) {
			bool isChanged = false;
			for (const ModelAssetDirectory& child : directory.directories) {
				ImGui::PushID(child.name.c_str());
				if (ContainsSelectedModelAsset(child, selectedName)) {
					ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
				}
				if (ImGui::TreeNodeEx(
					"##ModelAssetDirectory",
					ImGuiTreeNodeFlags_SpanAvailWidth,
					"%s",
					child.name.c_str())) {
					isChanged |= DrawModelAssetDirectory(child, selectedName);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			for (const ModelAssetItem& asset : directory.assets) {
				ImGui::PushID(asset.assetPath.c_str());
				const bool isSelected = asset.assetPath == selectedName;
				if (ImGui::Selectable(asset.fileName.c_str(), isSelected)) {
					selectedName = asset.assetPath;
					isChanged = true;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}
			return isChanged;
		}

		/// @brief Modelアセット選択Comboを描画
		/// @param label ImGuiで使用するラベル
		/// @param selectedName 現在選択中のModelアセットパス
		/// @param modelNames 選択候補のModelアセットパス一覧
		/// @return 選択が変更された場合はtrue
		bool DrawModelAssetCombo(
			const char* label,
			std::string& selectedName,
			const std::vector<std::string>& modelNames) {
			const std::string displayName = GetModelDisplayName(selectedName);
			const char* preview = selectedName.empty() ? "Modelを選択" : displayName.c_str();
			bool isChanged = false;
			if (ImGui::BeginCombo(label, preview)) {
				const ModelAssetDirectory tree = CreateModelAssetTree(modelNames);
				ImGui::TextDisabled("%s", tree.name.c_str());
				ImGui::Separator();
				isChanged |= DrawModelAssetDirectory(tree, selectedName);
				ImGui::EndCombo();
			}
			return isChanged;
		}

		/// @brief Modelの描画レイヤー選択Comboを描画
		/// @param model 編集対象のModel
		void DrawRenderLayerCombo(Model& model) {
			const Render::RenderLayer currentLayer = model.GetRenderLayer();
			if (ImGui::BeginCombo("描画レイヤー", Render::GetRenderLayerName(currentLayer))) {
				for (uint32_t index = 0; index < Render::kRenderLayerCount; ++index) {
					const Render::RenderLayer layer = Render::GetRenderLayerByIndex(index);
					const bool isSelected = layer == currentLayer;
					if (ImGui::Selectable(Render::GetRenderLayerName(layer), isSelected)) {
						model.SetRenderLayer(layer);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		/// @brief Modelの対象Scene選択Comboを描画
		/// @param model 編集対象のModel
		void DrawSceneCombo(Model& model) {
			const SceneType currentScene = model.GetSceneType();
			if (ImGui::BeginCombo("対象シーン", SceneTypeToString(currentScene).c_str())) {
				const bool isNoneSelected = currentScene == SceneType::None;
				if (ImGui::Selectable("None", isNoneSelected)) {
					model.SetSceneType(SceneType::None);
				}
				if (isNoneSelected) {
					ImGui::SetItemDefaultFocus();
				}

				for (uint32_t index = 0; index < kSceneTypeCount; ++index) {
					const SceneType sceneType = GetSceneTypeByIndex(index);
					const bool isSelected = sceneType == currentScene;
					const std::string sceneName = SceneTypeToString(sceneType);
					if (ImGui::Selectable(sceneName.c_str(), isSelected)) {
						model.SetSceneType(sceneType);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		/// @brief Modelが受け取るLightレイヤーマスクを編集
		/// @param model 編集対象のModel
		void DrawReceiveLightMask(Model& model) {
			LightLayerMask lightMask = model.GetReceiveLightMask();
			if (ImGui::TreeNode("受光レイヤー")) {
				for (uint32_t index = 0; index < kLightLayerCount; ++index) {
					const LightLayer layer = GetLightLayerByIndex(index);
					const LightLayerMask layerMask = ToLightLayerMask(layer);
					bool isEnabled = (lightMask & layerMask) != 0;
					if (ImGui::Checkbox(GetLightLayerName(layer), &isEnabled)) {
						if (isEnabled) {
							lightMask |= layerMask;
						} else {
							lightMask &= ~layerMask;
						}
						model.SetReceiveLightMask(lightMask);
					}
				}
				ImGui::TreePop();
			}
		}

		/// @brief 指定した頂点Indexの検索UIとデバッグ表示を描画
		/// @param model 検索対象のModel
		void DrawVertexSearch(Model& model) {
			static int vertexIndex = 0;
			static float markerRadius = kDefaultVertexMarkerRadius;
			static bool isMarkerVisible = false;

			ImGui::SeparatorText("頂点検索");

			const size_t vertexCount = model.GetVertexCount();
			ImGui::Text("頂点数: %zu", vertexCount);
			const int maxVertexIndex = vertexCount > 0
				? static_cast<int>(vertexCount - 1)
				: 0;
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("頂点Index");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(160.0f);
			ImGui::DragInt("##頂点Index", &vertexIndex, 1.0f, 0, maxVertexIndex);
			vertexIndex = std::clamp(vertexIndex, 0, maxVertexIndex);

			ImGui::SameLine();
			const bool canDecrementVertexIndex = vertexCount > 0 && vertexIndex > 0;
			if (!canDecrementVertexIndex) {
				ImGui::BeginDisabled();
			}
			if (ImGui::SmallButton("-##頂点Index減算")) {
				--vertexIndex;
			}
			if (!canDecrementVertexIndex) {
				ImGui::EndDisabled();
			}

			ImGui::SameLine();
			const bool canIncrementVertexIndex =
				vertexCount > 0 && vertexIndex < maxVertexIndex;
			if (!canIncrementVertexIndex) {
				ImGui::BeginDisabled();
			}
			if (ImGui::SmallButton("+##頂点Index加算")) {
				++vertexIndex;
			}
			if (!canIncrementVertexIndex) {
				ImGui::EndDisabled();
			}

			ImGui::SetNextItemWidth(160.0f);
			ImGui::DragFloat("表示半径", &markerRadius, 0.01f, 0.001f, 100.0f, "%.3f");
			markerRadius = std::clamp(markerRadius, 0.001f, 100.0f);
			ImGui::Checkbox("検索頂点を表示", &isMarkerVisible);

			if (vertexCount == 0) {
				ImGui::TextDisabled("頂点データがありません");
				return;
			}

			if (vertexIndex < 0 || static_cast<size_t>(vertexIndex) >= vertexCount) {
				ImGui::TextColored(
					ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
					"Indexは0～%zuの範囲で入力してください",
					vertexCount - 1
				);
				return;
			}

			Vector3 worldPosition;
			if (!model.TryGetVertexWorldPosition(
				static_cast<uint32_t>(vertexIndex),
				worldPosition)) {
				ImGui::TextDisabled("頂点位置を取得できません");
				return;
			}

			ImGui::Text(
				"ワールド位置: (%.3f, %.3f, %.3f)",
				worldPosition.x,
				worldPosition.y,
				worldPosition.z
			);

			if (isMarkerVisible) {
				Sphere marker;
				marker.center = worldPosition;
				marker.radius = markerRadius;
				MyDebugLine::AddShape(marker, kVertexMarkerColor);
			}
		}

		/// @brief Modelのプロパティ編集UIを描画
		/// @param model 編集対象のModel
		/// @param assetName Modelが使用しているアセットパス
		void DrawModelProperties(Model& model, const std::string& assetName) {
			const std::string displayName = GetModelDisplayName(assetName);
			ImGui::TextWrapped("アセット: %s", displayName.c_str());
			if (const ModelSharedData* sharedData = model.GetSharedData()) {
				ImGui::Text("種類: %s", ModelResource::ModelTypeToString(sharedData->type).c_str());
			}
			ImGui::Separator();

			ImGui::SeparatorText("テクスチャ");
			std::string selectedTextureName = model.GetTextureName();
			const TextureSelector textureSelector(128.0f);
			if (textureSelector.Draw("使用テクスチャ", selectedTextureName)) {
				model.SetTexture(selectedTextureName);
			}
			if (model.HasTextureOverride() && ImGui::Button("モデル既定に戻す")) {
				model.ResetTexture();
			}

			ImGui::SeparatorText("トランスフォーム・描画");

			Vector3 position = model.GetPosition();
			float positionValues[3] = { position.x, position.y, position.z };
			if (ImGui::DragFloat3("位置", positionValues, 0.05f)) {
				model.SetPosition({ positionValues[0], positionValues[1], positionValues[2] });
			}

			Vector3 rotation = model.GetRotation();
			float rotationValues[3] = {
				rotation.x * kRadiansToDegrees,
				rotation.y * kRadiansToDegrees,
				rotation.z * kRadiansToDegrees,
			};
			if (ImGui::DragFloat3("回転", rotationValues, 0.5f, -360.0f, 360.0f, "%.1f度")) {
				model.SetRotation({
					rotationValues[0] * kDegreesToRadians,
					rotationValues[1] * kDegreesToRadians,
					rotationValues[2] * kDegreesToRadians,
					});
			}

			Vector3 scale = model.GetScale();
			float scaleValues[3] = { scale.x, scale.y, scale.z };
			if (ImGui::DragFloat3("スケール", scaleValues, 0.01f, 0.001f, 1000.0f, "%.3f")) {
				model.SetScale({ scaleValues[0], scaleValues[1], scaleValues[2] });
			}

			Vector4 color = model.GetColor();
			float colorValues[4] = { color.x, color.y, color.z, color.w };
			if (ImGui::ColorEdit4("色", colorValues)) {
				model.SetColor({ colorValues[0], colorValues[1], colorValues[2], colorValues[3] });
			}

			bool isLightingEnabled = model.IsLightingEnabled();
			if (ImGui::Checkbox("ライティング", &isLightingEnabled)) {
				model.SetLightingEnabled(isLightingEnabled);
			}

			bool useBillboard = model.IsUseBillboard();
			if (ImGui::Checkbox("ビルボード", &useBillboard)) {
				model.SetUseBillboard(useBillboard);
			}

			Vector3 cameraTranslateOffset = model.GetCameraTranslateOffset();
			if (ImGui::DragFloat3("カメラ基準オフセット", &cameraTranslateOffset.x, 0.01f)) {
				model.SetCameraTranslateOffset(cameraTranslateOffset);
			}

			bool castShadow = model.CanCastShadow();
			if (ImGui::Checkbox("影を落とす", &castShadow)) {
				model.SetCastShadow(castShadow);
			}

			bool receiveShadow = model.CanReceiveShadow();
			if (ImGui::Checkbox("影を受ける", &receiveShadow)) {
				model.SetReceiveShadow(receiveShadow);
			}

			bool frustumCulling = model.IsFrustumCullingEnabled();
			if (ImGui::Checkbox("視錐台カリング", &frustumCulling)) {
				model.SetFrustumCullingEnabled(frustumCulling);
			}

			DrawRenderLayerCombo(model);
			DrawSceneCombo(model);
			DrawReceiveLightMask(model);
			DrawVertexSearch(model);
		}

#endif // USE_IMGUI

	} // namespace

	bool LoadModelEditorJson() {
		return ModelManager::GetInstance().LoadFromFile(kModelEditorJsonPath);
	}

	bool LoadModelEditorJson(SceneType sceneType) {
		return ModelManager::GetInstance().LoadFromFile(kModelEditorJsonPath, sceneType);
	}

#ifdef USE_IMGUI

	void DrawModelManagerEditorUI(SceneType currentSceneType) {
		ModelManager& manager = ModelManager::GetInstance();
		const std::vector<std::string> modelNames = manager.GetAvailableModelNames();

		// 選択Handleと作成候補をFrame間で維持するEditor Session状態
		static std::array<char, 128> createName{};
		static std::string createModelName;
		static ModelHandle selectedHandle{};
		static bool isInitialized = false;
		if (!isInitialized) {
			CopyToBuffer(createName, "Model");
			if (!modelNames.empty()) {
				createModelName = modelNames.front();
			}
			isInitialized = true;
		}

		ImGui::Begin("Model Editor");

		ImGui::SetNextItemWidth(180.0f);
		ImGui::InputText("新規名", createName.data(), createName.size());
		ImGui::SameLine();
		if (modelNames.empty()) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("追加")) {
			const std::string requestedName = createName.data();
			const ModelHandle created = manager.Create(
				requestedName,
				createModelName,
				SceneType::None,
				EditorManagementMode::EditorManaged);
			if (created.IsValid()) {
				selectedHandle = created;
				CopyToBuffer(createName, MakeNextAvailableModelName(manager, requestedName));
			}
		}
		if (modelNames.empty()) {
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(180.0f);
		DrawModelAssetCombo("Modelアセット", createModelName, modelNames);
		if (modelNames.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("利用可能なModelアセットがありません");
		}

		if (ImGui::Button("保存")) {
			manager.SaveToFile(kModelEditorJsonPath, currentSceneType);
		}
		ImGui::SameLine();
		if (ImGui::Button("読込")) {
			LoadModelEditorJson(currentSceneType);
		}
		ImGui::SameLine();
		if (ImGui::Button("復元")) {
			std::filesystem::path backupPath = kModelEditorJsonPath;
			backupPath += ".bak";
			manager.LoadFromFile(backupPath, currentSceneType);
		}
		ImGui::SameLine();
		ImGui::Text("インスタンス数: %zu", manager.GetModelCount());

		ImGui::Separator();

		const std::vector<std::string> names = manager.GetEditorManagedNames();
		ImGui::BeginChild("ModelList", ImVec2(220.0f, 0.0f), true);
		for (const std::string& name : names) {
			ImGui::PushID(name.c_str());
			const ModelHandle handle = manager.Find(name);
			Model* model = manager.TryGet(handle);
			if (!model) {
				ImGui::PopID();
				continue;
			}

			bool isVisible = model->IsVisible();
			if (ImGui::Checkbox("##Visible", &isVisible)) {
				model->SetVisible(isVisible);
			}
			ImGui::SameLine();

			const bool isSelected = handle == selectedHandle;
			const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			const float selectableWidth = (std::max)(
				1.0f,
				ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Selectable(name.c_str(), isSelected, 0, ImVec2(selectableWidth, 0.0f))) {
				selectedHandle = handle;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("削除")) {
				manager.RequestDestroy(handle);
				if (selectedHandle == handle) {
					selectedHandle = {};
				}
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("ModelProperties", ImVec2(0.0f, 0.0f), true);
		Model* selectedModel = manager.TryGet(selectedHandle);
		if (selectedModel) {
			std::string selectedName;
			for (const std::string& name : names) {
				if (manager.Find(name) == selectedHandle) {
					selectedName = name;
					break;
				}
			}
			std::array<char, 128> nameBuffer{};
			CopyToBuffer(nameBuffer, selectedName);
			if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
				const std::string newName = nameBuffer.data();
				if (!newName.empty()) {
					manager.Rename(selectedHandle, newName);
				}
			}

			DrawModelProperties(*selectedModel, manager.GetModelAssetName(selectedHandle));
		} else {
			ImGui::TextDisabled("Modelを選択してください。");
		}
		ImGui::EndChild();

		ImGui::End();
	}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
