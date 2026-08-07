#include "TextEditorUI.h"
#include "Render/Object/2d/Text/TextManager.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <vector>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

namespace {

	/// @brief バッファへ文字列をコピーします。
	/// @tparam Size バッファサイズ。
	/// @param buffer コピー先バッファ。
	/// @param text コピーする文字列。
	template<size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 追加したText名を基準に次の未使用名を生成する
	/// @param manager Text名の使用状況を確認するManager
	/// @param createdName 直前に追加したText名
	/// @return 末尾の番号を繰り上げた未使用のText名
	std::string MakeNextAvailableTextName(
		const TextManager& manager,
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

	/// @brief Textのアンカー選択Comboを描画する
	/// @param text 編集対象のText
	void DrawAnchorCombo(Text& text) {
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

		const Vector2 currentPoint = text.GetAnchorPoint();
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
					text.SetAnchorPoint(item.point);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/// @brief RenderLayerを選択するComboを描画します。
	/// @param text 編集対象Text。
	void DrawRenderLayerCombo(Text& text) {
		struct LayerItem {
			const char* label;
			Render::RenderLayer layer;
		};

		const LayerItem items[] = {
			{ "Default",               Render::RenderLayer::Default },
			{ "World",                 Render::RenderLayer::World },
			{ "MapEventObject",        Render::RenderLayer::MapEventObject },
			{ "MapEventObjectOutline", Render::RenderLayer::MapEventObjectOutline },
			{ "Player",                Render::RenderLayer::Player },
			{ "Effect",                Render::RenderLayer::Effect },
			{ "UI",					   Render::RenderLayer::UI },
			{ "Debug",                 Render::RenderLayer::Debug },
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
		const char* preview = currentType == TextFontFamilyType::Invalid
			? text.GetFontFamily().c_str()
			: GetTextFontDisplayName(currentType);

		if (ImGui::BeginCombo("フォント", preview)) {
			for (const TextFontDefinition& definition : kTextFontDefinitions) {
				const TextFontFamilyType type = definition.type;
				const bool selected = type == currentType;
				if (ImGui::Selectable(GetTextFontDisplayName(type), selected)) {
					text.SetFontFamily(GetTextFontFamilyName(type));
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			if (currentType == TextFontFamilyType::Invalid && !text.GetFontFamily().empty()) {
				ImGui::Separator();
				ImGui::TextDisabled("現在のフォントは候補外です。");
			}

			ImGui::EndCombo();
		}
	}

	/// @brief Text配置を選択するComboを描画します。
	/// @param text 編集対象Text。
	void DrawAlignmentControls(Text& text) {
		const char* horizontalItems[] = { "左", "中央", "右" };
		int horizontalIndex = static_cast<int>(text.GetHorizontalAlign());
		if (ImGui::Combo("横揃え", &horizontalIndex, horizontalItems, 3)) {
			text.SetHorizontalAlign(static_cast<TextHorizontalAlign>(horizontalIndex));
		}

		const char* verticalItems[] = { "上", "中央", "下" };
		int verticalIndex = static_cast<int>(text.GetVerticalAlign());
		if (ImGui::Combo("縦揃え", &verticalIndex, verticalItems, 3)) {
			text.SetVerticalAlign(static_cast<TextVerticalAlign>(verticalIndex));
		}
	}

} // namespace

#endif // USE_IMGUI

bool LoadTextEditorJson() {
	return TextManager::GetInstance().LoadFromFile("Assets/Json/TextObjects.json");
}

#ifdef USE_IMGUI

void DrawTextManagerEditorUI() {
	TextManager& manager = TextManager::GetInstance();

	static std::array<char, 128> createName{};
	static TextHandle selectedHandle{};
	static TextHandle editingHandle{};
	static std::array<char, 4096> textBuffer{};
	static std::array<char, 128> screenBuffer{};
	static bool isBufferInitialized = false;
	if (!isBufferInitialized) {
		CopyToBuffer(createName, "Text");
		isBufferInitialized = true;
	}

	ImGui::Begin("Text Editor");

	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputText("新規名", createName.data(), createName.size());
	ImGui::SameLine();
	if (ImGui::Button("追加")) {
		const std::string requestedName = createName.data();
		const TextHandle created = manager.Create(
			requestedName,
			SceneType::None,
			EditorManagementMode::EditorManaged);
		if (created.IsValid()) {
			selectedHandle = created;
			CopyToBuffer(createName, MakeNextAvailableTextName(manager, requestedName));
			editingHandle = {};
		}
	}
	if (ImGui::Button("保存")) {
		manager.SaveToFile("Assets/Json/TextObjects.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		LoadTextEditorJson();
		editingHandle = {};
	}
	ImGui::SameLine();
	if (ImGui::Button("復元")) {
		manager.LoadFromFile("Assets/Json/TextObjects.json.bak");
		editingHandle = {};
	}
	ImGui::SameLine();
	ImGui::Text("インスタンス数: %zu", manager.GetTextCount());

	ImGui::Separator();

	const std::vector<std::string> names = manager.GetEditorManagedNames();
	ImGui::BeginChild("TextList", ImVec2(180.0f, 0.0f), true);
	for (const std::string& name : names) {
		ImGui::PushID(name.c_str());
		const TextHandle handle = manager.Find(name);
		Text* text = manager.TryGet(handle);
		if (!text) {
			ImGui::PopID();
			continue;
		}

		bool visible = text->IsVisible();
		if (ImGui::Checkbox("##Visible", &visible)) {
			text->SetVisible(visible);
		}
		ImGui::SameLine();

		const bool selected = handle == selectedHandle;
		const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float selectableWidth = (std::max)(1.0f, ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::Selectable(name.c_str(), selected, 0, ImVec2(selectableWidth, 0.0f))) {
			selectedHandle = handle;
			editingHandle = {};
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("削除")) {
			manager.RequestDestroy(handle);
			if (selectedHandle == handle) {
				selectedHandle = {};
				editingHandle = {};
			}
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("TextProperties", ImVec2(0.0f, 0.0f), true);
	Text* selectedText = manager.TryGet(selectedHandle);
	if (selectedText) {
		std::string selectedName;
		for (const std::string& name : names) {
			if (manager.Find(name) == selectedHandle) {
				selectedName = name;
				break;
			}
		}
		if (editingHandle != selectedHandle) {
			CopyToBuffer(textBuffer, selectedText->GetText());
			CopyToBuffer(screenBuffer, selectedText->GetTargetScreen());
			editingHandle = selectedHandle;
		}

		std::array<char, 128> nameBuffer{};
		CopyToBuffer(nameBuffer, selectedName);
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
			const std::string newName = nameBuffer.data();
			if (!newName.empty()) {
				manager.Rename(selectedHandle, newName);
			}
		}

		if (ImGui::InputTextMultiline("本文", textBuffer.data(), textBuffer.size(), ImVec2(-1.0f, 120.0f))) {
			selectedText->SetText(textBuffer.data());
		}
		DrawFontCombo(*selectedText);

		float fontSize = selectedText->GetFontSize();
		if (ImGui::DragFloat("フォントサイズ", &fontSize, 0.5f, 1.0f, 1024.0f)) {
			selectedText->SetFontSize(fontSize);
		}

		float lineSpacing = selectedText->GetLineSpacing();
		if (ImGui::DragFloat("行間", &lineSpacing, 0.01f, 0.1f, 4.0f, "%.2f")) {
			selectedText->SetLineSpacing(lineSpacing);
		}

		float characterSpacing = selectedText->GetCharacterSpacing();
		if (ImGui::DragFloat("文字間", &characterSpacing, 0.1f, -64.0f, 1024.0f, "%.1f")) {
			selectedText->SetCharacterSpacing(characterSpacing);
		}

		Vector2 position = selectedText->GetPosition();
		float positionValues[2] = { position.x, position.y };
		if (ImGui::DragFloat2("位置", positionValues, 1.0f)) {
			selectedText->SetPosition({ positionValues[0], positionValues[1] });
		}

		DrawAnchorCombo(*selectedText);

		Vector2 areaSize = selectedText->GetAreaSize();
		float sizeValues[2] = { areaSize.x, areaSize.y };
		if (ImGui::DragFloat2("サイズ", sizeValues, 1.0f, 0.0f, 4096.0f)) {
			selectedText->SetAreaSize({ sizeValues[0], sizeValues[1] });
		}

		Vector4 color = selectedText->GetColor();
		float colorValues[4] = { color.x, color.y, color.z, color.w };
		if (ImGui::ColorEdit4("色", colorValues)) {
			selectedText->SetColor({ colorValues[0], colorValues[1], colorValues[2], colorValues[3] });
		}

		bool wordWrap = selectedText->IsWordWrapEnabled();
		if (ImGui::Checkbox("自動折り返し", &wordWrap)) {
			selectedText->SetWordWrap(wordWrap);
		}

		DrawAlignmentControls(*selectedText);
		DrawRenderLayerCombo(*selectedText);
		DrawSceneCombo(*selectedText);

		if (ImGui::InputText("スクリーン", screenBuffer.data(), screenBuffer.size())) {
			selectedText->SetTargetScreen(screenBuffer.data());
		}

	} else {
		ImGui::TextDisabled("Textを選択してください。");
	}
	ImGui::EndChild();

	ImGui::End();
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
