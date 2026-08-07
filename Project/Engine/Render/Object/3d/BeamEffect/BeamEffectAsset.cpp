#include "BeamEffectAsset.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace {

	using JsonValue = nlohmann::json;
	using namespace MadoEngine::Beam;

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
		if (!value) {
			return fallback;
		}
		if (value->is_number_unsigned()) {
			const uint64_t number = value->get<uint64_t>();
			return number <= (std::numeric_limits<uint32_t>::max)()
				? static_cast<uint32_t>(number)
				: fallback;
		}
		if (!value->is_number_integer()) {
			return fallback;
		}
		const int64_t number = value->get<int64_t>();
		return number >= 0 && number <= (std::numeric_limits<uint32_t>::max)()
			? static_cast<uint32_t>(number)
			: fallback;
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
		std::vector<MadoEngine::Effect::EffectKeyframe<T>> keys;
		if (const JsonValue* keyArray = FindValue(json, "keys"); keyArray && keyArray->is_array()) {
			for (const JsonValue& keyJson : *keyArray) {
				if (!keyJson.is_object()) {
					continue;
				}
				MadoEngine::Effect::EffectKeyframe<T> key;
				key.time = ReadFloat(keyJson, "time", 0.0f);
				if (const JsonValue* value = FindValue(keyJson, "value")) {
					key.value = reader(*value, track.GetDefaultValue());
				} else {
					key.value = track.GetDefaultValue();
				}
				key.easing = static_cast<EaseType>((std::min)(
					ReadUInt(keyJson, "easing", 0),
					static_cast<uint32_t>(EaseType::None)
				));
				keys.push_back(key);
			}
		}
		track.SetKeyframes(std::move(keys));
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
		for (const MadoEngine::Effect::EffectKeyframe<T>& key : track.GetKeyframes()) {
			json["keys"].push_back({
				{ "time", key.time },
				{ "value", writer(key.value) },
				{ "easing", static_cast<uint32_t>(key.easing) },
			});
		}
		return json;
	}

	/// @brief Track全体を安全な範囲へ補正する
	/// @tparam T Track値型
	/// @tparam Normalizer 値補正関数型
	/// @param track 補正対象Track
	/// @param normalizer 値補正関数
	template<class T, class Normalizer>
	void NormalizeTrack(MadoEngine::Effect::EffectTrack<T>& track, Normalizer normalizer) {
		track.SetDefaultValue(normalizer(track.GetDefaultValue()));
		std::vector<MadoEngine::Effect::EffectKeyframe<T>> keys = track.GetKeyframes();
		for (MadoEngine::Effect::EffectKeyframe<T>& key : keys) {
			key.time = std::clamp(std::isfinite(key.time) ? key.time : 0.0f, 0.0f, 1.0f);
			key.value = normalizer(key.value);
		}
		track.SetKeyframes(std::move(keys));
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

	/// @brief UV Mode文字列をEnumへ変換する
	/// @param value 変換元文字列
	/// @return 変換後Enum
	MadoEngine::Ribbon::RibbonUvMode ParseUvMode(const std::string& value) {
		return value == "tile"
			? MadoEngine::Ribbon::RibbonUvMode::Tile
			: MadoEngine::Ribbon::RibbonUvMode::Stretch;
	}

	/// @brief UV Modeを文字列へ変換する
	/// @param value 変換対象Enum
	/// @return 変換後文字列
	const char* ToString(MadoEngine::Ribbon::RibbonUvMode value) {
		return value == MadoEngine::Ribbon::RibbonUvMode::Tile ? "tile" : "stretch";
	}

} // namespace

namespace MadoEngine::Beam {

	bool BeamEffectAsset::LoadFromFile(const std::filesystem::path& filePath) {
		JsonValue json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}
		filePath_ = filePath;
		name_ = PathToUtf8String(filePath.stem());
		FromJson(json);
		return true;
	}

	bool BeamEffectAsset::SaveToFile(
		const std::filesystem::path& filePath,
		bool createBackup) const {
		const std::filesystem::path outputPath = filePath.empty() ? filePath_ : filePath;
		if (outputPath.empty()) {
			Logger::Output("Beam Effect Assetの保存先が指定されていません。", Logger::Level::Error);
			return false;
		}
		return MadoEngine::Json::JsonFile::Save(outputPath, ToJson(), 4, createBackup);
	}

	void BeamEffectAsset::FromJson(const nlohmann::json& json) {
		config_ = BeamEffectConfig{};
		version_ = ReadUInt(json, "version", kCurrentVersion);
		const uint32_t loadedVersion = version_;
		if (version_ > kCurrentVersion) {
			Logger::Output(
				"未対応のBeam Effect Asset Versionです: " + std::to_string(version_),
				Logger::Level::Warning
			);
		}
		bool hasExtensionTrack = false;
		if (const JsonValue* playback = FindValue(json, "playback")) {
			config_.playback.duration = ReadFloat(*playback, "duration", config_.playback.duration);
			config_.playback.isLoop = ReadBool(*playback, "isLoop", config_.playback.isLoop);
			if (const JsonValue* extension = FindValue(*playback, "extensionOverTime")) {
				config_.playback.extensionOverTime = ReadTrack(
					*extension,
					config_.playback.extensionOverTime.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
				hasExtensionTrack = true;
			}
		}
		if (loadedVersion < 2 && !hasExtensionTrack) {
			config_.playback.extensionOverTime = MadoEngine::Effect::EffectTrack<float>{ 1.0f };
		}
		if (const JsonValue* geometry = FindValue(json, "geometry")) {
			config_.geometry.segmentCount = ReadUInt(*geometry, "segmentCount", config_.geometry.segmentCount);
			config_.geometry.cameraFacing = ReadBool(*geometry, "cameraFacing", config_.geometry.cameraFacing);
			config_.geometry.startFade = ReadFloat(*geometry, "startFade", config_.geometry.startFade);
			config_.geometry.endFade = ReadFloat(*geometry, "endFade", config_.geometry.endFade);
			if (const JsonValue* width = FindValue(*geometry, "widthOverTime")) {
				config_.geometry.widthOverTime = ReadTrack(
					*width,
					config_.geometry.widthOverTime.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
			}
		}
		if (const JsonValue* noise = FindValue(json, "noise")) {
			config_.noise.amplitude = ReadFloat(*noise, "amplitude", config_.noise.amplitude);
			config_.noise.frequency = ReadFloat(*noise, "frequency", config_.noise.frequency);
			config_.noise.scrollSpeed = ReadFloat(*noise, "scrollSpeed", config_.noise.scrollSpeed);
			config_.noise.seed = ReadUInt(*noise, "seed", config_.noise.seed);
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
			if (const JsonValue* color = FindValue(*material, "colorOverTime")) {
				config_.material.colorOverTime = ReadTrack(
					*color,
					config_.material.colorOverTime.GetDefaultValue(),
					ReadVector4
				);
			}
			if (const JsonValue* color = FindValue(*material, "colorOverLength")) {
				config_.material.colorOverLength = ReadTrack(
					*color,
					config_.material.colorOverLength.GetDefaultValue(),
					ReadVector4
				);
			}
			if (const JsonValue* alpha = FindValue(*material, "globalAlphaOverTime")) {
				config_.material.globalAlphaOverTime = ReadTrack(
					*alpha,
					config_.material.globalAlphaOverTime.GetDefaultValue(),
					[](const JsonValue& value, float fallback) {
						return value.is_number() ? value.get<float>() : fallback;
					}
				);
			}
		}
		Validate();
	}

	nlohmann::json BeamEffectAsset::ToJson() const {
		return JsonValue{
			{ "version", kCurrentVersion },
			{ "playback", {
				{ "duration", config_.playback.duration },
				{ "isLoop", config_.playback.isLoop },
				{ "extensionOverTime", WriteTrack(config_.playback.extensionOverTime, [](float value) { return JsonValue(value); }) },
			} },
			{ "geometry", {
				{ "widthOverTime", WriteTrack(config_.geometry.widthOverTime, [](float value) { return JsonValue(value); }) },
				{ "segmentCount", config_.geometry.segmentCount },
				{ "cameraFacing", config_.geometry.cameraFacing },
				{ "startFade", config_.geometry.startFade },
				{ "endFade", config_.geometry.endFade },
			} },
			{ "noise", {
				{ "amplitude", config_.noise.amplitude },
				{ "frequency", config_.noise.frequency },
				{ "scrollSpeed", config_.noise.scrollSpeed },
				{ "seed", config_.noise.seed },
			} },
			{ "material", {
				{ "textureName", config_.material.textureName },
				{ "blendMode", ToString(config_.material.blendMode) },
				{ "cullMode", ToString(config_.material.cullMode) },
				{ "colorOverTime", WriteTrack(config_.material.colorOverTime, WriteVector4) },
				{ "colorOverLength", WriteTrack(config_.material.colorOverLength, WriteVector4) },
				{ "globalAlphaOverTime", WriteTrack(config_.material.globalAlphaOverTime, [](float value) { return JsonValue(value); }) },
				{ "uvScale", WriteVector2(config_.material.uvScale) },
				{ "uvOffset", WriteVector2(config_.material.uvOffset) },
				{ "uvScroll", WriteVector2(config_.material.uvScroll) },
				{ "uvMode", ToString(config_.material.uvMode) },
				{ "tileLength", config_.material.tileLength },
			} },
		};
	}

	void BeamEffectAsset::Validate() {
		version_ = kCurrentVersion;
		config_.playback.duration = std::clamp(
			std::isfinite(config_.playback.duration) ? config_.playback.duration : 1.0f,
			0.001f,
			3600.0f
		);
		config_.geometry.segmentCount = std::clamp(
			config_.geometry.segmentCount,
			kMinimumBeamSegmentCount,
			kMaximumBeamSegmentCount
		);
		config_.geometry.startFade = std::clamp(
			std::isfinite(config_.geometry.startFade) ? config_.geometry.startFade : 0.0f,
			0.0f,
			1.0f
		);
		config_.geometry.endFade = std::clamp(
			std::isfinite(config_.geometry.endFade) ? config_.geometry.endFade : 0.0f,
			0.0f,
			1.0f
		);
		const float fadeSum = config_.geometry.startFade + config_.geometry.endFade;
		if (fadeSum > 1.0f) {
			config_.geometry.startFade /= fadeSum;
			config_.geometry.endFade /= fadeSum;
		}
		config_.noise.amplitude = std::clamp(
			std::isfinite(config_.noise.amplitude) ? config_.noise.amplitude : 0.0f,
			0.0f,
			100000.0f
		);
		config_.noise.frequency = std::clamp(
			std::isfinite(config_.noise.frequency) ? config_.noise.frequency : 0.0f,
			0.0f,
			10000.0f
		);
		config_.noise.scrollSpeed = std::clamp(
			std::isfinite(config_.noise.scrollSpeed) ? config_.noise.scrollSpeed : 0.0f,
			-10000.0f,
			10000.0f
		);
		if (config_.material.textureName.empty()) {
			config_.material.textureName = "white2x2";
		}
		const auto normalizeFloat = [](float value, float minimum, float maximum, float fallback) {
			return std::clamp(std::isfinite(value) ? value : fallback, minimum, maximum);
		};
		NormalizeTrack(config_.geometry.widthOverTime, [&](float value) {
			return normalizeFloat(value, 0.0f, 100000.0f, 0.25f);
		});
		NormalizeTrack(config_.playback.extensionOverTime, [&](float value) {
			return normalizeFloat(value, 0.0f, 1.0f, 1.0f);
		});
		NormalizeTrack(config_.material.globalAlphaOverTime, [&](float value) {
			return normalizeFloat(value, 0.0f, 1.0f, 1.0f);
		});
		const auto normalizeColor = [&](const Vector4& value) {
			return Vector4{
				normalizeFloat(value.x, 0.0f, 100.0f, 1.0f),
				normalizeFloat(value.y, 0.0f, 100.0f, 1.0f),
				normalizeFloat(value.z, 0.0f, 100.0f, 1.0f),
				normalizeFloat(value.w, 0.0f, 1.0f, 1.0f),
			};
		};
		NormalizeTrack(config_.material.colorOverTime, normalizeColor);
		NormalizeTrack(config_.material.colorOverLength, normalizeColor);
		config_.material.uvScale.x = normalizeFloat(config_.material.uvScale.x, -10000.0f, 10000.0f, 1.0f);
		config_.material.uvScale.y = normalizeFloat(config_.material.uvScale.y, -10000.0f, 10000.0f, 1.0f);
		config_.material.uvOffset.x = normalizeFloat(config_.material.uvOffset.x, -100000.0f, 100000.0f, 0.0f);
		config_.material.uvOffset.y = normalizeFloat(config_.material.uvOffset.y, -100000.0f, 100000.0f, 0.0f);
		config_.material.uvScroll.x = normalizeFloat(config_.material.uvScroll.x, -10000.0f, 10000.0f, 0.0f);
		config_.material.uvScroll.y = normalizeFloat(config_.material.uvScroll.y, -10000.0f, 10000.0f, 0.0f);
		config_.material.tileLength = normalizeFloat(config_.material.tileLength, 0.0001f, 100000.0f, 1.0f);
	}

} // namespace MadoEngine::Beam
