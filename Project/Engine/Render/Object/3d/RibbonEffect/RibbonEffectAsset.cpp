#include "RibbonEffectAsset.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace {

	using JsonValue = nlohmann::json;
	using namespace MadoEngine::Ribbon;

	/// @brief Filesystem PathをUTF-8文字列へ変換する
	/// @param path 変換対象Path
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(value.data()), value.size());
	}

	/// @brief Json Objectから子要素を安全に取得する
	/// @param json 検索対象Json
	/// @param key 検索Key
	/// @return 子要素。存在しない場合はnullptr
	const JsonValue* FindValue(const JsonValue& json, const char* key) {
		if (!json.is_object() || !json.contains(key) || json.at(key).is_null()) {
			return nullptr;
		}
		return &json.at(key);
	}

	/// @brief Jsonからfloatを安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	float ReadFloat(const JsonValue& json, const char* key, float fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_number() ? value->get<float>() : fallback;
	}

	/// @brief Jsonから符号なし整数を安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	uint32_t ReadUInt(const JsonValue& json, const char* key, uint32_t fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_number_unsigned() ? value->get<uint32_t>() : fallback;
	}

	/// @brief Jsonからboolを安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	bool ReadBool(const JsonValue& json, const char* key, bool fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_boolean() ? value->get<bool>() : fallback;
	}

	/// @brief Jsonから文字列を安全に読み込む
	/// @param json 読み込み元Json
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	std::string ReadString(const JsonValue& json, const char* key, const std::string& fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_string() ? value->get<std::string>() : fallback;
	}

	/// @brief Json配列からVector2を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだVector2
	Vector2 ReadVector2(const JsonValue& json, const Vector2& fallback) {
		if (!json.is_array() || json.size() < 2 || !json[0].is_number() || !json[1].is_number()) {
			return fallback;
		}
		return { json[0].get<float>(), json[1].get<float>() };
	}

	/// @brief Json配列からVector3を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだVector3
	Vector3 ReadVector3(const JsonValue& json, const Vector3& fallback) {
		if (!json.is_array() || json.size() < 3) {
			return fallback;
		}
		for (std::size_t index = 0; index < 3; ++index) {
			if (!json[index].is_number()) {
				return fallback;
			}
		}
		return {
			json[0].get<float>(),
			json[1].get<float>(),
			json[2].get<float>(),
		};
	}

	/// @brief Json配列からVector4を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだVector4
	Vector4 ReadVector4(const JsonValue& json, const Vector4& fallback) {
		if (!json.is_array() || json.size() < 4) {
			return fallback;
		}
		for (std::size_t index = 0; index < 4; ++index) {
			if (!json[index].is_number()) {
				return fallback;
			}
		}
		return {
			json[0].get<float>(),
			json[1].get<float>(),
			json[2].get<float>(),
			json[3].get<float>(),
		};
	}

	/// @brief Vector2をJson配列へ変換する
	/// @param value 変換対象
	/// @return 変換後Json
	JsonValue WriteVector2(const Vector2& value) {
		return JsonValue::array({ value.x, value.y });
	}

	/// @brief Vector3をJson配列へ変換する
	/// @param value 変換対象
	/// @return 変換後Json
	JsonValue WriteVector3(const Vector3& value) {
		return JsonValue::array({ value.x, value.y, value.z });
	}

	/// @brief Manual Ribbonの既定制御点をJson配列へ変換する
	/// @param controlPoints 変換対象制御点
	/// @return 変換後Json配列
	JsonValue WriteControlPoints(const std::vector<Vector3>& controlPoints) {
		JsonValue json = JsonValue::array();
		for (const Vector3& point : controlPoints) {
			json.push_back(WriteVector3(point));
		}
		return json;
	}

	/// @brief Vector4をJson配列へ変換する
	/// @param value 変換対象
	/// @return 変換後Json
	JsonValue WriteVector4(const Vector4& value) {
		return JsonValue::array({ value.x, value.y, value.z, value.w });
	}

	/// @brief JsonからEffectTrackを読み込む
	/// @tparam T Track値型
	/// @tparam Reader 値読み込み関数型
	/// @param json 読み込み元Json
	/// @param fallback 読み込み失敗時の値
	/// @param reader 値読み込み関数
	/// @return 読み込んだTrack
	template<class T, class Reader>
	MadoEngine::Effect::EffectTrack<T> ReadTrack(
		const JsonValue& json,
		const T& fallback,
		Reader reader) {
		MadoEngine::Effect::EffectTrack<T> track(fallback);
		if (!json.is_object()) {
			return track;
		}
		if (const JsonValue* defaultValue = FindValue(json, "default")) {
			track.SetDefaultValue(reader(*defaultValue, fallback));
		}

		std::vector<MadoEngine::Effect::EffectKeyframe<T>> keyframes;
		if (const JsonValue* keys = FindValue(json, "keys"); keys && keys->is_array()) {
			for (const JsonValue& keyJson : *keys) {
				if (!keyJson.is_object()) {
					continue;
				}
				MadoEngine::Effect::EffectKeyframe<T> keyframe;
				keyframe.time = ReadFloat(keyJson, "time", 0.0f);
				if (const JsonValue* value = FindValue(keyJson, "value")) {
					keyframe.value = reader(*value, track.GetDefaultValue());
				} else {
					keyframe.value = track.GetDefaultValue();
				}
				const uint32_t easing = ReadUInt(keyJson, "easing", 0);
				keyframe.easing = static_cast<EaseType>((std::min)(easing, static_cast<uint32_t>(EaseType::None)));
				keyframes.push_back(keyframe);
			}
		}
		track.SetKeyframes(std::move(keyframes));
		return track;
	}

	/// @brief EffectTrackをJsonへ変換する
	/// @tparam T Track値型
	/// @tparam Writer 値変換関数型
	/// @param track 変換対象Track
	/// @param writer 値変換関数
	/// @return 変換後Json
	template<class T, class Writer>
	JsonValue WriteTrack(const MadoEngine::Effect::EffectTrack<T>& track, Writer writer) {
		JsonValue json;
		json["default"] = writer(track.GetDefaultValue());
		json["keys"] = JsonValue::array();
		for (const MadoEngine::Effect::EffectKeyframe<T>& keyframe : track.GetKeyframes()) {
			json["keys"].push_back({
				{ "time", keyframe.time },
				{ "value", writer(keyframe.value) },
				{ "easing", static_cast<uint32_t>(keyframe.easing) },
			});
		}
		return json;
	}

	/// @brief Trackの全値を安全な値へ補正する
	/// @tparam T Track値型
	/// @tparam Normalizer 補正関数型
	/// @param track 補正対象Track
	/// @param maximumTime Keyframe時刻上限
	/// @param normalizer 値補正関数
	template<class T, class Normalizer>
	void NormalizeTrack(
		MadoEngine::Effect::EffectTrack<T>& track,
		float maximumTime,
		Normalizer normalizer) {
		track.SetDefaultValue(normalizer(track.GetDefaultValue()));
		std::vector<MadoEngine::Effect::EffectKeyframe<T>> keyframes = track.GetKeyframes();
		for (MadoEngine::Effect::EffectKeyframe<T>& keyframe : keyframes) {
			keyframe.time = std::clamp(
				std::isfinite(keyframe.time) ? keyframe.time : 0.0f,
				0.0f,
				maximumTime
			);
			keyframe.value = normalizer(keyframe.value);
		}
		track.SetKeyframes(std::move(keyframes));
	}

	/// @brief generationMode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	RibbonPointGenerationMode ParseGenerationMode(const std::string& value) {
		return value == "manual"
			? RibbonPointGenerationMode::Manual
			: RibbonPointGenerationMode::TransformHistory;
	}

	/// @brief generationModeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(RibbonPointGenerationMode value) {
		return value == RibbonPointGenerationMode::Manual ? "manual" : "transformHistory";
	}

	/// @brief playbackMode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	RibbonPlaybackMode ParsePlaybackMode(const std::string& value) {
		if (value == "reveal") { return RibbonPlaybackMode::Reveal; }
		if (value == "sweep") { return RibbonPlaybackMode::Sweep; }
		return RibbonPlaybackMode::Full;
	}

	/// @brief playbackModeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(RibbonPlaybackMode value) {
		switch (value) {
		case RibbonPlaybackMode::Reveal: return "reveal";
		case RibbonPlaybackMode::Sweep: return "sweep";
		case RibbonPlaybackMode::Full:
		default: return "full";
		}
	}

	/// @brief simulationSpace文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	RibbonSimulationSpace ParseSimulationSpace(const std::string& value) {
		return value == "local" ? RibbonSimulationSpace::Local : RibbonSimulationSpace::World;
	}

	/// @brief simulationSpaceを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(RibbonSimulationSpace value) {
		return value == RibbonSimulationSpace::Local ? "local" : "world";
	}

	/// @brief interpolation文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	RibbonInterpolationMode ParseInterpolation(const std::string& value) {
		return value == "catmullRom" ? RibbonInterpolationMode::CatmullRom : RibbonInterpolationMode::Linear;
	}

	/// @brief interpolationを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(RibbonInterpolationMode value) {
		return value == RibbonInterpolationMode::CatmullRom ? "catmullRom" : "linear";
	}

	/// @brief UV Mode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	RibbonUvMode ParseUvMode(const std::string& value) {
		return value == "tile" ? RibbonUvMode::Tile : RibbonUvMode::Stretch;
	}

	/// @brief UV Modeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(RibbonUvMode value) {
		return value == RibbonUvMode::Tile ? "tile" : "stretch";
	}

	/// @brief Blend Mode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	MadoEngine::Render::BlendMode ParseBlendMode(const std::string& value) {
		if (value == "normal") { return MadoEngine::Render::BlendMode::Normal; }
		if (value == "subtract") { return MadoEngine::Render::BlendMode::Subtract; }
		if (value == "multiply") { return MadoEngine::Render::BlendMode::Multiply; }
		if (value == "none") { return MadoEngine::Render::BlendMode::None; }
		return MadoEngine::Render::BlendMode::Add;
	}

	/// @brief Blend Modeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
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

	/// @brief Cull Mode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	MadoEngine::Render::CullMode ParseCullMode(const std::string& value) {
		if (value == "front") { return MadoEngine::Render::CullMode::Front; }
		if (value == "back") { return MadoEngine::Render::CullMode::Back; }
		return MadoEngine::Render::CullMode::None;
	}

	/// @brief Cull Modeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(MadoEngine::Render::CullMode value) {
		switch (value) {
		case MadoEngine::Render::CullMode::Front: return "front";
		case MadoEngine::Render::CullMode::Back: return "back";
		case MadoEngine::Render::CullMode::None:
		default: return "none";
		}
	}

} // namespace

namespace MadoEngine::Ribbon {

	bool RibbonEffectAsset::LoadFromFile(const std::filesystem::path& filePath) {
		JsonValue json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}
		filePath_ = filePath;
		name_ = PathToUtf8String(filePath.stem());
		FromJson(json);
		return true;
	}

	bool RibbonEffectAsset::SaveToFile(
		const std::filesystem::path& filePath,
		bool createBackup) const {
		const std::filesystem::path outputPath = filePath.empty() ? filePath_ : filePath;
		if (outputPath.empty()) {
			Logger::Output("Ribbon Effect Assetの保存先が指定されていません。", Logger::Level::Error);
			return false;
		}
		return MadoEngine::Json::JsonFile::Save(outputPath, ToJson(), 4, createBackup);
	}

	void RibbonEffectAsset::FromJson(const nlohmann::json& json) {
		config_ = RibbonEffectConfig{};
		version_ = ReadUInt(json, "version", kCurrentVersion);
		if (version_ > kCurrentVersion) {
			Logger::Output(
				"未対応のRibbon Effect Asset Versionです: " + std::to_string(version_),
				Logger::Level::Warning
			);
		}

		if (const JsonValue* playback = FindValue(json, "playback")) {
			config_.playback.duration = ReadFloat(*playback, "duration", config_.playback.duration);
			config_.playback.isLoop = ReadBool(*playback, "isLoop", config_.playback.isLoop);
			config_.playback.mode = ParsePlaybackMode(ReadString(*playback, "mode", "full"));
			config_.playback.sweepLength = ReadFloat(
				*playback,
				"sweepLength",
				config_.playback.sweepLength
			);
			if (const JsonValue* progress = FindValue(*playback, "progress")) {
				config_.playback.progress = ReadTrack(
					*progress,
					config_.playback.progress.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
			}
		}
		if (const JsonValue* trail = FindValue(json, "trail")) {
			config_.trail.pointLifetime = ReadFloat(*trail, "pointLifetime", config_.trail.pointLifetime);
			config_.trail.minPointDistance = ReadFloat(*trail, "minPointDistance", config_.trail.minPointDistance);
			config_.trail.maxPointCount = ReadUInt(*trail, "maxPointCount", config_.trail.maxPointCount);
			config_.trail.generationMode = ParseGenerationMode(
				ReadString(*trail, "generationMode", "transformHistory")
			);
			config_.trail.simulationSpace = ParseSimulationSpace(
				ReadString(*trail, "simulationSpace", "world")
			);
			if (const JsonValue* controlPoints = FindValue(*trail, "defaultControlPoints")) {
				config_.trail.defaultControlPoints.clear();
				if (controlPoints->is_array()) {
					config_.trail.defaultControlPoints.reserve(controlPoints->size());
					for (const JsonValue& controlPoint : *controlPoints) {
						config_.trail.defaultControlPoints.push_back(ReadVector3(controlPoint, Vector3{}));
					}
				}
			}
		}
		if (const JsonValue* geometry = FindValue(json, "geometry")) {
			config_.geometry.interpolation = ParseInterpolation(
				ReadString(*geometry, "interpolation", "linear")
			);
			config_.geometry.smoothingSubdivision = ReadUInt(
				*geometry,
				"smoothingSubdivision",
				config_.geometry.smoothingSubdivision
			);
			config_.geometry.cameraFacing = ReadBool(*geometry, "cameraFacing", config_.geometry.cameraFacing);
			if (const JsonValue* width = FindValue(*geometry, "widthOverLifetime")) {
				config_.geometry.widthOverLifetime = ReadTrack(
					*width,
					config_.geometry.widthOverLifetime.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
			}
		}
		if (const JsonValue* material = FindValue(json, "material")) {
			config_.material.textureName = ReadString(*material, "textureName", config_.material.textureName);
			config_.material.blendMode = ParseBlendMode(ReadString(*material, "blendMode", "add"));
			config_.material.cullMode = ParseCullMode(ReadString(*material, "cullMode", "none"));
			config_.material.uvMode = ParseUvMode(ReadString(*material, "uvMode", "stretch"));
			config_.material.tileLength = ReadFloat(*material, "tileLength", config_.material.tileLength);
			if (const JsonValue* value = FindValue(*material, "uvScale")) {
				config_.material.uvScale = ReadVector2(*value, config_.material.uvScale);
			}
			if (const JsonValue* value = FindValue(*material, "uvOffset")) {
				config_.material.uvOffset = ReadVector2(*value, config_.material.uvOffset);
			}
			if (const JsonValue* value = FindValue(*material, "uvScroll")) {
				config_.material.uvScroll = ReadVector2(*value, config_.material.uvScroll);
			}
			if (const JsonValue* color = FindValue(*material, "colorOverLifetime")) {
				config_.material.colorOverLifetime = ReadTrack(
					*color,
					config_.material.colorOverLifetime.GetDefaultValue(),
					ReadVector4
				);
			}
			if (const JsonValue* alpha = FindValue(*material, "globalAlpha")) {
				config_.material.globalAlpha = ReadTrack(
					*alpha,
					config_.material.globalAlpha.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
			}
		}
		Validate();
	}

	nlohmann::json RibbonEffectAsset::ToJson() const {
		return JsonValue{
			{ "version", kCurrentVersion },
			{ "playback", {
				{ "duration", config_.playback.duration },
				{ "isLoop", config_.playback.isLoop },
				{ "mode", ToString(config_.playback.mode) },
				{ "progress", WriteTrack(config_.playback.progress, [](float value) { return JsonValue(value); }) },
				{ "sweepLength", config_.playback.sweepLength },
			} },
			{ "trail", {
				{ "pointLifetime", config_.trail.pointLifetime },
				{ "minPointDistance", config_.trail.minPointDistance },
				{ "maxPointCount", config_.trail.maxPointCount },
				{ "generationMode", ToString(config_.trail.generationMode) },
				{ "simulationSpace", ToString(config_.trail.simulationSpace) },
				{ "defaultControlPoints", WriteControlPoints(config_.trail.defaultControlPoints) },
			} },
			{ "geometry", {
				{ "widthOverLifetime", WriteTrack(config_.geometry.widthOverLifetime, [](float value) { return JsonValue(value); }) },
				{ "interpolation", ToString(config_.geometry.interpolation) },
				{ "smoothingSubdivision", config_.geometry.smoothingSubdivision },
				{ "cameraFacing", config_.geometry.cameraFacing },
			} },
			{ "material", {
				{ "textureName", config_.material.textureName },
				{ "blendMode", ToString(config_.material.blendMode) },
				{ "cullMode", ToString(config_.material.cullMode) },
				{ "colorOverLifetime", WriteTrack(config_.material.colorOverLifetime, WriteVector4) },
				{ "globalAlpha", WriteTrack(config_.material.globalAlpha, [](float value) { return JsonValue(value); }) },
				{ "uvScale", WriteVector2(config_.material.uvScale) },
				{ "uvOffset", WriteVector2(config_.material.uvOffset) },
				{ "uvScroll", WriteVector2(config_.material.uvScroll) },
				{ "uvMode", ToString(config_.material.uvMode) },
				{ "tileLength", config_.material.tileLength },
			} },
		};
	}

	void RibbonEffectAsset::Validate() {
		switch (config_.playback.mode) {
		case RibbonPlaybackMode::Full:
		case RibbonPlaybackMode::Reveal:
		case RibbonPlaybackMode::Sweep:
			break;
		default:
			config_.playback.mode = RibbonPlaybackMode::Full;
			break;
		}
		config_.playback.duration = std::clamp(
			std::isfinite(config_.playback.duration) ? config_.playback.duration : 1.0f,
			0.001f,
			3600.0f
		);
		config_.playback.sweepLength = std::clamp(
			std::isfinite(config_.playback.sweepLength) ? config_.playback.sweepLength : 1.0f,
			0.001f,
			100000.0f
		);
		NormalizeTrack(
			config_.playback.progress,
			1.0f,
			[](float value) {
				return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
			}
		);
		config_.trail.pointLifetime = std::clamp(
			std::isfinite(config_.trail.pointLifetime) ? config_.trail.pointLifetime : 0.5f,
			0.001f,
			3600.0f
		);
		config_.trail.minPointDistance = std::clamp(
			std::isfinite(config_.trail.minPointDistance) ? config_.trail.minPointDistance : 0.05f,
			0.0f,
			100000.0f
		);
		config_.trail.maxPointCount = std::clamp(
			config_.trail.maxPointCount,
			kMinimumRibbonPointCount,
			kMaximumRibbonPointCount
		);
		config_.trail.defaultControlPoints.erase(
			std::remove_if(
				config_.trail.defaultControlPoints.begin(),
				config_.trail.defaultControlPoints.end(),
				[](const Vector3& point) {
					return !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z);
				}
			),
			config_.trail.defaultControlPoints.end()
		);
		if (config_.trail.defaultControlPoints.size() > config_.trail.maxPointCount) {
			config_.trail.defaultControlPoints.resize(config_.trail.maxPointCount);
		}
		config_.geometry.smoothingSubdivision = std::clamp(
			config_.geometry.smoothingSubdivision,
			0u,
			kMaximumRibbonSmoothingSubdivision
		);
		if (
			config_.geometry.interpolation == RibbonInterpolationMode::CatmullRom &&
			config_.geometry.smoothingSubdivision == 0) {
			config_.geometry.smoothingSubdivision = kDefaultRibbonCurveSubdivision;
		}

		NormalizeTrack(
			config_.geometry.widthOverLifetime,
			1.0f,
			[](float value) {
				return std::clamp(std::isfinite(value) ? value : 0.5f, 0.0f, 100000.0f);
			}
		);
		NormalizeTrack(
			config_.material.colorOverLifetime,
			1.0f,
			[](const Vector4& value) {
				return Vector4{
					std::clamp(std::isfinite(value.x) ? value.x : 1.0f, 0.0f, 100.0f),
					std::clamp(std::isfinite(value.y) ? value.y : 1.0f, 0.0f, 100.0f),
					std::clamp(std::isfinite(value.z) ? value.z : 1.0f, 0.0f, 100.0f),
					std::clamp(std::isfinite(value.w) ? value.w : 1.0f, 0.0f, 1.0f),
				};
			}
		);
		NormalizeTrack(
			config_.material.globalAlpha,
			config_.playback.duration,
			[](float value) {
				return std::clamp(std::isfinite(value) ? value : 1.0f, 0.0f, 1.0f);
			}
		);

		if (config_.material.textureName.empty()) {
			config_.material.textureName = "white2x2";
		}
		config_.material.uvScale.x = std::isfinite(config_.material.uvScale.x) ? config_.material.uvScale.x : 1.0f;
		config_.material.uvScale.y = std::isfinite(config_.material.uvScale.y) ? config_.material.uvScale.y : 1.0f;
		config_.material.uvOffset.x = std::isfinite(config_.material.uvOffset.x) ? config_.material.uvOffset.x : 0.0f;
		config_.material.uvOffset.y = std::isfinite(config_.material.uvOffset.y) ? config_.material.uvOffset.y : 0.0f;
		config_.material.uvScroll.x = std::isfinite(config_.material.uvScroll.x) ? config_.material.uvScroll.x : 0.0f;
		config_.material.uvScroll.y = std::isfinite(config_.material.uvScroll.y) ? config_.material.uvScroll.y : 0.0f;
		config_.material.tileLength = std::clamp(
			std::isfinite(config_.material.tileLength) ? config_.material.tileLength : 1.0f,
			0.001f,
			100000.0f
		);
	}

} // namespace MadoEngine::Ribbon
