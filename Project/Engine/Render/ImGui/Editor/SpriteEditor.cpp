#include "SpriteEditor.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Core/TextureManager/TextureManager.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace MadoEngine::Editor {

namespace {

	const std::filesystem::path kSpriteEditorJsonPath = "Assets/Json/SpriteObjects.json";

#ifdef USE_IMGUI

	constexpr float kRadiansToDegrees = 57.29577951308232f;
	constexpr float kDegreesToRadians = 0.017453292519943295f;
	constexpr float kTexturePreviewMaxSize = 64.0f;
	const std::filesystem::path kTextureDirectoryPath = "Assets/Texture";

	struct TexturePreviewData {
		ImTextureID textureId = ImTextureID_Invalid;
		ImVec2 displaySize{};
		Vector2 pixelSize{};
	};

	struct SpriteTextureItem {
		std::string textureName;
		std::string fileName;
		std::string relativePath;
	};

	struct SpriteTextureDirectory {
		std::string name;
		std::vector<SpriteTextureDirectory> directories;
		std::vector<SpriteTextureItem> textures;
	};

	struct SpriteTextureTree {
		SpriteTextureDirectory root;
		std::vector<std::string> otherTextureNames;
	};

	/// @brief テクスチャプレビューの描画情報を作成
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @return 描画情報、テクスチャが見つからない場合はstd::nullopt
	std::optional<TexturePreviewData> CreateTexturePreviewData(const std::string& textureName) {
		TextureManager& textureManager = TextureManager::GetInstance();
		const uint32_t textureIndex = textureManager.GetTextureIndex(textureName);
		if (textureIndex == UINT32_MAX) {
			return std::nullopt;
		}

		const Vector2 pixelSize = textureManager.GetPixelSize(textureName);
		const float width = (std::max)(1.0f, pixelSize.x);
		const float height = (std::max)(1.0f, pixelSize.y);
		const float scale = (std::min)(1.0f, kTexturePreviewMaxSize / (std::max)(width, height));
		const D3D12_GPU_DESCRIPTOR_HANDLE handle = textureManager.GetSrvHandleGPU(textureIndex);

		return TexturePreviewData{
			static_cast<ImTextureID>(handle.ptr),
			{ width * scale, height * scale },
			pixelSize,
		};
	}

	/// @brief 直前のImGui項目にカーソルが重なっている場合にテクスチャプレビューを表示
	/// @param textureName プレビュー対象のテクスチャ名
	void DrawHoveredTexturePreview(const std::string& textureName) {
		if (textureName.empty() || !ImGui::IsItemHovered()) {
			return;
		}

		const std::optional<TexturePreviewData> previewData = CreateTexturePreviewData(textureName);
		if (!previewData) {
			return;
		}

		if (ImGui::BeginTooltip()) {
			ImGui::Text(
				"%s (%d x %d)",
				textureName.c_str(),
				static_cast<int>(previewData->pixelSize.x),
				static_cast<int>(previewData->pixelSize.y));
			ImGui::Image(previewData->textureId, previewData->displaySize);
			ImGui::EndTooltip();
		}
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

	/// @brief 追加したSprite名を基準に次の未使用名を生成
	/// @param manager Sprite名の使用状況を確認するManager
	/// @param createdName 直前に追加したSprite名
	/// @return 末尾の番号を繰り上げた未使用のSprite名
	std::string MakeNextAvailableSpriteName(
		const SpriteManager& manager,
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

	/// @brief Sprite Editorで選択可能な静的テクスチャ名を取得
	/// @return 名前順に並んだテクスチャ名一覧
	std::vector<std::string> GetSelectableTextureNames() {
		std::vector<std::string> names = TextureManager::GetInstance().GetTextureNames();
		names.erase(
			std::remove_if(names.begin(), names.end(), [](const std::string& name) {
				return name.starts_with("__");
			}),
			names.end());
		return names;
	}

	/// @brief Texture階層内の子Directoryを取得または追加
	/// @param parent 親Directory
	/// @param directoryName 子Directory名
	/// @return 取得または追加した子Directory
	SpriteTextureDirectory& FindOrAddTextureDirectory(
		SpriteTextureDirectory& parent,
		const std::string& directoryName) {
		const auto found = std::find_if(
			parent.directories.begin(),
			parent.directories.end(),
			[&directoryName](const SpriteTextureDirectory& directory) {
				return directory.name == directoryName;
			}
		);
		if (found != parent.directories.end()) {
			return *found;
		}

		parent.directories.push_back(SpriteTextureDirectory{ directoryName });
		return parent.directories.back();
	}

	/// @brief Texture階層をDirectory名とFile名で再帰的に整列
	/// @param directory 整列対象Directory
	void SortTextureDirectory(SpriteTextureDirectory& directory) {
		std::sort(
			directory.directories.begin(),
			directory.directories.end(),
			[](const SpriteTextureDirectory& left, const SpriteTextureDirectory& right) {
				return left.name < right.name;
			}
		);
		std::sort(
			directory.textures.begin(),
			directory.textures.end(),
			[](const SpriteTextureItem& left, const SpriteTextureItem& right) {
				return left.fileName < right.fileName;
			}
		);
		for (SpriteTextureDirectory& child : directory.directories) {
			SortTextureDirectory(child);
		}
	}

	/// @brief Sprite Editor専用の実File階層に対応したTexture Treeを構築
	/// @param textureNames TextureManagerに登録済みのTexture名一覧
	/// @return 実File階層と実Fileを持たないTexture名一覧
	SpriteTextureTree CreateSpriteTextureTree(const std::vector<std::string>& textureNames) {
		SpriteTextureTree tree;
		tree.root.name = kTextureDirectoryPath.generic_string();
		const std::unordered_set<std::string> selectableNames(
			textureNames.begin(),
			textureNames.end()
		);
		std::unordered_set<std::string> mappedNames;
		std::error_code errorCode;
		std::filesystem::recursive_directory_iterator iterator(
			kTextureDirectoryPath,
			std::filesystem::directory_options::skip_permission_denied,
			errorCode
		);
		const std::filesystem::recursive_directory_iterator end;
		for (; !errorCode && iterator != end; iterator.increment(errorCode)) {
			if (!iterator->is_regular_file(errorCode) || errorCode) {
				continue;
			}
			const std::filesystem::path& filePath = iterator->path();
			if (filePath.extension() != ".png") {
				continue;
			}

			const std::string textureName = filePath.stem().string();
			if (!selectableNames.contains(textureName)) {
				continue;
			}
			const std::filesystem::path relativePath =
				std::filesystem::relative(filePath, kTextureDirectoryPath, errorCode);
			if (errorCode) {
				break;
			}

			// TextureManagerの管理KeyはFile名のため、実Pathを表示用階層としてのみ保持
			SpriteTextureDirectory* directory = &tree.root;
			for (const std::filesystem::path& component : relativePath.parent_path()) {
				directory = &FindOrAddTextureDirectory(*directory, component.string());
			}
			directory->textures.push_back(SpriteTextureItem{
				textureName,
				filePath.filename().string(),
				relativePath.generic_string(),
			});
			mappedNames.insert(textureName);
		}

		for (const std::string& textureName : textureNames) {
			if (!mappedNames.contains(textureName)) {
				tree.otherTextureNames.push_back(textureName);
			}
		}
		SortTextureDirectory(tree.root);
		return tree;
	}

	/// @brief Directory配下に選択中Textureが存在するか確認
	/// @param directory 確認対象Directory
	/// @param selectedName 選択中Texture名
	/// @return 選択中Textureが存在する場合はtrue
	bool ContainsSelectedTexture(
		const SpriteTextureDirectory& directory,
		const std::string& selectedName) {
		if (std::any_of(
			directory.textures.begin(),
			directory.textures.end(),
			[&selectedName](const SpriteTextureItem& texture) {
				return texture.textureName == selectedName;
			})) {
			return true;
		}
		return std::any_of(
			directory.directories.begin(),
			directory.directories.end(),
			[&selectedName](const SpriteTextureDirectory& child) {
				return ContainsSelectedTexture(child, selectedName);
			}
		);
	}

	/// @brief Texture Directory配下の選択項目を再帰的に描画
	/// @param directory 描画対象Directory
	/// @param selectedName 現在選択中のTexture名
	/// @return 選択が変更された場合はtrue
	bool DrawTextureDirectory(
		const SpriteTextureDirectory& directory,
		std::string& selectedName) {
		bool isChanged = false;
		for (const SpriteTextureDirectory& child : directory.directories) {
			ImGui::PushID(child.name.c_str());
			if (ContainsSelectedTexture(child, selectedName)) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
			}
			const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			if (ImGui::TreeNodeEx("##TextureDirectory", flags, "%s", child.name.c_str())) {
				isChanged |= DrawTextureDirectory(child, selectedName);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		for (const SpriteTextureItem& texture : directory.textures) {
			ImGui::PushID(texture.relativePath.c_str());
			const bool isSelected = texture.textureName == selectedName;
			if (ImGui::Selectable(texture.fileName.c_str(), isSelected)) {
				selectedName = texture.textureName;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
			DrawHoveredTexturePreview(texture.textureName);
			ImGui::PopID();
		}
		return isChanged;
	}

	/// @brief テクスチャ選択Comboを描画
	/// @param label ImGuiで使用するラベル
	/// @param selectedName 現在選択中のテクスチャ名
	/// @param textureNames 選択候補のテクスチャ名一覧
	/// @return 選択が変更された場合はtrue
	bool DrawTextureCombo(
		const char* label,
		std::string& selectedName,
		const std::vector<std::string>& textureNames) {
		const char* preview = selectedName.empty() ? "テクスチャを選択" : selectedName.c_str();
		bool isChanged = false;
		const bool isComboOpen = ImGui::BeginCombo(label, preview);
		if (!isComboOpen) {
			DrawHoveredTexturePreview(selectedName);
		}
		if (isComboOpen) {
			const SpriteTextureTree tree = CreateSpriteTextureTree(textureNames);
			ImGui::TextDisabled("%s", tree.root.name.c_str());
			ImGui::Separator();
			isChanged |= DrawTextureDirectory(tree.root, selectedName);
			if (!tree.otherTextureNames.empty()) {
				if (ImGui::TreeNodeEx(
					"その他",
					ImGuiTreeNodeFlags_SpanAvailWidth)) {
					for (const std::string& textureName : tree.otherTextureNames) {
						const bool isSelected = textureName == selectedName;
						if (ImGui::Selectable(textureName.c_str(), isSelected)) {
							selectedName = textureName;
							isChanged = true;
						}
						DrawHoveredTexturePreview(textureName);
					}
					ImGui::TreePop();
				}
			}
			ImGui::EndCombo();
		}
		return isChanged;
	}

	/// @brief Spriteの描画レイヤー選択Comboを描画
	/// @param sprite 編集対象のSprite
	void DrawRenderLayerCombo(Sprite& sprite) {
		const Render::RenderLayer currentLayer = sprite.GetRenderLayer();
		if (ImGui::BeginCombo("描画レイヤー", Render::GetRenderLayerName(currentLayer))) {
			for (uint32_t index = 0; index < Render::kRenderLayerCount; ++index) {
				const Render::RenderLayer layer = Render::GetRenderLayerByIndex(index);
				const bool isSelected = layer == currentLayer;
				if (ImGui::Selectable(Render::GetRenderLayerName(layer), isSelected)) {
					sprite.SetRenderLayer(layer);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/// @brief Spriteの対象シーン選択Comboを描画
	/// @param sprite 編集対象のSprite
	void DrawSceneCombo(Sprite& sprite) {
		const SceneType currentScene = sprite.GetSceneType();
		if (ImGui::BeginCombo("対象シーン", SceneTypeToString(currentScene).c_str())) {
			const bool isNoneSelected = currentScene == SceneType::None;
			if (ImGui::Selectable("None", isNoneSelected)) {
				sprite.SetSceneType(SceneType::None);
			}
			if (isNoneSelected) {
				ImGui::SetItemDefaultFocus();
			}

			for (uint32_t index = 0; index < kSceneTypeCount; ++index) {
				const SceneType sceneType = GetSceneTypeByIndex(index);
				const bool isSelected = sceneType == currentScene;
				const std::string sceneName = SceneTypeToString(sceneType);
				if (ImGui::Selectable(sceneName.c_str(), isSelected)) {
					sprite.SetSceneType(sceneType);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/// @brief Spriteのアンカー選択Comboを描画
	/// @param sprite 編集対象のSprite
	void DrawAnchorCombo(Sprite& sprite) {
		struct AnchorItem {
			const char* label;
			Vector2 point;
		};

		constexpr AnchorItem anchorItems[] = {
			{ "左上", { 0.0f, 0.0f } },
			{ "右上", { 1.0f, 0.0f } },
			{ "中央", { 0.5f, 0.5f } },
			{ "左下", { 0.0f, 1.0f } },
			{ "右下", { 1.0f, 1.0f } },
		};

		const Vector2 currentPoint = sprite.GetAnchorPoint();
		const char* preview = "カスタム";
		for (const AnchorItem& item : anchorItems) {
			if (item.point == currentPoint) {
				preview = item.label;
				break;
			}
		}

		if (ImGui::BeginCombo("アンカー", preview)) {
			for (const AnchorItem& item : anchorItems) {
				const bool isSelected = item.point == currentPoint;
				if (ImGui::Selectable(item.label, isSelected)) {
					sprite.SetAnchorPoint(item.point);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		Vector2 anchorPoint = sprite.GetAnchorPoint();
		float anchorValues[2] = { anchorPoint.x, anchorPoint.y };
		if (ImGui::DragFloat2("アンカーポイント", anchorValues, 0.01f, 0.0f, 1.0f, "%.2f")) {
			sprite.SetAnchorPoint({ anchorValues[0], anchorValues[1] });
		}
	}

	/// @brief Spriteのテクスチャプレビューを描画
	/// @param sprite 表示対象のSprite
	void DrawTexturePreview(const Sprite& sprite) {
		const std::optional<TexturePreviewData> previewData = CreateTexturePreviewData(sprite.GetTextureName());
		if (!previewData) {
			return;
		}

		ImGui::Text(
			"プレビュー (%d x %d)",
			static_cast<int>(previewData->pixelSize.x),
			static_cast<int>(previewData->pixelSize.y));
		ImGui::Image(previewData->textureId, previewData->displaySize);
	}

	/// @brief Spriteのプロパティ編集UIを描画
	/// @param sprite 編集対象のSprite
	/// @param textureNames 選択候補のテクスチャ名一覧
	void DrawSpriteProperties(Sprite& sprite, const std::vector<std::string>& textureNames) {
		std::string textureName = sprite.GetTextureName();
		if (DrawTextureCombo("テクスチャ", textureName, textureNames)) {
			sprite.SetTexture(textureName);
		}

		DrawTexturePreview(sprite);
		ImGui::Separator();

		Vector2 position = sprite.GetPosition();
		float positionValues[2] = { position.x, position.y };
		if (ImGui::DragFloat2("位置", positionValues, 1.0f)) {
			sprite.SetPosition({ positionValues[0], positionValues[1] });
		}

		Vector2 scale = sprite.GetScale();
		float scaleValues[2] = { scale.x, scale.y };
		if (ImGui::DragFloat2("スケール", scaleValues, 0.01f, 0.0f, 100.0f, "%.2f")) {
			sprite.SetScale({ scaleValues[0], scaleValues[1] });
		}

		float rotationDegrees = sprite.GetRotation() * kRadiansToDegrees;
		if (ImGui::DragFloat("回転", &rotationDegrees, 0.5f, -360.0f, 360.0f, "%.1f度")) {
			sprite.SetRotation(rotationDegrees * kDegreesToRadians);
		}

		Vector4 color = sprite.GetColor();
		float colorValues[4] = { color.x, color.y, color.z, color.w };
		if (ImGui::ColorEdit4("色", colorValues)) {
			sprite.SetColor({ colorValues[0], colorValues[1], colorValues[2], colorValues[3] });
		}

		DrawAnchorCombo(sprite);

		bool isFitToScreen = sprite.IsFitToScreen();
		if (ImGui::Checkbox("画面全体へフィット", &isFitToScreen)) {
			sprite.SetFitToScreen(isFitToScreen);
		}

		DrawRenderLayerCombo(sprite);
		DrawSceneCombo(sprite);
	}

#endif // USE_IMGUI

} // namespace

bool LoadSpriteEditorJson() {
	return SpriteManager::GetInstance().LoadFromFile(kSpriteEditorJsonPath);
}

bool LoadSpriteEditorJson(SceneType sceneType) {
	return SpriteManager::GetInstance().LoadFromFile(kSpriteEditorJsonPath, sceneType);
}

#ifdef USE_IMGUI

void DrawSpriteManagerEditorUI(SceneType currentSceneType) {
	SpriteManager& manager = SpriteManager::GetInstance();
	const std::vector<std::string> textureNames = GetSelectableTextureNames();

	// 選択Handleと作成候補をFrame間で維持するEditor Session状態
	static std::array<char, 128> createName{};
	static std::string createTextureName;
	static SpriteHandle selectedHandle{};
	static bool isInitialized = false;
	if (!isInitialized) {
		CopyToBuffer(createName, "Sprite");
		if (!textureNames.empty()) {
			createTextureName = textureNames.front();
		}
		isInitialized = true;
	}

	ImGui::Begin("Sprite Editor");

	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputText("新規名", createName.data(), createName.size());
	ImGui::SameLine();
	if (textureNames.empty()) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("追加")) {
		const std::string requestedName = createName.data();
		const SpriteHandle created = manager.Create(
			requestedName,
			createTextureName,
			SceneType::None,
			EditorManagementMode::EditorManaged);
		if (created.IsValid()) {
			selectedHandle = created;
			CopyToBuffer(createName, MakeNextAvailableSpriteName(manager, requestedName));
		}
	}
	if (textureNames.empty()) {
		ImGui::EndDisabled();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(180.0f);
	DrawTextureCombo("新規テクスチャ", createTextureName, textureNames);
	if (textureNames.empty()) {
		ImGui::SameLine();
		ImGui::TextDisabled("利用可能なテクスチャがありません");
	}

	if (ImGui::Button("保存")) {
		manager.SaveToFile(kSpriteEditorJsonPath, currentSceneType);
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		LoadSpriteEditorJson(currentSceneType);
	}
	ImGui::SameLine();
	if (ImGui::Button("復元")) {
		std::filesystem::path backupPath = kSpriteEditorJsonPath;
		backupPath += ".bak";
		manager.LoadFromFile(backupPath, currentSceneType);
	}
	ImGui::SameLine();
	ImGui::Text("インスタンス数: %zu", manager.GetSpriteCount());

	ImGui::Separator();

	const std::vector<std::string> names = manager.GetEditorManagedNames();
	ImGui::BeginChild("SpriteList", ImVec2(200.0f, 0.0f), true);
	for (const std::string& name : names) {
		ImGui::PushID(name.c_str());
		const SpriteHandle handle = manager.Find(name);
		Sprite* sprite = manager.TryGet(handle);
		if (!sprite) {
			ImGui::PopID();
			continue;
		}

		bool isVisible = sprite->IsVisible();
		if (ImGui::Checkbox("##Visible", &isVisible)) {
			sprite->SetVisible(isVisible);
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

	ImGui::BeginChild("SpriteProperties", ImVec2(0.0f, 0.0f), true);
	Sprite* selectedSprite = manager.TryGet(selectedHandle);
	if (selectedSprite) {
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

		DrawSpriteProperties(*selectedSprite, textureNames);
	} else {
		ImGui::TextDisabled("Spriteを選択してください。");
	}
	ImGui::EndChild();

	ImGui::End();
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
