#include "TextSdfGenerator.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace MadoEngine {
namespace {
	constexpr float kInfiniteDistance = 1.0e20f;
	constexpr std::size_t kMaximumPixels = 16 * 1024 * 1024;

	/// @brief 放物線の下包絡線から一次元の二乗距離を計算
	/// @param values 距離の初期値
	/// @param result 計算結果の出力先
	/// @param sites 最短距離候補の作業領域
	/// @param boundaries 候補の切り替え境界の作業領域
	void TransformLine(std::span<const float> values, std::span<float> result, std::span<int> sites, std::span<double> boundaries) {
		int lastSite = -1;

		// 有限の候補だけで包絡線を構築し、空白行の無限値同士の減算を回避
		for (int index = 0; index < static_cast<int>(values.size()); ++index) {
			if (values[index] >= kInfiniteDistance) {
				continue;
			}
			double boundary = -std::numeric_limits<double>::infinity();
			while (lastSite >= 0) {
				const int previous = sites[lastSite];
				boundary = (static_cast<double>(values[index]) + static_cast<double>(index) * index -
					values[previous] - static_cast<double>(previous) * previous) / (2.0 * (index - previous));
				if (boundary > boundaries[lastSite]) {
					break;
				}
				--lastSite;
			}
			++lastSite;
			sites[lastSite] = index;
			boundaries[lastSite] = boundary;
			boundaries[lastSite + 1] = std::numeric_limits<double>::infinity();
		}
		if (lastSite < 0) {
			std::fill(result.begin(), result.end(), kInfiniteDistance);
			return;
		}

		// 候補の切り替え位置を一方向にたどり、各行の計算量を画素数に比例
		int site = 0;
		for (int index = 0; index < static_cast<int>(values.size()); ++index) {
			while (boundaries[site + 1] < index) {
				++site;
			}
			const float delta = static_cast<float>(index - sites[site]);
			result[index] = delta * delta + values[sites[site]];
		}
	}

	/// @brief 縦横の一次元変換で二次元の二乗距離を計算
	/// @param distances 距離の初期値と計算結果
	/// @param width 画像幅
	/// @param height 画像高さ
	void TransformImage(std::vector<float>& distances, uint32_t width, uint32_t height) {
		const uint32_t length = (std::max)(width, height);
		std::vector<float> line(length);
		std::vector<float> result(length);
		std::vector<int> sites(length);
		std::vector<double> boundaries(static_cast<std::size_t>(length) + 1);

		// 縦方向の結果を横方向へ渡し、全画素からの総当たり探索を回避
		for (uint32_t x = 0; x < width; ++x) {
			for (uint32_t y = 0; y < height; ++y) {
				line[y] = distances[static_cast<std::size_t>(y) * width + x];
			}
			TransformLine({ line.data(), height }, { result.data(), height }, sites, boundaries);
			for (uint32_t y = 0; y < height; ++y) {
				distances[static_cast<std::size_t>(y) * width + x] = result[y];
			}
		}
		for (uint32_t y = 0; y < height; ++y) {
			const std::size_t offset = static_cast<std::size_t>(y) * width;
			TransformLine({ distances.data() + offset, width }, { result.data(), width }, sites, boundaries);
			std::copy_n(result.data(), width, distances.data() + offset);
		}
	}

} // namespace

bool TextSdfGenerator::Generate(const TextTexturePixels& source, TextTexturePixels& outPixels) {
	if (&source == &outPixels || source.width == 0 || source.height == 0 ||
		source.width > 16384 - kPadding * 2 || source.height > 16384 - kPadding * 2) {
		return false;
	}
	const uint32_t width = source.width + kPadding * 2;
	const uint32_t height = source.height + kPadding * 2;
	const std::size_t pixelCount = static_cast<std::size_t>(width) * height;
	if (pixelCount > kMaximumPixels || source.pixels.size() < static_cast<std::size_t>(source.width) * source.height * 4) {
		return false;
	}
	std::vector<float> inside(pixelCount, kInfiniteDistance);
	std::vector<float> outside(pixelCount, 0.0f);

	// 半透明の輪郭を小数の距離として残し、二値化による細線や小さい文字の変形を抑制
	for (uint32_t y = 0; y < source.height; ++y) {
		for (uint32_t x = 0; x < source.width; ++x) {
			const float alpha = source.pixels[(static_cast<std::size_t>(y) * source.width + x) * 4 + 3] / 255.0f;
			const std::size_t index = static_cast<std::size_t>(y + kPadding) * width + x + kPadding;
			if (alpha >= 1.0f) {
				inside[index] = 0.0f;
				outside[index] = kInfiniteDistance;
			} else if (alpha > 0.0f) {
				const float inward = (std::max)(0.0f, 0.5f - alpha);
				const float outward = (std::max)(0.0f, alpha - 0.5f);
				inside[index] = inward * inward;
				outside[index] = outward * outward;
			}
		}
	}
	TransformImage(inside, width, height);
	TransformImage(outside, width, height);
	outPixels.width = width;
	outPixels.height = height;
	outPixels.pixels.resize(pixelCount * 4);

	// 距離ゼロを0.5へ対応させ、生成範囲内の縁幅をShader定数だけで変更可能な形式
	for (std::size_t index = 0; index < pixelCount; ++index) {
		const float distance = std::sqrt(outside[index]) - std::sqrt(inside[index]);
		const float normalized = std::clamp(0.5f + distance / (kDistanceRange * 2.0f), 0.0f, 1.0f);
		outPixels.pixels[index * 4] = 255;
		outPixels.pixels[index * 4 + 1] = 255;
		outPixels.pixels[index * 4 + 2] = 255;
		outPixels.pixels[index * 4 + 3] = static_cast<uint8_t>(std::round(normalized * 255.0f));
	}
	return true;
}

} // namespace MadoEngine
