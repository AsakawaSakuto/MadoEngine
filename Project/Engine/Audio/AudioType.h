#pragma once
#include <string>

/// @brief オーディオの種類を表す列挙型
enum  class AudioType {
	SE,
	BGM,
	Voice,

	Count
};

inline std::string AudioTypeToString(AudioType type) {
	switch (type) {
	case AudioType::SE:    return "SE";
	case AudioType::BGM:   return "BGM";
	case AudioType::Voice: return "Voice";
	default:               return "Unknown";
	}
}