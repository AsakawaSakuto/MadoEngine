#include "TextGlyphAtlas.h"
#include "TextSdfGenerator.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace MadoEngine {

bool TextGlyphAtlas::Generate(const TextTextureDesc& desc, TextTexturePixels& outPixels, bool sdfEnabled) {
	constexpr std::string_view kCharacters = "0123456789+-.,%:/ ";
	constexpr uint32_t kColumns = 8;
	constexpr uint32_t kPadding = 2;
	std::array<TextTexturePixels, kCharacters.size()> images;
	std::array<TextGlyphMetrics, kCharacters.size()> metrics;
	glyphs_ = {};
	fontSize_ = 0.0f;
	baseline_ = 0.0f;
	lineHeight_ = 0.0f;
	uint32_t cellWidth = 0;
	uint32_t cellHeight = 0;
	float descent = 0.0f;
	if (!std::isfinite(desc.fontSize) || desc.fontSize < 1.0f || desc.fontSize > 512.0f) {
		return false;
	}

	// フォント変更時だけ各文字を画像化し、文字ごとの幅と共通ベースラインを保持
	for (std::size_t index = 0; index < kCharacters.size(); ++index) {
		if (!TextTextureGenerator::GetInstance().GenerateGlyph(desc, kCharacters[index], images[index], metrics[index])) {
			return false;
		}
		Glyph& glyph = glyphs_[static_cast<unsigned char>(kCharacters[index])];
		glyph.layoutSize = { static_cast<float>(images[index].width), static_cast<float>(images[index].height) };
		if (sdfEnabled) {

			// 距離画像の余白と文字本来の送り幅を分離し、SDF切り替え時の字間変更を防止
			TextTexturePixels sdfPixels;
			if (!TextSdfGenerator::Generate(images[index], sdfPixels)) {
				return false;
			}
			images[index] = std::move(sdfPixels);
			glyph.padding = static_cast<float>(TextSdfGenerator::kPadding);
		}
		cellWidth = (std::max)(cellWidth, images[index].width + kPadding * 2);
		cellHeight = (std::max)(cellHeight, images[index].height + kPadding * 2);
		baseline_ = (std::max)(baseline_, metrics[index].baseline);
		descent = (std::max)(descent, metrics[index].lineHeight - metrics[index].baseline);
	}
	fontSize_ = desc.fontSize;
	lineHeight_ = baseline_ + descent;
	outPixels.width = cellWidth * kColumns;
	outPixels.height = cellHeight * static_cast<uint32_t>((kCharacters.size() + kColumns - 1) / kColumns);
	outPixels.pixels.resize(static_cast<std::size_t>(outPixels.width) * outPixels.height * 4);

	// 線形補間で隣の文字や黒い透明色が混入しないよう白色透明の余白を確保
	for (std::size_t offset = 0; offset < outPixels.pixels.size(); offset += 4) {
		outPixels.pixels[offset] = 255;
		outPixels.pixels[offset + 1] = 255;
		outPixels.pixels[offset + 2] = 255;
		outPixels.pixels[offset + 3] = 0;
	}
	for (std::size_t index = 0; index < kCharacters.size(); ++index) {
		const TextTexturePixels& image = images[index];
		const uint32_t x = static_cast<uint32_t>(index % kColumns) * cellWidth + kPadding;
		const uint32_t y = static_cast<uint32_t>(index / kColumns) * cellHeight + kPadding;
		for (uint32_t row = 0; row < image.height; ++row) {
			const std::size_t sourceOffset = static_cast<std::size_t>(row) * image.width * 4;
			const std::size_t destinationOffset = (static_cast<std::size_t>(y + row) * outPixels.width + x) * 4;
			std::copy_n(image.pixels.data() + sourceOffset, static_cast<std::size_t>(image.width) * 4, outPixels.pixels.data() + destinationOffset);
		}
		Glyph& glyph = glyphs_[static_cast<unsigned char>(kCharacters[index])];
		glyph.uvMin = { static_cast<float>(x) / outPixels.width, static_cast<float>(y) / outPixels.height };
		glyph.uvMax = { static_cast<float>(x + image.width) / outPixels.width, static_cast<float>(y + image.height) / outPixels.height };
		glyph.pixelSize = { static_cast<float>(image.width), static_cast<float>(image.height) };
		glyph.metrics = metrics[index];
		glyph.isAvailable = true;
	}
	return true;
}

bool TextGlyphAtlas::CanRender(std::string_view text) const {

	// 日本語、改行、長文は全文字列方式へ委譲し、未登録文字の欠落を防止
	if (fontSize_ <= 0.0f || text.size() > kMaximumCharacters) {
		return false;
	}
	for (char character : text) {
		if (!FindGlyph(character)) {
			return false;
		}
	}
	return true;
}

const TextGlyphAtlas::Glyph* TextGlyphAtlas::FindGlyph(char character) const {
	const std::size_t index = static_cast<unsigned char>(character);
	if (index >= glyphs_.size() || !glyphs_[index].isAvailable) {
		return nullptr;
	}
	return &glyphs_[index];
}

bool TextGlyphAtlas::BuildVertices(std::string_view text, const TextAtlasLayoutDesc& desc, std::span<SpriteVertexData> vertices, uint32_t& outVertexCount, Vector2& outSize) const {
	outVertexCount = 0;
	if (!CanRender(text) || vertices.size() < text.size() * 6 ||
		!std::isfinite(desc.fontSize) || desc.fontSize <= 0.0f || desc.lineSpacing != 1.0f ||
		!std::isfinite(desc.characterSpacing) || !std::isfinite(desc.areaSize.x) || !std::isfinite(desc.areaSize.y)) {
		return false;
	}
	const float scale = desc.fontSize / fontSize_;
	float advance = 0.0f;
	float left = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	float bottom = lineHeight_ * scale;

	// 送り幅と画像の張り出しから表示領域を計測し、文字の基準位置とAnchorを整合
	for (char character : text) {
		const Glyph& glyph = *FindGlyph(character);
		const float glyphTop = (baseline_ - glyph.metrics.baseline + glyph.metrics.offset.y) * scale;
		left = (std::min)(left, advance + glyph.metrics.offset.x * scale);
		right = (std::max)(right, advance + (glyph.metrics.offset.x + glyph.layoutSize.x) * scale);
		top = (std::min)(top, glyphTop);
		bottom = (std::max)(bottom, glyphTop + glyph.layoutSize.y * scale);
		advance += glyph.metrics.advance * scale + desc.characterSpacing;
	}
	const float contentWidth = (std::max)(advance, right) - left;
	const float contentHeight = bottom - top;
	if (desc.wordWrap && desc.areaSize.x > 0.0f && contentWidth > desc.areaSize.x) {

		// 自動折り返しが必要な文章配置はDirectWriteのLayout規則を維持
		return false;
	}
	outSize = { (std::max)(1.0f, std::ceil(contentWidth)), (std::max)(1.0f, std::ceil(contentHeight)) };
	float offsetX = -left;
	float offsetY = -top;
	if (desc.areaSize.x > 0.0f) {
		outSize.x = (std::max)(1.0f, std::ceil(desc.areaSize.x));
		if (desc.horizontalAlign == TextHorizontalAlign::Center) {
			offsetX += (outSize.x - contentWidth) * 0.5f;
		} else if (desc.horizontalAlign == TextHorizontalAlign::Right) {
			offsetX += outSize.x - contentWidth;
		}
	}
	if (desc.areaSize.y > 0.0f) {
		outSize.y = (std::max)(1.0f, std::ceil(desc.areaSize.y));
		if (desc.verticalAlign == TextVerticalAlign::Center) {
			offsetY += (outSize.y - contentHeight) * 0.5f;
		} else if (desc.verticalAlign == TextVerticalAlign::Bottom) {
			offsetY += outSize.y - contentHeight;
		}
	}

	// 固定数の頂点へ全桁をまとめ、画像再生成と一文字ごとのDrawCallを回避
	advance = 0.0f;
	for (char character : text) {
		const Glyph& glyph = *FindGlyph(character);
		const float padding = glyph.padding * scale;
		const float x0 = offsetX + advance + glyph.metrics.offset.x * scale - padding;
		const float y0 = offsetY + (baseline_ - glyph.metrics.baseline + glyph.metrics.offset.y) * scale - padding;
		const float x1 = x0 + glyph.pixelSize.x * scale;
		const float y1 = y0 + glyph.pixelSize.y * scale;
		advance += glyph.metrics.advance * scale + desc.characterSpacing;
		float clipLeft = -padding;
		float clipTop = -padding;
		float clipRight = outSize.x + padding;
		float clipBottom = outSize.y + padding;
		if (desc.areaSize.x > 0.0f) {
			clipLeft = 0.0f;
			clipRight = outSize.x;
		}
		if (desc.areaSize.y > 0.0f) {
			clipTop = 0.0f;
			clipBottom = outSize.y;
		}
		if (character == ' ' || x1 <= clipLeft || y1 <= clipTop || x0 >= clipRight || y0 >= clipBottom) {
			continue;
		}

		// 明示領域からはみ出す文字をUVと同時に切り詰め、隣接UIへの描画漏れを防止
		const float clippedX0 = (std::max)(clipLeft, x0);
		const float clippedY0 = (std::max)(clipTop, y0);
		const float clippedX1 = (std::min)(clipRight, x1);
		const float clippedY1 = (std::min)(clipBottom, y1);
		const float u0 = std::lerp(glyph.uvMin.x, glyph.uvMax.x, (clippedX0 - x0) / (x1 - x0));
		const float v0 = std::lerp(glyph.uvMin.y, glyph.uvMax.y, (clippedY0 - y0) / (y1 - y0));
		const float u1 = std::lerp(glyph.uvMin.x, glyph.uvMax.x, (clippedX1 - x0) / (x1 - x0));
		const float v1 = std::lerp(glyph.uvMin.y, glyph.uvMax.y, (clippedY1 - y0) / (y1 - y0));
		const SpriteVertexData bottomLeft = { { clippedX0 / outSize.x, clippedY1 / outSize.y, 0.0f, 1.0f }, { u0, v1 } };
		const SpriteVertexData topLeft = { { clippedX0 / outSize.x, clippedY0 / outSize.y, 0.0f, 1.0f }, { u0, v0 } };
		const SpriteVertexData bottomRight = { { clippedX1 / outSize.x, clippedY1 / outSize.y, 0.0f, 1.0f }, { u1, v1 } };
		const SpriteVertexData topRight = { { clippedX1 / outSize.x, clippedY0 / outSize.y, 0.0f, 1.0f }, { u1, v0 } };
		vertices[outVertexCount++] = bottomLeft;
		vertices[outVertexCount++] = topLeft;
		vertices[outVertexCount++] = bottomRight;
		vertices[outVertexCount++] = topLeft;
		vertices[outVertexCount++] = topRight;
		vertices[outVertexCount++] = bottomRight;
	}
	return true;
}

} // namespace MadoEngine
