#pragma once

#ifdef USE_IMGUI

#include <string>

namespace MadoEngine::Editor {

	/// @brief テクスチャ選択とプレビューをまとめて描画するUI部品
	class TextureSelector final {
	public:
		/// @brief プレビューの最大表示サイズを指定して初期化
		/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
		explicit TextureSelector(float previewMaxSize = 64.0f);

		/// @brief テクスチャ選択Comboと選択中テクスチャのプレビューを描画
		/// @param label ImGuiで使用するラベル
		/// @param selectedTextureName 現在選択中のテクスチャ名
		/// @return 選択が変更された場合はtrue
		bool Draw(const char* label, std::string& selectedTextureName) const;

	private:
		float previewMaxSize_ = 64.0f;
	};

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
