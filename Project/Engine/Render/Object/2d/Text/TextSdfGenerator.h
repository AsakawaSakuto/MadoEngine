#pragma once
#include "TextTextureGenerator.h"

namespace MadoEngine {

/// @brief フォントや文字種に依存しない単一チャンネルの符号付き距離画像の生成
class TextSdfGenerator {
public:
	static constexpr float kDistanceRange = 32.0f;
	static constexpr uint32_t kPadding = 33;

	/// @brief 文字のAlpha画像を余白付きのSDF画像へ変換
	/// @param source 変換元のストレートRGBA文字画像
	/// @param outPixels Alphaへ距離を格納した余白付きRGBA画像
	/// @return 有効な画像サイズで変換に成功した場合はtrue
	static bool Generate(const TextTexturePixels& source, TextTexturePixels& outPixels);
};

} // namespace MadoEngine
