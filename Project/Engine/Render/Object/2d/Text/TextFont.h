#pragma once

/// @brief Textで選択可能なフォント種別です。
enum class TextFontFamilyType {
	YuGothicUI,
	Meiryo,
	MSGothic,
	MSMincho,
	SegoeUI,
	Arial,
	Consolas,
	Count,
};

struct TextFontDefinition {
	TextFontFamilyType type;
	const char* displayName;
	const char* familyName;
};

const TextFontDefinition kTextFontDefinitions[] = {
	{ TextFontFamilyType::YuGothicUI, "Yu Gothic UI", "Yu Gothic UI" },
	{ TextFontFamilyType::Meiryo, "Meiryo", "Meiryo" },
	{ TextFontFamilyType::MSGothic, "MS Gothic", "MS Gothic" },
	{ TextFontFamilyType::MSMincho, "MS Mincho", "MS Mincho" },
	{ TextFontFamilyType::SegoeUI, "Segoe UI", "Segoe UI" },
	{ TextFontFamilyType::Arial, "Arial", "Arial" },
	{ TextFontFamilyType::Consolas, "Consolas", "Consolas" },
};