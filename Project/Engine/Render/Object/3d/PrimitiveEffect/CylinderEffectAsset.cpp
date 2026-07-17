#include "CylinderEffectAsset.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace {

	using JsonValue = nlohmann::json;
	using namespace MadoEngine::Effect;

	struct EaseTypeName {
		std::string_view name;
		EaseType type;
	};

	constexpr EaseTypeName kEaseTypeNames[] = {
		{ "Linear", EaseType::Linear },
		{ "EaseInQuad", EaseType::EaseInQuad },
		{ "EaseOutQuad", EaseType::EaseOutQuad },
		{ "EaseInOutQuad", EaseType::EaseInOutQuad },
		{ "EaseOutInQuad", EaseType::EaseOutInQuad },
		{ "EaseInCubic", EaseType::EaseInCubic },
		{ "EaseOutCubic", EaseType::EaseOutCubic },
		{ "EaseInOutCubic", EaseType::EaseInOutCubic },
		{ "EaseOutInCubic", EaseType::EaseOutInCubic },
		{ "EaseInQuart", EaseType::EaseInQuart },
		{ "EaseOutQuart", EaseType::EaseOutQuart },
		{ "EaseInOutQuart", EaseType::EaseInOutQuart },
		{ "EaseOutInQuart", EaseType::EaseOutInQuart },
		{ "EaseInQuint", EaseType::EaseInQuint },
		{ "EaseOutQuint", EaseType::EaseOutQuint },
		{ "EaseInOutQuint", EaseType::EaseInOutQuint },
		{ "EaseOutInQuint", EaseType::EaseOutInQuint },
		{ "EaseInSine", EaseType::EaseInSine },
		{ "EaseOutSine", EaseType::EaseOutSine },
		{ "EaseInOutSine", EaseType::EaseInOutSine },
		{ "EaseOutInSine", EaseType::EaseOutInSine },
		{ "EaseInExpo", EaseType::EaseInExpo },
		{ "EaseOutExpo", EaseType::EaseOutExpo },
		{ "EaseInOutExpo", EaseType::EaseInOutExpo },
		{ "EaseOutInExpo", EaseType::EaseOutInExpo },
		{ "EaseInCirc", EaseType::EaseInCirc },
		{ "EaseOutCirc", EaseType::EaseOutCirc },
		{ "EaseInOutCirc", EaseType::EaseInOutCirc },
		{ "EaseOutInCirc", EaseType::EaseOutInCirc },
		{ "EaseInBack", EaseType::EaseInBack },
		{ "EaseOutBack", EaseType::EaseOutBack },
		{ "EaseInOutBack", EaseType::EaseInOutBack },
		{ "EaseOutInBack", EaseType::EaseOutInBack },
		{ "EaseInElastic", EaseType::EaseInElastic },
		{ "EaseOutElastic", EaseType::EaseOutElastic },
		{ "EaseInOutElastic", EaseType::EaseInOutElastic },
		{ "EaseOutInElastic", EaseType::EaseOutInElastic },
		{ "EaseInBounce", EaseType::EaseInBounce },
		{ "EaseOutBounce", EaseType::EaseOutBounce },
		{ "EaseInOutBounce", EaseType::EaseInOutBounce },
		{ "EaseOutInBounce", EaseType::EaseOutInBounce },
		{ "None", EaseType::None },
	};

	/// @brief ファイルパスをUTF-8文字列へ変換する
	/// @param path 変換するファイルパス
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(value.data()), value.size());
	}

	/// @brief Jsonオブジェクトから子要素を取得する
	/// @param json 検索元Json
	/// @param key 検索するキー
	/// @return 子要素。存在しない場合はnullptr
	const JsonValue* FindValue(const JsonValue& json, const char* key) {
		if (!json.is_object() || !json.contains(key) || json.at(key).is_null()) {
			return nullptr;
		}
		return &json.at(key);
	}

	/// @brief Jsonから文字列を安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	std::string ReadString(const JsonValue& json, const char* key, const std::string& fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_string() ? value->get<std::string>() : fallback;
	}

	/// @brief Jsonからfloatを安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	float ReadFloat(const JsonValue& json, const char* key, float fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_number() ? value->get<float>() : fallback;
	}

	/// @brief Jsonから符号なし整数を安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	uint32_t ReadUInt(const JsonValue& json, const char* key, uint32_t fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_number_unsigned() ? value->get<uint32_t>() : fallback;
	}

	/// @brief Jsonからboolを安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	bool ReadBool(const JsonValue& json, const char* key, bool fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_boolean() ? value->get<bool>() : fallback;
	}

	/// @brief 文字列からイージング種類を取得する
	/// @param value 変換元文字列
	/// @return イージング種類
	EaseType ParseEaseType(const std::string& value) {
		for (const EaseTypeName& entry : kEaseTypeNames) {
			if (entry.name == value) {
				return entry.type;
			}
		}
		return EaseType::Linear;
	}

	/// @brief イージング種類を文字列へ変換する
	/// @param value 変換するイージング種類
	/// @return イージング名
	const char* ToString(EaseType value) {
		for (const EaseTypeName& entry : kEaseTypeNames) {
			if (entry.type == value) {
				return entry.name.data();
			}
		}
		return "Linear";
	}

	/// @brief Jsonから型付きトラックを読み込む
	/// @tparam T トラック値の型
	/// @tparam Reader 値の読み込み関数型
	/// @param json 読み込み元Json
	/// @param fallback 読み込み失敗時の値
	/// @param reader 値の読み込み関数
	/// @return 読み込んだトラック
	template<class T, class Reader>
	EffectTrack<T> ReadTrack(const JsonValue& json, const T& fallback, Reader reader) {
		EffectTrack<T> track(fallback);
		if (!json.is_object() || (!json.contains("default") && !json.contains("keys"))) {
			track.SetDefaultValue(reader(json, fallback));
			return track;
		}

		if (const JsonValue* defaultValue = FindValue(json, "default")) {
			track.SetDefaultValue(reader(*defaultValue, fallback));
		}

		std::vector<EffectKeyframe<T>> keyframes;
		const JsonValue* keys = FindValue(json, "keys");
		if (keys && keys->is_array()) {
			for (const JsonValue& keyJson : *keys) {
				if (!keyJson.is_object()) {
					continue;
				}

				EffectKeyframe<T> keyframe;
				keyframe.time = ReadFloat(keyJson, "time", 0.0f);
				if (const JsonValue* value = FindValue(keyJson, "value")) {
					keyframe.value = reader(*value, track.GetDefaultValue());
				} else {
					keyframe.value = track.GetDefaultValue();
				}
				keyframe.easing = ParseEaseType(ReadString(keyJson, "easing", "Linear"));
				keyframes.push_back(keyframe);
			}
		}
		track.SetKeyframes(std::move(keyframes));
		return track;
	}

	/// @brief 型付きトラックをJsonへ変換する
	/// @tparam T トラック値の型
	/// @param track 変換するトラック
	/// @return 変換されたJson
	template<class T>
	JsonValue WriteTrack(const EffectTrack<T>& track) {
		JsonValue json;
		json["default"] = track.GetDefaultValue();
		json["keys"] = JsonValue::array();
		for (const EffectKeyframe<T>& keyframe : track.GetKeyframes()) {
			json["keys"].push_back({
				{ "time", keyframe.time },
				{ "value", keyframe.value },
				{ "easing", ToString(keyframe.easing) },
			});
		}
		return json;
	}

	/// @brief 文字列からUV方向を取得する
	/// @param value 変換元文字列
	/// @return UV方向
	CylinderUvDirection ParseUvDirection(const std::string& value) {
		if (value == "bottomToTop") { return CylinderUvDirection::BottomToTop; }
		if (value == "clockwise") { return CylinderUvDirection::Clockwise; }
		if (value == "counterClockwise") { return CylinderUvDirection::CounterClockwise; }
		return CylinderUvDirection::TopToBottom;
	}

	/// @brief UV方向を文字列へ変換する
	/// @param value 変換するUV方向
	/// @return UV方向名
	const char* ToString(CylinderUvDirection value) {
		switch (value) {
		case CylinderUvDirection::BottomToTop: return "bottomToTop";
		case CylinderUvDirection::Clockwise: return "clockwise";
		case CylinderUvDirection::CounterClockwise: return "counterClockwise";
		case CylinderUvDirection::TopToBottom:
		default: return "topToBottom";
		}
	}

	/// @brief 文字列からPivotを取得する
	/// @param value 変換元文字列
	/// @return Pivot
	CylinderPivot ParsePivot(const std::string& value) {
		if (value == "center") { return CylinderPivot::Center; }
		if (value == "top") { return CylinderPivot::Top; }
		return CylinderPivot::Bottom;
	}

	/// @brief Pivotを文字列へ変換する
	/// @param value 変換するPivot
	/// @return Pivot名
	const char* ToString(CylinderPivot value) {
		switch (value) {
		case CylinderPivot::Center: return "center";
		case CylinderPivot::Top: return "top";
		case CylinderPivot::Bottom:
		default: return "bottom";
		}
	}

	/// @brief 文字列からBlendModeを取得する
	/// @param value 変換元文字列
	/// @return BlendMode
	MadoEngine::Render::BlendMode ParseBlendMode(const std::string& value) {
		if (value == "normal") { return MadoEngine::Render::BlendMode::Normal; }
		if (value == "subtract") { return MadoEngine::Render::BlendMode::Subtract; }
		if (value == "multiply") { return MadoEngine::Render::BlendMode::Multiply; }
		if (value == "none") { return MadoEngine::Render::BlendMode::None; }
		return MadoEngine::Render::BlendMode::Add;
	}

	/// @brief BlendModeを文字列へ変換する
	/// @param value 変換するBlendMode
	/// @return BlendMode名
	const char* ToString(MadoEngine::Render::BlendMode value) {
		switch (value) {
		case MadoEngine::Render::BlendMode::Normal: return "normal";
		case MadoEngine::Render::BlendMode::Subtract: return "subtract";
		case MadoEngine::Render::BlendMode::Multiply: return "multiply";
		case MadoEngine::Render::BlendMode::None: return "none";
		case MadoEngine::Render::BlendMode::Add:
		default: return "add";
		}
	}

	/// @brief 文字列からCullModeを取得する
	/// @param value 変換元文字列
	/// @return CullMode
	MadoEngine::Render::CullMode ParseCullMode(const std::string& value) {
		if (value == "front") { return MadoEngine::Render::CullMode::Front; }
		if (value == "back") { return MadoEngine::Render::CullMode::Back; }
		return MadoEngine::Render::CullMode::None;
	}

	/// @brief CullModeを文字列へ変換する
	/// @param value 変換するCullMode
	/// @return CullMode名
	const char* ToString(MadoEngine::Render::CullMode value) {
		switch (value) {
		case MadoEngine::Render::CullMode::Front: return "front";
		case MadoEngine::Render::CullMode::Back: return "back";
		case MadoEngine::Render::CullMode::None:
		default: return "none";
		}
	}

	/// @brief トラック内の全値を補正する
	/// @tparam T トラック値の型
	/// @tparam Normalizer 値の補正関数型
	/// @param track 補正対象トラック
	/// @param normalizer 値の補正関数
	template<class T, class Normalizer>
	void NormalizeTrack(EffectTrack<T>& track, Normalizer normalizer) {
		track.SetDefaultValue(normalizer(track.GetDefaultValue()));
		std::vector<EffectKeyframe<T>> keyframes = track.GetKeyframes();
		for (EffectKeyframe<T>& keyframe : keyframes) {
			keyframe.value = normalizer(keyframe.value);
		}
		track.SetKeyframes(std::move(keyframes));
	}

} // namespace

namespace MadoEngine::Effect {

	bool CylinderEffectAsset::LoadFromFile(const std::filesystem::path& filePath) {
		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}

		filePath_ = filePath;
		name_ = PathToUtf8String(filePath.stem());
		FromJson(json);
		return true;
	}

	bool CylinderEffectAsset::SaveToFile(const std::filesystem::path& filePath, bool createBackup) const {
		const std::filesystem::path outputPath = filePath.empty() ? filePath_ : filePath;
		if (outputPath.empty()) {
			Logger::Output("Cylinder Effect Assetの保存先が指定されていません。", Logger::Level::Error);
			return false;
		}
		return MadoEngine::Json::JsonFile::Save(outputPath, ToJson(), 4, createBackup);
	}

	void CylinderEffectAsset::FromJson(const nlohmann::json& json) {
		version_ = ReadUInt(json, "version", kCurrentVersion);
		if (version_ > kCurrentVersion) {
			Logger::Output("未対応のCylinder Effect Assetバージョンです: " + std::to_string(version_), Logger::Level::Warning);
		}

		config_.duration = ReadFloat(json, "duration", config_.duration);
		config_.isLoop = ReadBool(json, "loop", config_.isLoop);

		const auto readFloatValue = [](const JsonValue& value, float fallback) {
			return value.is_number() ? value.get<float>() : fallback;
		};
		const auto readVector2Value = [](const JsonValue& value, const Vector2& fallback) {
			return MadoEngine::Json::JsonSerializer::ToVector2(value, fallback);
		};
		const auto readVector4Value = [](const JsonValue& value, const Vector4& fallback) {
			return MadoEngine::Json::JsonSerializer::ToVector4(value, fallback);
		};

		if (const JsonValue* geometry = FindValue(json, "geometry")) {
			config_.geometry.radialSegments = ReadUInt(*geometry, "radialSegments", config_.geometry.radialSegments);
			config_.geometry.heightSegments = ReadUInt(*geometry, "heightSegments", config_.geometry.heightSegments);
			config_.geometry.pivot = ParsePivot(ReadString(*geometry, "pivot", "bottom"));
			if (const JsonValue* value = FindValue(*geometry, "bottomRadii")) {
				config_.geometry.bottomRadii = ReadTrack(*value, config_.geometry.bottomRadii.GetDefaultValue(), readVector2Value);
			}
			if (const JsonValue* value = FindValue(*geometry, "topRadii")) {
				config_.geometry.topRadii = ReadTrack(*value, config_.geometry.topRadii.GetDefaultValue(), readVector2Value);
			}
			if (const JsonValue* value = FindValue(*geometry, "height")) {
				config_.geometry.height = ReadTrack(*value, config_.geometry.height.GetDefaultValue(), readFloatValue);
			}
			if (const JsonValue* value = FindValue(*geometry, "startAngleDegrees")) {
				config_.geometry.startAngleDegrees = ReadTrack(*value, config_.geometry.startAngleDegrees.GetDefaultValue(), readFloatValue);
			}
			if (const JsonValue* value = FindValue(*geometry, "arcAngleDegrees")) {
				config_.geometry.arcAngleDegrees = ReadTrack(*value, config_.geometry.arcAngleDegrees.GetDefaultValue(), readFloatValue);
			}
		}

		if (const JsonValue* material = FindValue(json, "material")) {
			config_.material.textureName = ReadString(*material, "textureName", config_.material.textureName);
			config_.material.blendMode = ParseBlendMode(ReadString(*material, "blendMode", "add"));
			config_.material.cullMode = ParseCullMode(ReadString(*material, "cullMode", "none"));
			if (const JsonValue* value = FindValue(*material, "globalAlpha")) {
				config_.material.globalAlpha = ReadTrack(*value, config_.material.globalAlpha.GetDefaultValue(), readFloatValue);
			}
			if (const JsonValue* value = FindValue(*material, "bottomFadeRange")) {
				config_.material.bottomFadeRange = ReadTrack(*value, config_.material.bottomFadeRange.GetDefaultValue(), readFloatValue);
			}
			if (const JsonValue* value = FindValue(*material, "topFadeRange")) {
				config_.material.topFadeRange = ReadTrack(*value, config_.material.topFadeRange.GetDefaultValue(), readFloatValue);
			}

			if (const JsonValue* uv = FindValue(*material, "uv")) {
				config_.material.uv.direction = ParseUvDirection(ReadString(*uv, "direction", "topToBottom"));
				if (const JsonValue* value = FindValue(*uv, "scale")) {
					config_.material.uv.scale = ReadTrack(*value, config_.material.uv.scale.GetDefaultValue(), readVector2Value);
				}
				if (const JsonValue* value = FindValue(*uv, "offset")) {
					config_.material.uv.offset = ReadTrack(*value, config_.material.uv.offset.GetDefaultValue(), readVector2Value);
				}
				if (const JsonValue* value = FindValue(*uv, "rotationDegrees")) {
					config_.material.uv.rotationDegrees = ReadTrack(*value, config_.material.uv.rotationDegrees.GetDefaultValue(), readFloatValue);
				}
			}

			config_.material.gradient.clear();
			if (const JsonValue* gradient = FindValue(*material, "gradient"); gradient && gradient->is_array()) {
				for (const JsonValue& stopJson : *gradient) {
					if (!stopJson.is_object()) {
						continue;
					}
					CylinderColorStop stop;
					stop.position = ReadFloat(stopJson, "position", stop.position);
					if (const JsonValue* value = FindValue(stopJson, "color")) {
						stop.color = ReadTrack(*value, stop.color.GetDefaultValue(), readVector4Value);
					}
					config_.material.gradient.push_back(std::move(stop));
				}
			}
		}

		Validate();
	}

	nlohmann::json CylinderEffectAsset::ToJson() const {
		JsonValue gradient = JsonValue::array();
		for (const CylinderColorStop& stop : config_.material.gradient) {
			gradient.push_back({
				{ "position", stop.position },
				{ "color", WriteTrack(stop.color) },
			});
		}

		return JsonValue{
			{ "version", kCurrentVersion },
			{ "duration", config_.duration },
			{ "loop", config_.isLoop },
			{ "geometry", {
				{ "radialSegments", config_.geometry.radialSegments },
				{ "heightSegments", config_.geometry.heightSegments },
				{ "pivot", ToString(config_.geometry.pivot) },
				{ "bottomRadii", WriteTrack(config_.geometry.bottomRadii) },
				{ "topRadii", WriteTrack(config_.geometry.topRadii) },
				{ "height", WriteTrack(config_.geometry.height) },
				{ "startAngleDegrees", WriteTrack(config_.geometry.startAngleDegrees) },
				{ "arcAngleDegrees", WriteTrack(config_.geometry.arcAngleDegrees) },
			} },
			{ "material", {
				{ "textureName", config_.material.textureName },
				{ "blendMode", ToString(config_.material.blendMode) },
				{ "cullMode", ToString(config_.material.cullMode) },
				{ "globalAlpha", WriteTrack(config_.material.globalAlpha) },
				{ "bottomFadeRange", WriteTrack(config_.material.bottomFadeRange) },
				{ "topFadeRange", WriteTrack(config_.material.topFadeRange) },
				{ "uv", {
					{ "direction", ToString(config_.material.uv.direction) },
					{ "scale", WriteTrack(config_.material.uv.scale) },
					{ "offset", WriteTrack(config_.material.uv.offset) },
					{ "rotationDegrees", WriteTrack(config_.material.uv.rotationDegrees) },
				} },
				{ "gradient", gradient },
			} },
		};
	}

	void CylinderEffectAsset::Validate() {
		config_.duration = std::clamp(std::isfinite(config_.duration) ? config_.duration : 1.0f, 0.001f, 3600.0f);
		config_.geometry.radialSegments = std::clamp(config_.geometry.radialSegments, 3u, 256u);
		config_.geometry.heightSegments = std::clamp(config_.geometry.heightSegments, 1u, 64u);

		const auto normalizeRadii = [](Vector2 value) {
			value.x = std::isfinite(value.x) ? (std::max)(0.0f, value.x) : 1.0f;
			value.y = std::isfinite(value.y) ? (std::max)(0.0f, value.y) : 1.0f;
			return value;
		};
		const auto normalizeHeight = [](float value) {
			return std::clamp(std::isfinite(value) ? value : 1.0f, 0.001f, 10000.0f);
		};
		const auto normalizeAngle = [](float value) {
			return std::isfinite(value) ? value : 0.0f;
		};
		const auto normalizeArc = [](float value) {
			return std::clamp(std::isfinite(value) ? value : 360.0f, -360.0f, 360.0f);
		};
		const auto normalizeUnit = [](float value) {
			return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
		};
		const auto normalizeVector2 = [](Vector2 value) {
			value.x = std::isfinite(value.x) ? value.x : 0.0f;
			value.y = std::isfinite(value.y) ? value.y : 0.0f;
			return value;
		};
		const auto normalizeColor = [](Vector4 value) {
			value.x = std::isfinite(value.x) ? (std::max)(0.0f, value.x) : 1.0f;
			value.y = std::isfinite(value.y) ? (std::max)(0.0f, value.y) : 1.0f;
			value.z = std::isfinite(value.z) ? (std::max)(0.0f, value.z) : 1.0f;
			value.w = std::clamp(std::isfinite(value.w) ? value.w : 1.0f, 0.0f, 1.0f);
			return value;
		};

		NormalizeTrack(config_.geometry.bottomRadii, normalizeRadii);
		NormalizeTrack(config_.geometry.topRadii, normalizeRadii);
		NormalizeTrack(config_.geometry.height, normalizeHeight);
		NormalizeTrack(config_.geometry.startAngleDegrees, normalizeAngle);
		NormalizeTrack(config_.geometry.arcAngleDegrees, normalizeArc);
		NormalizeTrack(config_.material.globalAlpha, normalizeUnit);
		NormalizeTrack(config_.material.bottomFadeRange, normalizeUnit);
		NormalizeTrack(config_.material.topFadeRange, normalizeUnit);
		NormalizeTrack(config_.material.uv.scale, normalizeVector2);
		NormalizeTrack(config_.material.uv.offset, normalizeVector2);
		NormalizeTrack(config_.material.uv.rotationDegrees, normalizeAngle);

		if (config_.material.textureName.empty()) {
			config_.material.textureName = "white2x2";
		}
		if (config_.material.gradient.empty()) {
			CylinderColorStop bottom;
			bottom.position = 0.0f;
			CylinderColorStop top;
			top.position = 1.0f;
			config_.material.gradient = { bottom, top };
		}
		if (config_.material.gradient.size() > kMaximumCylinderGradientStops) {
			config_.material.gradient.resize(kMaximumCylinderGradientStops);
		}
		for (CylinderColorStop& stop : config_.material.gradient) {
			stop.position = normalizeUnit(stop.position);
			NormalizeTrack(stop.color, normalizeColor);
		}
		std::stable_sort(
			config_.material.gradient.begin(),
			config_.material.gradient.end(),
			[](const CylinderColorStop& lhs, const CylinderColorStop& rhs) {
				return lhs.position < rhs.position;
			}
		);
	}

} // namespace MadoEngine::Effect
