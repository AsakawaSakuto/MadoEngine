#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace MadoEngine {

/// @brief ASCII英字の大小とパス区切りの差を吸収するアセット検索キー
class AssetKey {
public:
	/// @brief 検索キーの表記差を吸収した文字列を生成
	/// @param value アセット名またはファイルパス
	/// @return ASCII英字を小文字、パス区切りをスラッシュへ統一した文字列
	static std::string Normalize(std::string_view value) {
		std::string normalized(value);
		for (char& character : normalized) {
			character = NormalizeCharacter(character);
		}
		return normalized;
	}

	/// @brief 表記差を吸収してアセット検索キーを比較
	/// @param left 比較元のアセット名またはファイルパス
	/// @param right 比較先のアセット名またはファイルパス
	/// @return 同じ検索キーを表す場合はtrue
	static bool Equals(std::string_view left, std::string_view right) noexcept {
		if (left.size() != right.size()) {
			return false;
		}

		for (std::size_t index = 0; index < left.size(); ++index) {
			if (NormalizeCharacter(left[index]) != NormalizeCharacter(right[index])) {
				return false;
			}
		}
		return true;
	}

	/// @brief 元のファイル名を保持したまま表記差を吸収するハッシュ
	struct Hash {
		/// @brief 正規化後の文字列と等価なハッシュ値を算出
		/// @param value アセット名またはファイルパス
		/// @return 表記差に依存しないハッシュ値
		std::size_t operator()(std::string_view value) const noexcept {
			std::uint64_t hash = 14695981039346656037ull;
			for (const char character : value) {
				hash ^= static_cast<unsigned char>(NormalizeCharacter(character));
				hash *= 1099511628211ull;
			}
			return static_cast<std::size_t>(hash);
		}
	};

	/// @brief アセット検索用コンテナの等価判定
	struct Equal {
		/// @brief アセット検索キーを比較
		/// @param left 比較元の検索キー
		/// @param right 比較先の検索キー
		/// @return 同じアセットを表す場合はtrue
		bool operator()(std::string_view left, std::string_view right) const noexcept {
			return Equals(left, right);
		}
	};

private:
	/// @brief UTF-8の非ASCIIバイトを保持して検索用の一文字を正規化
	/// @param character 正規化する文字
	/// @return ASCII英字の大小とパス区切りを統一した文字
	static char NormalizeCharacter(char character) noexcept {
		if (character >= 'A' && character <= 'Z') {
			return static_cast<char>(character + ('a' - 'A'));
		}
		if (character == '\\') {
			return '/';
		}
		return character;
	}
};

} // namespace MadoEngine
