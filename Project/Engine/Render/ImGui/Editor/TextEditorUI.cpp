#include "TextEditorUI.h"
#include "Render/Object/2d/Text/TextManager.h"
#include <array>
#include <cstring>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

namespace {

	/// @brief バッファへ文字列をコピーします。
	/// @tparam Size バッファサイズ。
	/// @param buffer コピー先。
	/// @param text コピー元文字列。
	template<size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief RenderLayerを選択するComboを描画します。
	/// @param text 編集対象Text。
	void DrawRenderLayerCombo(Text& text) {
		struct LayerItem {
			const char* label;
			Render::RenderLayer layer;
		};

		const LayerItem items[] = {
			{ "Default", Render::RenderLayer::Default },
			{ "World", Render::RenderLayer::World },
			{ "MapEventObject", Render::RenderLayer::MapEventObject },
			{ "Player", Render::RenderLayer::Player },
			{ "Effect", Render::RenderLayer::Effect },
			{ "UI", Render::RenderLayer::UI },
			{ "Debug", Render::RenderLayer::Debug },
		};

		const Render::RenderLayer current = text.GetRenderLayer();
		const char* preview = "Default";
		for (const LayerItem& item : items) {
			if (item.layer == current) {
				preview = item.label;
				break;
			}
		}

		if (ImGui::BeginCombo("Layer", preview)) {
			for (const LayerItem& item : items) {
				const bool selected = item.layer == current;
				if (ImGui::Selectable(item.label, selected)) {
					text.SetRenderLayer(item.layer);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/// @brief SceneTypeを選択するComboを描画します。
	/// @param text 編集対象Text。
	void DrawSceneCombo(Text& text) {
		struct SceneItem {
			const char* label;
			SceneType sceneType;
		};

		const SceneItem items[] = {
			{ "None", SceneType::None },
			{ "Title", SceneType::Title },
			{ "Game", SceneType::Game },
			{ "Result", SceneType::Result },
			{ "Test", SceneType::Test },
		};

		const SceneType current = text.GetSceneType();
		const char* preview = "None";
		for (const SceneItem& item : items) {
			if (item.sceneType == current) {
				preview = item.label;
				break;
			}
		}

		if (ImGui::BeginCombo("Scene", preview)) {
			for (const SceneItem& item : items) {
				const bool selected = item.sceneType == current;
				if (ImGui::Selectable(item.label, selected)) {
					text.SetSceneType(item.sceneType);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/// @brief Textフォント選択Comboを描画します。
	/// @param text 編集対象Text。
	void DrawFontCombo(Text& text) {
		const TextFontFamilyType currentType = GetTextFontFamilyTypeFromName(text.GetFontFamily());
		const char* preview = currentType == TextFontFamilyType::Count
			? text.GetFontFamily().c_str()
			: GetTextFontDisplayName(currentType);

		if (ImGui::BeginCombo("Font", preview)) {
			for (int index = 0; index < static_cast<int>(TextFontFamilyType::Count); ++index) {
				const TextFontFamilyType type = static_cast<TextFontFamilyType>(index);
				const bool selected = type == currentType;
				if (ImGui::Selectable(GetTextFontDisplayName(type), selected)) {
					text.SetFontFamily(GetTextFontFamilyName(type));
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			if (currentType == TextFontFamilyType::Count && !text.GetFontFamily().empty()) {
				ImGui::Separator();
				ImGui::TextDisabled("現在のフォントは候補外です");
			}

			ImGui::EndCombo();
		}
	}

	/// @brief Text配置を選択するComboを描画します。
	/// @param text 編集対象Text。
	void DrawAlignmentControls(Text& text) {
		const char* horizontalItems[] = { "Left", "Center", "Right" };
		int horizontalIndex = static_cast<int>(text.GetHorizontalAlign());
		if (ImGui::Combo("Horizontal", &horizontalIndex, horizontalItems, 3)) {
			text.SetHorizontalAlign(static_cast<TextHorizontalAlign>(horizontalIndex));
		}

		const char* verticalItems[] = { "Top", "Center", "Bottom" };
		int verticalIndex = static_cast<int>(text.GetVerticalAlign());
		if (ImGui::Combo("Vertical", &verticalIndex, verticalItems, 3)) {
			text.SetVerticalAlign(static_cast<TextVerticalAlign>(verticalIndex));
		}
	}

} // namespace

void DrawTextManagerEditorUI() {
	TextManager& manager = TextManager::GetInstance();

	static std::array<char, 128> createName{};
	static std::array<char, 260> jsonPath{};
	static std::string selectedName;
	static std::string editingName;
	static std::array<char, 4096> textBuffer{};
	static std::array<char, 128> screenBuffer{};
	static bool isBufferInitialized = false;
	if (!isBufferInitialized) {
		CopyToBuffer(createName, "Text");
		CopyToBuffer(jsonPath, "Assets/Json/TextObjects.json");
		isBufferInitialized = true;
	}

	ImGui::Begin("Text Editor");

	ImGui::InputText("New Name", createName.data(), createName.size());
	ImGui::SameLine();
	if (ImGui::Button("Create")) {
		Text* created = manager.Create(createName.data());
		if (created) {
			selectedName = createName.data();
			editingName.clear();
		}
	}

	ImGui::Separator();

	const std::vector<std::string> names = manager.GetNames();
	ImGui::BeginChild("TextList", ImVec2(180.0f, 0.0f), true);
	for (const std::string& name : names) {
		ImGui::PushID(name.c_str());
		const bool selected = name == selectedName;
		const float deleteButtonWidth = ImGui::CalcTextSize("Delete").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float selectableWidth = (std::max)(1.0f, ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::Selectable(name.c_str(), selected, 0, ImVec2(selectableWidth, 0.0f))) {
			selectedName = name;
			editingName.clear();
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Delete")) {
			manager.Destroy(name);
			if (selectedName == name) {
				selectedName.clear();
				editingName.clear();
			}
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("TextProperties", ImVec2(0.0f, 0.0f), true);
	Text* selectedText = manager.Get(selectedName);
	if (selectedText) {
		if (editingName != selectedName) {
			CopyToBuffer(textBuffer, selectedText->GetText());
			CopyToBuffer(screenBuffer, selectedText->GetTargetScreen());
			editingName = selectedName;
		}

		if (ImGui::InputTextMultiline("Text", textBuffer.data(), textBuffer.size(), ImVec2(-1.0f, 120.0f))) {
			selectedText->SetText(textBuffer.data());
		}
		DrawFontCombo(*selectedText);

		float fontSize = selectedText->GetFontSize();
		if (ImGui::DragFloat("Font Size", &fontSize, 0.5f, 1.0f, 256.0f)) {
			selectedText->SetFontSize(fontSize);
		}

		float lineSpacing = selectedText->GetLineSpacing();
		if (ImGui::DragFloat("Line Spacing", &lineSpacing, 0.01f, 0.1f, 4.0f, "%.2f")) {
			selectedText->SetLineSpacing(lineSpacing);
		}

		float characterSpacing = selectedText->GetCharacterSpacing();
		if (ImGui::DragFloat("Character Spacing", &characterSpacing, 0.1f, -64.0f, 256.0f, "%.1f")) {
			selectedText->SetCharacterSpacing(characterSpacing);
		}

		Vector2 position = selectedText->GetPosition();
		float positionValues[2] = { position.x, position.y };
		if (ImGui::DragFloat2("Position", positionValues, 1.0f)) {
			selectedText->SetPosition({ positionValues[0], positionValues[1] });
		}

		Vector2 areaSize = selectedText->GetAreaSize();
		float sizeValues[2] = { areaSize.x, areaSize.y };
		if (ImGui::DragFloat2("Size", sizeValues, 1.0f, 0.0f, 4096.0f)) {
			selectedText->SetAreaSize({ sizeValues[0], sizeValues[1] });
		}

		Vector4 color = selectedText->GetColor();
		float colorValues[4] = { color.x, color.y, color.z, color.w };
		if (ImGui::ColorEdit4("Color", colorValues)) {
			selectedText->SetColor({ colorValues[0], colorValues[1], colorValues[2], colorValues[3] });
		}

		bool visible = selectedText->IsVisible();
		if (ImGui::Checkbox("Visible", &visible)) {
			selectedText->SetVisible(visible);
		}

		bool wordWrap = selectedText->IsWordWrapEnabled();
		if (ImGui::Checkbox("Word Wrap", &wordWrap)) {
			selectedText->SetWordWrap(wordWrap);
		}

		DrawAlignmentControls(*selectedText);
		DrawRenderLayerCombo(*selectedText);
		DrawSceneCombo(*selectedText);

		if (ImGui::InputText("Screen", screenBuffer.data(), screenBuffer.size())) {
			selectedText->SetTargetScreen(screenBuffer.data());
		}

	} else {
		ImGui::TextDisabled("Textを選択してください。");
	}
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::InputText("Json Path", jsonPath.data(), jsonPath.size());
	if (ImGui::Button("Save Json")) {
		manager.SaveToFile(jsonPath.data());
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Json")) {
		manager.LoadFromFile(jsonPath.data());
		editingName.clear();
	}

	ImGui::End();
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
