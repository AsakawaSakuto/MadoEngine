#pragma once
#include "Math/Transform.h"
#include "Math/Vector4.h"
#include <nlohmann/json.hpp>
#include <string>

namespace MadoEngine::Json {

	/// @brief エンジン標準型とJsonの相互変換を担当するユーティリティ
	class JsonSerializer {
	public:
		/// @brief Vector2をJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Vector2& value);

		/// @brief Vector3をJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Vector3& value);

		/// @brief Vector4をJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Vector4& value);

		/// @brief QuaternionをJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Quaternion& value);

		/// @brief Transform2DをJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Transform2D& value);

		/// @brief Transform3DをJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const Transform3D& value);

		/// @brief EulerTransformをJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const EulerTransform& value);

		/// @brief QuaternionTransformをJsonへ変換する
		/// @param value 変換する値
		/// @return 変換されたJson
		static nlohmann::json ToJson(const QuaternionTransform& value);

		/// @brief JsonからVector2を読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Vector2 ToVector2(const nlohmann::json& json, const Vector2& defaultValue = {});

		/// @brief JsonからVector3を読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Vector3 ToVector3(const nlohmann::json& json, const Vector3& defaultValue = {});

		/// @brief JsonからVector4を読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Vector4 ToVector4(const nlohmann::json& json, const Vector4& defaultValue = {});

		/// @brief JsonからQuaternionを読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Quaternion ToQuaternion(const nlohmann::json& json, const Quaternion& defaultValue = { 0.0f, 0.0f, 0.0f, 1.0f });

		/// @brief JsonからTransform2Dを読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Transform2D ToTransform2D(const nlohmann::json& json, const Transform2D& defaultValue = {});

		/// @brief JsonからTransform3Dを読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static Transform3D ToTransform3D(const nlohmann::json& json, const Transform3D& defaultValue = {});

		/// @brief JsonからEulerTransformを読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static EulerTransform ToEulerTransform(const nlohmann::json& json, const EulerTransform& defaultValue = {});

		/// @brief JsonからQuaternionTransformを読み込む
		/// @param json 読み込み元のJson
		/// @param defaultValue 読み込みに失敗した場合の値
		/// @return 読み込んだ値
		static QuaternionTransform ToQuaternionTransform(const nlohmann::json& json, const QuaternionTransform& defaultValue = {});

		/// @brief Jsonから指定キーの値を取得する
		/// @param json 読み込み元のJson
		/// @param key 取得するキー
		/// @param defaultValue キーが存在しない場合の値
		/// @return 取得した値
		template <class T>
		static T GetOrDefault(const nlohmann::json& json, const std::string& key, const T& defaultValue) {
			if (!json.is_object() || !json.contains(key) || json.at(key).is_null()) {
				return defaultValue;
			}

			try {
				return json.at(key).get<T>();
			}
			catch (const nlohmann::json::exception&) {
				return defaultValue;
			}
		}
	};

}

/// @brief Vector2をnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Vector2& value);

/// @brief nlohmann::jsonからVector2へ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Vector2& value);

/// @brief Vector3をnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Vector3& value);

/// @brief nlohmann::jsonからVector3へ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Vector3& value);

/// @brief Vector4をnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Vector4& value);

/// @brief nlohmann::jsonからVector4へ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Vector4& value);

/// @brief Quaternionをnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Quaternion& value);

/// @brief nlohmann::jsonからQuaternionへ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Quaternion& value);

/// @brief Transform2Dをnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Transform2D& value);

/// @brief nlohmann::jsonからTransform2Dへ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Transform2D& value);

/// @brief Transform3Dをnlohmann::jsonへ変換する
/// @param json 変換先のJson
/// @param value 変換する値
void to_json(nlohmann::json& json, const Transform3D& value);

/// @brief nlohmann::jsonからTransform3Dへ変換する
/// @param json 読み込み元のJson
/// @param value 変換先の値
void from_json(const nlohmann::json& json, Transform3D& value);
