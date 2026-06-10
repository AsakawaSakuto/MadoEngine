#include "JsonSerializer.h"

namespace {

	/// @brief Json配列からfloatを取得する
	/// @param json 読み込み元のJson
	/// @param index 取得する配列番号
	/// @param defaultValue 取得できない場合の値
	/// @return 取得した値
	float GetArrayFloat(const nlohmann::json& json, std::size_t index, float defaultValue) {
		if (!json.is_array() || json.size() <= index || !json.at(index).is_number()) {
			return defaultValue;
		}

		return json.at(index).get<float>();
	}

	/// @brief Jsonオブジェクトからfloatを取得する
	/// @param json 読み込み元のJson
	/// @param key 取得するキー
	/// @param defaultValue 取得できない場合の値
	/// @return 取得した値
	float GetObjectFloat(const nlohmann::json& json, const char* key, float defaultValue) {
		if (!json.is_object() || !json.contains(key) || !json.at(key).is_number()) {
			return defaultValue;
		}

		return json.at(key).get<float>();
	}

}

namespace MadoEngine::Json {

	nlohmann::json JsonSerializer::ToJson(const Vector2& value) {
		return nlohmann::json::array({ value.x, value.y });
	}

	nlohmann::json JsonSerializer::ToJson(const Vector3& value) {
		return nlohmann::json::array({ value.x, value.y, value.z });
	}

	nlohmann::json JsonSerializer::ToJson(const Vector4& value) {
		return nlohmann::json::array({ value.x, value.y, value.z, value.w });
	}

	nlohmann::json JsonSerializer::ToJson(const Quaternion& value) {
		return nlohmann::json::array({ value.x, value.y, value.z, value.w });
	}

	nlohmann::json JsonSerializer::ToJson(const Transform2D& value) {
		return {
			{ "scale", ToJson(value.scale) },
			{ "rotate", value.rotate },
			{ "translate", ToJson(value.translate) }
		};
	}

	nlohmann::json JsonSerializer::ToJson(const Transform3D& value) {
		return {
			{ "scale", ToJson(value.scale) },
			{ "rotate", ToJson(value.rotate) },
			{ "translate", ToJson(value.translate) }
		};
	}

	nlohmann::json JsonSerializer::ToJson(const EulerTransform& value) {
		return {
			{ "scale", ToJson(value.scale) },
			{ "rotate", ToJson(value.rotate) },
			{ "translate", ToJson(value.translate) }
		};
	}

	nlohmann::json JsonSerializer::ToJson(const QuaternionTransform& value) {
		return {
			{ "scale", ToJson(value.scale) },
			{ "rotate", ToJson(value.rotate) },
			{ "translate", ToJson(value.translate) }
		};
	}

	Vector2 JsonSerializer::ToVector2(const nlohmann::json& json, const Vector2& defaultValue) {
		if (json.is_array()) {
			return {
				GetArrayFloat(json, 0, defaultValue.x),
				GetArrayFloat(json, 1, defaultValue.y)
			};
		}

		if (json.is_object()) {
			return {
				GetObjectFloat(json, "x", defaultValue.x),
				GetObjectFloat(json, "y", defaultValue.y)
			};
		}

		return defaultValue;
	}

	Vector3 JsonSerializer::ToVector3(const nlohmann::json& json, const Vector3& defaultValue) {
		if (json.is_array()) {
			return {
				GetArrayFloat(json, 0, defaultValue.x),
				GetArrayFloat(json, 1, defaultValue.y),
				GetArrayFloat(json, 2, defaultValue.z)
			};
		}

		if (json.is_object()) {
			return {
				GetObjectFloat(json, "x", defaultValue.x),
				GetObjectFloat(json, "y", defaultValue.y),
				GetObjectFloat(json, "z", defaultValue.z)
			};
		}

		return defaultValue;
	}

	Vector4 JsonSerializer::ToVector4(const nlohmann::json& json, const Vector4& defaultValue) {
		if (json.is_array()) {
			return {
				GetArrayFloat(json, 0, defaultValue.x),
				GetArrayFloat(json, 1, defaultValue.y),
				GetArrayFloat(json, 2, defaultValue.z),
				GetArrayFloat(json, 3, defaultValue.w)
			};
		}

		if (json.is_object()) {
			return {
				GetObjectFloat(json, "x", defaultValue.x),
				GetObjectFloat(json, "y", defaultValue.y),
				GetObjectFloat(json, "z", defaultValue.z),
				GetObjectFloat(json, "w", defaultValue.w)
			};
		}

		return defaultValue;
	}

	Quaternion JsonSerializer::ToQuaternion(const nlohmann::json& json, const Quaternion& defaultValue) {
		if (json.is_array()) {
			return {
				GetArrayFloat(json, 0, defaultValue.x),
				GetArrayFloat(json, 1, defaultValue.y),
				GetArrayFloat(json, 2, defaultValue.z),
				GetArrayFloat(json, 3, defaultValue.w)
			};
		}

		if (json.is_object()) {
			return {
				GetObjectFloat(json, "x", defaultValue.x),
				GetObjectFloat(json, "y", defaultValue.y),
				GetObjectFloat(json, "z", defaultValue.z),
				GetObjectFloat(json, "w", defaultValue.w)
			};
		}

		return defaultValue;
	}

	Transform2D JsonSerializer::ToTransform2D(const nlohmann::json& json, const Transform2D& defaultValue) {
		if (!json.is_object()) {
			return defaultValue;
		}

		Transform2D result = defaultValue;
		result.scale = json.contains("scale") ? ToVector2(json.at("scale"), defaultValue.scale) : defaultValue.scale;
		result.rotate = GetOrDefault<float>(json, "rotate", defaultValue.rotate);
		result.translate = json.contains("translate") ? ToVector2(json.at("translate"), defaultValue.translate) : defaultValue.translate;
		return result;
	}

	Transform3D JsonSerializer::ToTransform3D(const nlohmann::json& json, const Transform3D& defaultValue) {
		if (!json.is_object()) {
			return defaultValue;
		}

		Transform3D result = defaultValue;
		result.scale = json.contains("scale") ? ToVector3(json.at("scale"), defaultValue.scale) : defaultValue.scale;
		result.rotate = json.contains("rotate") ? ToVector3(json.at("rotate"), defaultValue.rotate) : defaultValue.rotate;
		result.translate = json.contains("translate") ? ToVector3(json.at("translate"), defaultValue.translate) : defaultValue.translate;
		return result;
	}

	EulerTransform JsonSerializer::ToEulerTransform(const nlohmann::json& json, const EulerTransform& defaultValue) {
		if (!json.is_object()) {
			return defaultValue;
		}

		EulerTransform result = defaultValue;
		result.scale = json.contains("scale") ? ToVector3(json.at("scale"), defaultValue.scale) : defaultValue.scale;
		result.rotate = json.contains("rotate") ? ToVector3(json.at("rotate"), defaultValue.rotate) : defaultValue.rotate;
		result.translate = json.contains("translate") ? ToVector3(json.at("translate"), defaultValue.translate) : defaultValue.translate;
		return result;
	}

	QuaternionTransform JsonSerializer::ToQuaternionTransform(const nlohmann::json& json, const QuaternionTransform& defaultValue) {
		if (!json.is_object()) {
			return defaultValue;
		}

		QuaternionTransform result = defaultValue;
		result.scale = json.contains("scale") ? ToVector3(json.at("scale"), defaultValue.scale) : defaultValue.scale;
		result.rotate = json.contains("rotate") ? ToQuaternion(json.at("rotate"), defaultValue.rotate) : defaultValue.rotate;
		result.translate = json.contains("translate") ? ToVector3(json.at("translate"), defaultValue.translate) : defaultValue.translate;
		return result;
	}

}

void to_json(nlohmann::json& json, const Vector2& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Vector2& value) {
	value = MadoEngine::Json::JsonSerializer::ToVector2(json, value);
}

void to_json(nlohmann::json& json, const Vector3& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Vector3& value) {
	value = MadoEngine::Json::JsonSerializer::ToVector3(json, value);
}

void to_json(nlohmann::json& json, const Vector4& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Vector4& value) {
	value = MadoEngine::Json::JsonSerializer::ToVector4(json, value);
}

void to_json(nlohmann::json& json, const Quaternion& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Quaternion& value) {
	value = MadoEngine::Json::JsonSerializer::ToQuaternion(json, value);
}

void to_json(nlohmann::json& json, const Transform2D& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Transform2D& value) {
	value = MadoEngine::Json::JsonSerializer::ToTransform2D(json, value);
}

void to_json(nlohmann::json& json, const Transform3D& value) {
	json = MadoEngine::Json::JsonSerializer::ToJson(value);
}

void from_json(const nlohmann::json& json, Transform3D& value) {
	value = MadoEngine::Json::JsonSerializer::ToTransform3D(json, value);
}
