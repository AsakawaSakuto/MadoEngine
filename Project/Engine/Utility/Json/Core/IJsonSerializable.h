#pragma once
#include <nlohmann/json.hpp>

namespace MadoEngine::Json {

	/// @brief Jsonへ変換できるクラスの共通インターフェース
	class IJsonSerializable {
	public:
		virtual ~IJsonSerializable() = default;

		/// @brief Jsonから値を読み込む
		/// @param json 読み込み元のJson
		virtual void FromJson(const nlohmann::json& json) = 0;

		/// @brief 現在の値をJsonへ変換する
		/// @return 変換されたJson
		virtual nlohmann::json ToJson() const = 0;
	};
}
