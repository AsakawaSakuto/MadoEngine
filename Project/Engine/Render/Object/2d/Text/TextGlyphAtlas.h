#pragma once
#include "TextTextureGenerator.h"
#include "Render/Object/2d/Sprite/SpriteData.h"
#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace MadoEngine {

/// @brief 画像再生成を伴わない単行文字列の配置設定
struct TextAtlasLayoutDesc {
	float fontSize = 24.0f;
	float lineSpacing = 1.0f;
	float characterSpacing = 0.0f;
	Vector2 areaSize = {};
	TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Left;
	TextVerticalAlign verticalAlign = TextVerticalAlign::Top;
	bool wordWrap = true;
};

/// @brief 数字と基本記号を共有するフォント画像と単行描画の組み立て
class TextGlyphAtlas {
public:
	static constexpr std::size_t kMaximumCharacters = 64;
	static constexpr std::size_t kMaximumVertices = kMaximumCharacters * 6;

	/// @brief 一文字の画像範囲とフォント計測値
	struct Glyph {
		Vector2 uvMin = {};
		Vector2 uvMax = {};
		Vector2 pixelSize = {};
		Vector2 layoutSize = {};
		float padding = 0.0f;
		TextGlyphMetrics metrics;
		bool isAvailable = false;
	};

	/// @brief 指定フォントから数字と基本記号の画像を生成
	/// @param desc フォントと画像化サイズの設定
	/// @param outPixels 共有するアトラス画像
	/// @param sdfEnabled 単一チャンネルSDFへ変換する場合はtrue
	/// @return 全文字の生成に成功した場合はtrue
	bool Generate(const TextTextureDesc& desc, TextTexturePixels& outPixels, bool sdfEnabled = false);

	/// @brief 指定文字列をアトラスで表示可能か確認
	/// @param text 表示するUTF-8文字列
	/// @return 文字数制限内ですべての文字が登録済みの場合はtrue
	bool CanRender(std::string_view text) const;

	/// @brief 一文字の画像範囲とフォント計測値を取得
	/// @param character 取得するASCII文字
	/// @return 登録済み文字の情報、未登録の場合はnullptr
	const Glyph* FindGlyph(char character) const;

	/// @brief 文字列全体を一度に描画する正規化頂点を構築
	/// @param text 表示するUTF-8文字列
	/// @param desc 文字サイズ、間隔、表示領域、配置の設定
	/// @param vertices 出力先の確保済み頂点領域
	/// @param outVertexCount 構築した頂点数
	/// @param outSize 文字列の表示領域サイズ
	/// @return アトラス方式で組み立て可能な場合はtrue
	bool BuildVertices(std::string_view text, const TextAtlasLayoutDesc& desc, std::span<SpriteVertexData> vertices, uint32_t& outVertexCount, Vector2& outSize) const;

private:
	std::array<Glyph, 128> glyphs_;
	float fontSize_ = 0.0f;
	float baseline_ = 0.0f;
	float lineHeight_ = 0.0f;
};

} // namespace MadoEngine
