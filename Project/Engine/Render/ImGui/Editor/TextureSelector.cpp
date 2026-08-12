#include "TextureSelector.h"
#include "ImGuiHeaders.h"
#include "Core/TextureManager/TextureManager.h"
#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

namespace {

	const std::filesystem::path kTextureDirectoryPath = "Assets/Texture";

	/// @brief テクスチャプレビューの描画情報
	struct TexturePreviewData {
		ImTextureID textureId = ImTextureID_Invalid;
		ImVec2 displaySize{};
		Vector2 pixelSize{};
	};

	/// @brief Texture選択Treeへ表示するFile情報
	struct TextureSelectionItem {
		std::string textureName;
		std::string fileName;
		std::string relativePath;
	};

	/// @brief Texture選択TreeのDirectory情報
	struct TextureSelectionDirectory {
		std::string name;
		std::vector<TextureSelectionDirectory> directories;
		std::vector<TextureSelectionItem> textures;
	};

	/// @brief 実File階層と実Fileを持たないTexture候補
	struct TextureSelectionTree {
		TextureSelectionDirectory root;
		std::vector<std::string> otherTextureNames;
	};

	/// @brief テクスチャプレビューの描画情報を作成
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
	/// @return 描画情報、テクスチャが見つからない場合はstd::nullopt
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

		// Aspect比を維持したまま指定最大Size内に収まるPreview倍率
		const float scale = (std::min)(1.0f, previewMaxSize / (std::max)(width, height));
		const D3D12_GPU_DESCRIPTOR_HANDLE handle = textureManager.GetSrvHandleGPU(textureIndex);

		return TexturePreviewData{
			static_cast<ImTextureID>(handle.ptr),
			{ width * scale, height * scale },
			pixelSize,
		};
	}

	/// @brief 直前のImGui項目にカーソルが重なっている場合にテクスチャプレビューを表示
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

	/// @brief 選択中テクスチャのプレビューを描画
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

	/// @brief Texture階層内の子Directoryを取得または追加
	/// @param parent 親Directory
	/// @param directoryName 子Directory名
	/// @return 取得または追加した子Directory
	TextureSelectionDirectory& FindOrAddTextureDirectory(
		TextureSelectionDirectory& parent,
		const std::string& directoryName) {
		const auto found = std::find_if(
			parent.directories.begin(),
			parent.directories.end(),
			[&directoryName](const TextureSelectionDirectory& directory) {
				return directory.name == directoryName;
			}
		);
		if (found != parent.directories.end()) {
			return *found;
		}

		parent.directories.push_back(TextureSelectionDirectory{ directoryName });
		return parent.directories.back();
	}

	/// @brief Texture階層をDirectory名とFile名で再帰的に整列
	/// @param directory 整列対象Directory
	void SortTextureDirectory(TextureSelectionDirectory& directory) {
		std::sort(
			directory.directories.begin(),
			directory.directories.end(),
			[](const TextureSelectionDirectory& left, const TextureSelectionDirectory& right) {
				return left.name < right.name;
			}
		);
		std::sort(
			directory.textures.begin(),
			directory.textures.end(),
			[](const TextureSelectionItem& left, const TextureSelectionItem& right) {
				return left.fileName < right.fileName;
			}
		);
		for (TextureSelectionDirectory& child : directory.directories) {
			SortTextureDirectory(child);
		}
	}

	/// @brief 実File階層に対応したTexture選択Treeを構築
	/// @param textureNames TextureManagerに登録済みのTexture名一覧
	/// @return 実File階層と実Fileを持たないTexture名一覧
	TextureSelectionTree CreateTextureSelectionTree(
		const std::vector<std::string>& textureNames) {
		TextureSelectionTree tree;
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
			TextureSelectionDirectory* directory = &tree.root;
			for (const std::filesystem::path& component : relativePath.parent_path()) {
				directory = &FindOrAddTextureDirectory(*directory, component.string());
			}
			directory->textures.push_back(TextureSelectionItem{
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
		const TextureSelectionDirectory& directory,
		const std::string& selectedName) {
		if (std::any_of(
			directory.textures.begin(),
			directory.textures.end(),
			[&selectedName](const TextureSelectionItem& texture) {
				return texture.textureName == selectedName;
			})) {
			return true;
		}
		return std::any_of(
			directory.directories.begin(),
			directory.directories.end(),
			[&selectedName](const TextureSelectionDirectory& child) {
				return ContainsSelectedTexture(child, selectedName);
			}
		);
	}

	/// @brief Texture Directory配下の選択項目を再帰的に描画
	/// @param directory 描画対象Directory
	/// @param selectedName 現在選択中のTexture名
	/// @param previewMaxSize Preview画像の一辺あたりの最大表示Size
	/// @return 選択が変更された場合はtrue
	bool DrawTextureDirectory(
		const TextureSelectionDirectory& directory,
		std::string& selectedName,
		float previewMaxSize) {
		bool isChanged = false;
		for (const TextureSelectionDirectory& child : directory.directories) {
			ImGui::PushID(child.name.c_str());
			if (ContainsSelectedTexture(child, selectedName)) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
			}
			if (ImGui::TreeNodeEx(
				"##TextureDirectory",
				ImGuiTreeNodeFlags_SpanAvailWidth,
				"%s",
				child.name.c_str())) {
				isChanged |= DrawTextureDirectory(child, selectedName, previewMaxSize);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		for (const TextureSelectionItem& texture : directory.textures) {
			ImGui::PushID(texture.relativePath.c_str());
			const bool isSelected = texture.textureName == selectedName;
			if (ImGui::Selectable(texture.fileName.c_str(), isSelected)) {
				selectedName = texture.textureName;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
			DrawHoveredTexturePreview(texture.textureName, previewMaxSize);
			ImGui::PopID();
		}
		return isChanged;
	}

	/// @brief 実Fileを持たないTexture選択項目を描画
	/// @param textureNames 描画対象Texture名一覧
	/// @param selectedName 現在選択中のTexture名
	/// @param previewMaxSize Preview画像の一辺あたりの最大表示Size
	/// @return 選択が変更された場合はtrue
	bool DrawOtherTextures(
		const std::vector<std::string>& textureNames,
		std::string& selectedName,
		float previewMaxSize) {
		if (textureNames.empty()) {
			return false;
		}
		if (std::find(textureNames.begin(), textureNames.end(), selectedName) != textureNames.end()) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
		}
		if (!ImGui::TreeNodeEx("その他", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return false;
		}

		bool isChanged = false;
		for (const std::string& textureName : textureNames) {
			const bool isSelected = textureName == selectedName;
			if (ImGui::Selectable(textureName.c_str(), isSelected)) {
				selectedName = textureName;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
			DrawHoveredTexturePreview(textureName, previewMaxSize);
		}
		ImGui::TreePop();
		return isChanged;
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

	// Combo閉鎖時は選択項目、展開時は各候補へ同じHover Previewを提供
	if (!isComboOpen && isSelectedTextureAvailable) {
		DrawHoveredTexturePreview(selectedTextureName, previewMaxSize_);
	}
	if (isComboOpen) {
		const TextureSelectionTree tree = CreateTextureSelectionTree(textureNames);
		ImGui::TextDisabled("%s", tree.root.name.c_str());
		ImGui::Separator();
		isChanged |= DrawTextureDirectory(
			tree.root,
			selectedTextureName,
			previewMaxSize_
		);
		isChanged |= DrawOtherTextures(
			tree.otherTextureNames,
			selectedTextureName,
			previewMaxSize_
		);
		ImGui::EndCombo();
	}

	if (isSelectedTextureAvailable || isChanged) {
		DrawSelectedTexturePreview(selectedTextureName, previewMaxSize_);
	}
	return isChanged;
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
