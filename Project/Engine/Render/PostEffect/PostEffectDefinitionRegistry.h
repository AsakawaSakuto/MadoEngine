#pragma once

#include "Render/PostEffect/PostEffectType.h"
#include <cstddef>
#include <span>
#include <string_view>

namespace MadoEngine::Render {

/// @brief Editorと文字列APIで使用するfloatパラメータ定義
struct PostEffectFloatParameterDefinition {
	std::string_view key;
	std::string_view label;
	std::size_t offset = 0;
	float minValue = 0.0f;
	float maxValue = 1.0f;
	float speed = 0.01f;
	const std::string_view* selectionOptions = nullptr;
	std::size_t selectionOptionCount = 0;

	/// @brief 選択式UIへ表示する項目一覧を取得
	/// @return 選択項目の範囲、通常の数値Parameterの場合は空範囲
	[[nodiscard]] std::span<const std::string_view> GetSelectionOptions() const {
		return { selectionOptions, selectionOptionCount };
	}
};

/// @brief ポストエフェクトのShaderとパラメータレイアウト定義
struct PostEffectDefinition {
	PostEffectType type = PostEffectType::CopyImage;
	std::string_view typeName;
	std::string_view displayName;
	std::string_view shaderKey;
	const void* defaultParameterData = nullptr;
	std::size_t parameterSize = 0;
	const PostEffectFloatParameterDefinition* parameters = nullptr;
	std::size_t parameterCount = 0;

	/// @brief Editor用floatパラメータ定義を取得
	/// @return floatパラメータ定義の範囲
	[[nodiscard]] std::span<const PostEffectFloatParameterDefinition> GetParameters() const {
		return { parameters, parameterCount };
	}
};

/// @brief ポストエフェクト定義を一元管理するRegistry
class PostEffectDefinitionRegistry {
public:
	/// @brief PostEffectTypeから定義を取得
	/// @param type 検索するEffect種別
	/// @return 定義、範囲外の場合はnullptr
	[[nodiscard]] static const PostEffectDefinition* Find(PostEffectType type);

	/// @brief Shaderキーから定義を取得
	/// @param shaderKey 検索するPixelShaderキー
	/// @return 定義、見つからない場合はnullptr
	[[nodiscard]] static const PostEffectDefinition* FindByShaderKey(std::string_view shaderKey);

	/// @brief JSON用Effect種別名から定義を取得
	/// @param typeName 検索するEffect種別名
	/// @return 定義、見つからない場合はnullptr
	[[nodiscard]] static const PostEffectDefinition* FindByTypeName(std::string_view typeName);

	/// @brief 登録済みの全Effect定義を取得
	/// @return Effect定義の範囲
	[[nodiscard]] static std::span<const PostEffectDefinition> GetAll();
};

} // namespace MadoEngine::Render
