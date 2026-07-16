#include "SpriteEditor.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Core/TextureManager/TextureManager.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <vector>

namespace MadoEngine::Editor {

namespace {

	const std::filesystem::path kSpriteEditorJsonPath = "Assets/Json/SpriteObjects.json";

#ifdef USE_IMGUI

	constexpr float kRadiansToDegrees = 57.29577951308232f;
	constexpr float kDegreesToRadians = 0.017453292519943295f;
	constexpr float kTexturePreviewMaxSize = 64.0f;

	struct TexturePreviewData {
		ImTextureID textureId = ImTextureID_Invalid;
		ImVec2 displaySize{};
		Vector2 pixelSize{};
	};

	/// @brief テクスチャプレビューの描画情報を作成する
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @return 描画情報。テクスチャが見つからない場合はstd::nullopt
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

	/// @brief 直前のImGui項目にカーソルが重なっている場合にテクスチャプレビューを表示する
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

	/// @brief 文字列を固定長バッファへコピーする
	/// @tparam Size バッファサイズ
	/// @param buffer コピー先バッファ
	/// @param text コピー元文字列
	template<size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief Sprite Editorで選択可能な静的テクスチャ名を取得する
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

	/// @brief テクスチャ選択Comboを描画する
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
			for (const std::string& textureName : textureNames) {
				const bool isSelected = textureName == selectedName;
				if (ImGui::Selectable(textureName.c_str(), isSelected)) {
					selectedName = textureName;
					isChanged = true;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
				DrawHoveredTexturePreview(textureName);
			}
			ImGui::EndCombo();
		}
		return isChanged;
	}

	/// @brief Spriteの描画レイヤー選択Comboを描画する
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

	/// @brief Spriteの対象シーン選択Comboを描画する
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

	/// @brief Spriteのアンカー選択Comboを描画する
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

	/// @brief Spriteのテクスチャプレビューを描画する
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

	/// @brief Spriteのプロパティ編集UIを描画する
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

		bool isVisible = sprite.IsVisible();
		if (ImGui::Checkbox("表示", &isVisible)) {
			sprite.SetVisible(isVisible);
		}

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

#ifdef USE_IMGUI

void DrawSpriteManagerEditorUI() {
	SpriteManager& manager = SpriteManager::GetInstance();
	const std::vector<std::string> textureNames = GetSelectableTextureNames();

	static std::array<char, 128> createName{};
	static std::string createTextureName;
	static std::string selectedName;
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
	ImGui::SetNextItemWidth(180.0f);
	DrawTextureCombo("新規テクスチャ", createTextureName, textureNames);
	ImGui::SameLine();
	if (textureNames.empty()) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("追加")) {
		Sprite* created = manager.Create(
			createName.data(),
			createTextureName,
			SceneType::None,
			EditorManagementMode::EditorManaged);
		if (created) {
			selectedName = createName.data();
		}
	}
	if (textureNames.empty()) {
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("利用可能なテクスチャがありません");
	}

	if (ImGui::Button("保存")) {
		manager.SaveToFile(kSpriteEditorJsonPath);
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		LoadSpriteEditorJson();
	}
	ImGui::SameLine();
	if (ImGui::Button("復元")) {
		std::filesystem::path backupPath = kSpriteEditorJsonPath;
		backupPath += ".bak";
		manager.LoadFromFile(backupPath);
	}

	ImGui::Separator();

	const std::vector<std::string> names = manager.GetEditorManagedNames();
	ImGui::BeginChild("SpriteList", ImVec2(200.0f, 0.0f), true);
	for (const std::string& name : names) {
		ImGui::PushID(name.c_str());
		const bool isSelected = name == selectedName;
		const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float selectableWidth = (std::max)(
			1.0f,
			ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::Selectable(name.c_str(), isSelected, 0, ImVec2(selectableWidth, 0.0f))) {
			selectedName = name;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("削除")) {
			manager.Destroy(name);
			if (selectedName == name) {
				selectedName.clear();
			}
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("SpriteProperties", ImVec2(0.0f, 0.0f), true);
	Sprite* selectedSprite = nullptr;
	if (std::find(names.begin(), names.end(), selectedName) != names.end()) {
		selectedSprite = manager.Get(selectedName);
	}
	if (selectedSprite) {
		std::array<char, 128> nameBuffer{};
		CopyToBuffer(nameBuffer, selectedName);
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
			const std::string newName = nameBuffer.data();
			if (!newName.empty() && manager.Rename(selectedName, newName)) {
				selectedName = newName;
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
