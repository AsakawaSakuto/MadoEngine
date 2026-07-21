#pragma once

#include <string>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

	/// @brief テクスチャ選択とプレビューをまとめて描画するUI部品
	class TextureSelector final {
	public:
		/// @brief プレビューの最大表示サイズを指定して初期化する
		/// @param previewMaxSize プレビュー画像の一辺あたりの最大表示サイズ
		explicit TextureSelector(float previewMaxSize = 64.0f);

		/// @brief テクスチャ選択Comboと選択中テクスチャのプレビューを描画する
		/// @param label ImGuiで使用するラベル
		/// @param selectedTextureName 現在選択中のテクスチャ名
		/// @return 選択が変更された場合はtrue
		bool Draw(const char* label, std::string& selectedTextureName) const;

	private:
		float previewMaxSize_ = 64.0f;
	};

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
