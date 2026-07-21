#include "TextureSelector.h"
#include "ImGuiHeaders.h"
#include "Core/TextureManager/TextureManager.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

namespace {

	/// @brief テクスチャプレビューの描画情報
	struct TexturePreviewData {
		ImTextureID textureId = ImTextureID_Invalid;
		ImVec2 displaySize{};
		Vector2 pixelSize{};
	};

	/// @brief テクスチャプレビューの描画情報を作成する
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
	/// @return 描画情報。テクスチャが見つからない場合はstd::nullopt
	std::optional<TexturePreviewData> CreateTexturePreviewData(
		const std::string& textureName,
		float previewMaxSize) {
		TextureManager& textureManager = TextureManager::GetInstance();
		const uint32_t textureIndex = textureManager.GetTextureIndex(textureName);
		if (textureIndex == UINT32_MAX) {
			return std::nullopt;
		}

		const Vector2 pixelSize = textureManager.GetPixelSize(textureName);
		const float width = (std::max)(1.0f, pixelSize.x);
		const float height = (std::max)(1.0f, pixelSize.y);
		const float scale = (std::min)(1.0f, previewMaxSize / (std::max)(width, height));
		const D3D12_GPU_DESCRIPTOR_HANDLE handle = textureManager.GetSrvHandleGPU(textureIndex);

		return TexturePreviewData{
			static_cast<ImTextureID>(handle.ptr),
			{ width * scale, height * scale },
			pixelSize,
		};
	}

	/// @brief 直前のImGui項目にカーソルが重なっている場合にテクスチャプレビューを表示する
	/// @param textureName プレビュー対象のテクスチャ名
	/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
	void DrawHoveredTexturePreview(const std::string& textureName, float previewMaxSize) {
		if (!ImGui::IsItemHovered()) {
			return;
		}

		const std::optional<TexturePreviewData> previewData =
			CreateTexturePreviewData(textureName, previewMaxSize);
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

	/// @brief 選択中テクスチャのプレビューを描画する
	/// @param textureName プレビュー対象のテクスチャ名
	/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
	void DrawSelectedTexturePreview(const std::string& textureName, float previewMaxSize) {
		const std::optional<TexturePreviewData> previewData =
			CreateTexturePreviewData(textureName, previewMaxSize);
		if (!previewData) {
			return;
		}

		ImGui::Text(
			"プレビュー (%d x %d)",
			static_cast<int>(previewData->pixelSize.x),
			static_cast<int>(previewData->pixelSize.y));
		ImGui::Image(previewData->textureId, previewData->displaySize);
	}

} // namespace

TextureSelector::TextureSelector(float previewMaxSize)
	: previewMaxSize_((std::max)(1.0f, previewMaxSize)) {
}

bool TextureSelector::Draw(const char* label, std::string& selectedTextureName) const {
	const std::vector<std::string> textureNames = TextureManager::GetInstance().GetTextureNames();
	const bool isSelectedTextureAvailable =
		std::find(textureNames.begin(), textureNames.end(), selectedTextureName) != textureNames.end();
	const char* comboPreview = selectedTextureName.empty()
		? "テクスチャを選択"
		: selectedTextureName.c_str();
	bool isChanged = false;

	const bool isComboOpen = ImGui::BeginCombo(label, comboPreview);
	if (!isComboOpen && isSelectedTextureAvailable) {
		DrawHoveredTexturePreview(selectedTextureName, previewMaxSize_);
	}
	if (isComboOpen) {
		for (const std::string& textureName : textureNames) {
			const bool isSelected = textureName == selectedTextureName;
			if (ImGui::Selectable(textureName.c_str(), isSelected)) {
				selectedTextureName = textureName;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
			DrawHoveredTexturePreview(textureName, previewMaxSize_);
		}
		ImGui::EndCombo();
	}

	if (isSelectedTextureAvailable || isChanged) {
		DrawSelectedTexturePreview(selectedTextureName, previewMaxSize_);
	}
	return isChanged;
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
