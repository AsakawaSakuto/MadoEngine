#include "ParticleEffectAsset.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace {

	using Json = nlohmann::json;
	using namespace MadoEngine::Particle;

	/// @brief Filesystem PathをUTF-8文字列へ変換する
	/// @param path 変換するPath
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(
			reinterpret_cast<const char*>(value.data()),
			value.size()
		);
	}

	/// @brief Jsonオブジェクトから子要素を取得する
	/// @param json 検索元Json
	/// @param key 検索するキー
	/// @return 子要素。存在しない場合はnullptr
	const Json* FindValue(const Json& json, const char* key) {
		if (!json.is_object() || !json.contains(key) || json.at(key).is_null()) {
			return nullptr;
		}

		return &json.at(key);
	}

	/// @brief Jsonから文字列を安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ文字列
	std::string ReadString(const Json& json, const char* key, const std::string& fallback) {
		const Json* value = FindValue(json, key);
		return value && value->is_string() ? value->get<std::string>() : fallback;
	}

	/// @brief Jsonからfloatを安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ値
	float ReadFloat(const Json& json, const char* key, float fallback) {
		const Json* value = FindValue(json, key);
		return value && value->is_number() ? value->get<float>() : fallback;
	}

	/// @brief Jsonからuint32_tを安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ値
	uint32_t ReadUInt(const Json& json, const char* key, uint32_t fallback) {
		const Json* value = FindValue(json, key);
		if (!value || !value->is_number_unsigned()) {
			return fallback;
		}

		return value->get<uint32_t>();
	}

	/// @brief Jsonからboolを安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ値
	bool ReadBool(const Json& json, const char* key, bool fallback) {
		const Json* value = FindValue(json, key);
		return value && value->is_boolean() ? value->get<bool>() : fallback;
	}

	/// @brief JsonからVector2を安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだVector2
	Vector2 ReadVector2(const Json& json, const char* key, const Vector2& fallback) {
		const Json* value = FindValue(json, key);
		return value ? MadoEngine::Json::JsonSerializer::ToVector2(*value, fallback) : fallback;
	}

	/// @brief JsonからVector3を安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだVector3
	Vector3 ReadVector3(const Json& json, const char* key, const Vector3& fallback) {
		const Json* value = FindValue(json, key);
		return value ? MadoEngine::Json::JsonSerializer::ToVector3(*value, fallback) : fallback;
	}

	/// @brief JsonからVector4を安全に取得する
	/// @param json 読み込み元Json
	/// @param key 読み込むキー
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだVector4
	Vector4 ReadVector4(const Json& json, const char* key, const Vector4& fallback) {
		const Json* value = FindValue(json, key);
		return value ? MadoEngine::Json::JsonSerializer::ToVector4(*value, fallback) : fallback;
	}

	/// @brief Jsonからfloat範囲を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ範囲
	ValueRange<float> ReadFloatRange(const Json& json, const ValueRange<float>& fallback) {
		if (json.is_number()) {
			const float value = json.get<float>();
			return { value, value };
		}
		if (!json.is_object()) {
			return fallback;
		}

		return {
			ReadFloat(json, "min", fallback.min),
			ReadFloat(json, "max", fallback.max),
		};
	}

	/// @brief JsonからVector2範囲を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ範囲
	ValueRange<Vector2> ReadVector2Range(const Json& json, const ValueRange<Vector2>& fallback) {
		if (json.is_array()) {
			const Vector2 value = MadoEngine::Json::JsonSerializer::ToVector2(json, fallback.min);
			return { value, value };
		}
		if (!json.is_object()) {
			return fallback;
		}

		return {
			ReadVector2(json, "min", fallback.min),
			ReadVector2(json, "max", fallback.max),
		};
	}

	/// @brief JsonからVector3範囲を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ範囲
	ValueRange<Vector3> ReadVector3Range(const Json& json, const ValueRange<Vector3>& fallback) {
		if (json.is_array()) {
			const Vector3 value = MadoEngine::Json::JsonSerializer::ToVector3(json, fallback.min);
			return { value, value };
		}
		if (!json.is_object()) {
			return fallback;
		}

		return {
			ReadVector3(json, "min", fallback.min),
			ReadVector3(json, "max", fallback.max),
		};
	}

	/// @brief JsonからVector4範囲を読み込む
	/// @param json 読み込み元Json
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだ範囲
	ValueRange<Vector4> ReadVector4Range(const Json& json, const ValueRange<Vector4>& fallback) {
		if (json.is_array()) {
			const Vector4 value = MadoEngine::Json::JsonSerializer::ToVector4(json, fallback.min);
			return { value, value };
		}
		if (!json.is_object()) {
			return fallback;
		}

		return {
			ReadVector4(json, "min", fallback.min),
			ReadVector4(json, "max", fallback.max),
		};

	}

	/// @brief float範囲をJsonへ変換する
	/// @param range 変換する範囲
	/// @return 変換したJson
	Json WriteRange(const ValueRange<float>& range) {
		return Json{ { "min", range.min }, { "max", range.max } };
	}

	/// @brief Vector2範囲をJsonへ変換する
	/// @param range 変換する範囲
	/// @return 変換したJson
	Json WriteRange(const ValueRange<Vector2>& range) {
		return Json{ { "min", range.min }, { "max", range.max } };
	}

	/// @brief Vector3範囲をJsonへ変換する
	/// @param range 変換する範囲
	/// @return 変換したJson
	Json WriteRange(const ValueRange<Vector3>& range) {
		return Json{ { "min", range.min }, { "max", range.max } };
	}

	/// @brief Vector4範囲をJsonへ変換する
	/// @param range 変換する範囲
	/// @return 変換したJson
	Json WriteRange(const ValueRange<Vector4>& range) {
		return Json{ { "min", range.min }, { "max", range.max } };
	}

	/// @brief 文字列からSimulationSpaceを取得する
	/// @param value 変換元文字列
	/// @return 変換したSimulationSpace
	SimulationSpace ParseSimulationSpace(const std::string& value) {
		return value == "local" ? SimulationSpace::Local : SimulationSpace::World;
	}

	/// @brief SimulationSpaceを文字列へ変換する
	/// @param value 変換するSimulationSpace
	/// @return 変換した文字列
	const char* ToString(SimulationSpace value) {
		return value == SimulationSpace::Local ? "local" : "world";
	}

	/// @brief 文字列からDirectionModeを取得する
	/// @param value 変換元文字列
	/// @return 変換したDirectionMode
	DirectionMode ParseDirectionMode(const std::string& value) {
		return value == "shapeOutward" ? DirectionMode::ShapeOutward : DirectionMode::Configured;
	}

	/// @brief DirectionModeを文字列へ変換する
	/// @param value 変換するDirectionMode
	/// @return 変換した文字列
	const char* ToString(DirectionMode value) {
		return value == DirectionMode::ShapeOutward ? "shapeOutward" : "configured";
	}

	/// @brief 文字列からSortModeを取得する
	/// @param value 変換元文字列
	/// @return 変換したSortMode
	SortMode ParseSortMode(const std::string& value) {
		return value == "backToFront" ? SortMode::BackToFront : SortMode::None;
	}

	/// @brief SortModeを文字列へ変換する
	/// @param value 変換するSortMode
	/// @return 変換した文字列
	const char* ToString(SortMode value) {
		return value == SortMode::BackToFront ? "backToFront" : "none";
	}

	/// @brief 文字列からBlendModeを取得する
	/// @param value 変換元文字列
	/// @return 変換したBlendMode
	MadoEngine::Render::BlendMode ParseBlendMode(const std::string& value) {
		if (value == "normal") { return MadoEngine::Render::BlendMode::Normal; }
		if (value == "subtract") { return MadoEngine::Render::BlendMode::Subtract; }
		if (value == "multiply") { return MadoEngine::Render::BlendMode::Multiply; }
		if (value == "none") { return MadoEngine::Render::BlendMode::None; }
		return MadoEngine::Render::BlendMode::Add;
	}

	/// @brief BlendModeを文字列へ変換する
	/// @param value 変換するBlendMode
	/// @return 変換した文字列
	const char* ToString(MadoEngine::Render::BlendMode value) {
		switch (value) {
		case MadoEngine::Render::BlendMode::Normal:
			return "normal";
		case MadoEngine::Render::BlendMode::Subtract:
			return "subtract";
		case MadoEngine::Render::BlendMode::Multiply:
			return "multiply";
		case MadoEngine::Render::BlendMode::None:
			return "none";
		case MadoEngine::Render::BlendMode::Add:
		default:
			return "add";
		}
	}

	/// @brief Jsonから発生形状を読み込む
	/// @param json 読み込み元Json
	/// @return 読み込んだ発生形状
	ParticleShape ReadShape(const Json& json) {
		const std::string type = ReadString(json, "type", "point");
		if (type == "line") {
			LineShape shape;
			shape.start = ReadVector3(json, "start", shape.start);
			shape.end = ReadVector3(json, "end", shape.end);
			return shape;
		}
		if (type == "sphere") {
			SphereShape shape;
			shape.radius = ReadFloat(json, "radius", shape.radius);
			shape.emitFromSurface = ReadBool(json, "emitFromSurface", shape.emitFromSurface);
			return shape;
		}
		if (type == "box") {
			BoxShape shape;
			shape.halfExtents = ReadVector3(json, "halfExtents", shape.halfExtents);
			shape.emitFromSurface = ReadBool(json, "emitFromSurface", shape.emitFromSurface);
			return shape;
		}
		if (type == "plane") {
			PlaneShape shape;
			shape.halfExtents = ReadVector2(json, "halfExtents", shape.halfExtents);
			shape.normal = ReadVector3(json, "normal", shape.normal);
			return shape;
		}
		if (type == "ring") {
			RingShape shape;
			shape.innerRadius = ReadFloat(json, "innerRadius", shape.innerRadius);
			shape.outerRadius = ReadFloat(json, "outerRadius", shape.outerRadius);
			shape.normal = ReadVector3(json, "normal", shape.normal);
			shape.emitFromEdge = ReadBool(json, "emitFromEdge", shape.emitFromEdge);
			return shape;
		}

		PointShape shape;
		shape.offset = ReadVector3(json, "offset", shape.offset);
		return shape;
	}

	/// @brief 発生形状をJsonへ変換する
	/// @param shape 変換する発生形状
	/// @return 変換したJson
	Json WriteShape(const ParticleShape& shape) {
		return std::visit([](const auto& value) -> Json {
			using ShapeType = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<ShapeType, PointShape>) {
				return Json{ { "type", "point" }, { "offset", value.offset } };
			} else if constexpr (std::is_same_v<ShapeType, LineShape>) {
				return Json{ { "type", "line" }, { "start", value.start }, { "end", value.end } };
			} else if constexpr (std::is_same_v<ShapeType, SphereShape>) {
				return Json{ { "type", "sphere" }, { "radius", value.radius }, { "emitFromSurface", value.emitFromSurface } };
			} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
				return Json{ { "type", "box" }, { "halfExtents", value.halfExtents }, { "emitFromSurface", value.emitFromSurface } };
			} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
				return Json{ { "type", "plane" }, { "halfExtents", value.halfExtents }, { "normal", value.normal } };
			} else {
				return Json{
					{ "type", "ring" },
					{ "innerRadius", value.innerRadius },
					{ "outerRadius", value.outerRadius },
					{ "normal", value.normal },
					{ "emitFromEdge", value.emitFromEdge },
				};
			}
		}, shape);
	}

	/// @brief float範囲の最小値と最大値を正しい順序へ補正する
	/// @param range 補正する範囲
	void NormalizeRange(ValueRange<float>& range) {
		if (range.min > range.max) {
			std::swap(range.min, range.max);
		}
	}

	/// @brief Vector2範囲の最小値と最大値を成分ごとに補正する
	/// @param range 補正する範囲
	void NormalizeRange(ValueRange<Vector2>& range) {
		if (range.min.x > range.max.x) { std::swap(range.min.x, range.max.x); }
		if (range.min.y > range.max.y) { std::swap(range.min.y, range.max.y); }
	}

	/// @brief Vector3範囲の最小値と最大値を成分ごとに補正する
	/// @param range 補正する範囲
	void NormalizeRange(ValueRange<Vector3>& range) {
		if (range.min.x > range.max.x) { std::swap(range.min.x, range.max.x); }
		if (range.min.y > range.max.y) { std::swap(range.min.y, range.max.y); }
		if (range.min.z > range.max.z) { std::swap(range.min.z, range.max.z); }
	}

	/// @brief Vector4範囲の最小値と最大値を成分ごとに補正する
	/// @param range 補正する範囲
	void NormalizeRange(ValueRange<Vector4>& range) {
		if (range.min.x > range.max.x) { std::swap(range.min.x, range.max.x); }
		if (range.min.y > range.max.y) { std::swap(range.min.y, range.max.y); }
		if (range.min.z > range.max.z) { std::swap(range.min.z, range.max.z); }
		if (range.min.w > range.max.w) { std::swap(range.min.w, range.max.w); }
	}

} // namespace

namespace MadoEngine::Particle {

	bool ParticleEffectAsset::LoadFromFile(const std::filesystem::path& filePath) {
		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}

		filePath_ = filePath;
		name_ = PathToUtf8String(filePath.stem());
		FromJson(json);
		if (emitters_.empty()) {
			Logger::Output("Particle AssetにEmitterがありません: " + PathToUtf8String(filePath), Logger::Level::Warning);
			return false;
		}

		return true;
	}

	bool ParticleEffectAsset::SaveToFile(const std::filesystem::path& filePath, bool createBackup) const {
		const std::filesystem::path outputPath = filePath.empty() ? filePath_ : filePath;
		if (outputPath.empty()) {
			Logger::Output("Particle Assetの保存先が指定されていません。", Logger::Level::Error);
			return false;
		}

		return MadoEngine::Json::JsonFile::Save(outputPath, ToJson(), 4, createBackup);
	}

	void ParticleEffectAsset::FromJson(const nlohmann::json& json) {
		version_ = ReadUInt(json, "version", kCurrentVersion);
		if (version_ > kCurrentVersion) {
			Logger::Output("新しいVersionのParticle Assetを読み込みます: " + std::to_string(version_), Logger::Level::Warning);
		}

		const std::string jsonName = ReadString(json, "name", name_);
		if (!jsonName.empty()) {
			name_ = jsonName;
		}

		emitters_.clear();
		const nlohmann::json* emitterArray = FindValue(json, "emitters");
		if (!emitterArray || !emitterArray->is_array()) {
			return;
		}

		for (const nlohmann::json& emitterJson : *emitterArray) {
			if (!emitterJson.is_object()) {
				continue;
			}

			EmitterConfig emitter;
			emitter.name = ReadString(emitterJson, "name", emitter.name);
			emitter.simulationSpace = ParseSimulationSpace(ReadString(emitterJson, "simulationSpace", "world"));

			if (const nlohmann::json* emission = FindValue(emitterJson, "emission")) {
				emitter.emission.maxParticles = ReadUInt(*emission, "maxParticles", emitter.emission.maxParticles);
				emitter.emission.ratePerSecond = ReadFloat(*emission, "ratePerSecond", emitter.emission.ratePerSecond);
				emitter.emission.duration = ReadFloat(*emission, "duration", emitter.emission.duration);
				emitter.emission.startDelay = ReadFloat(*emission, "startDelay", emitter.emission.startDelay);
				emitter.emission.isLoop = ReadBool(*emission, "isLoop", emitter.emission.isLoop);
				if (const nlohmann::json* bursts = FindValue(*emission, "bursts"); bursts && bursts->is_array()) {
					for (const nlohmann::json& burstJson : *bursts) {
						BurstConfig burst;
						burst.time = ReadFloat(burstJson, "time", burst.time);
						burst.count = ReadUInt(burstJson, "count", burst.count);
						emitter.emission.bursts.push_back(burst);
					}
				}
			}

			if (const nlohmann::json* shape = FindValue(emitterJson, "shape")) {
				emitter.shape = ReadShape(*shape);
			}

			if (const nlohmann::json* initial = FindValue(emitterJson, "initial")) {
				if (const nlohmann::json* value = FindValue(*initial, "lifeTime")) {
					emitter.initial.lifeTime = ReadFloatRange(*value, emitter.initial.lifeTime);
				}
				if (const nlohmann::json* value = FindValue(*initial, "speed")) {
					emitter.initial.speed = ReadFloatRange(*value, emitter.initial.speed);
				}
				if (const nlohmann::json* value = FindValue(*initial, "direction")) {
					emitter.initial.direction = ReadVector3Range(*value, emitter.initial.direction);
				}
				if (const nlohmann::json* value = FindValue(*initial, "rotation")) {
					emitter.initial.rotation = ReadFloatRange(*value, emitter.initial.rotation);
				}
				if (const nlohmann::json* value = FindValue(*initial, "angularVelocity")) {
					emitter.initial.angularVelocity = ReadFloatRange(*value, emitter.initial.angularVelocity);
				}
				emitter.initial.directionMode = ParseDirectionMode(ReadString(*initial, "directionMode", "configured"));
			}

			if (const nlohmann::json* motion = FindValue(emitterJson, "motion")) {
				emitter.motion.gravity = ReadVector3(*motion, "gravity", emitter.motion.gravity);
				emitter.motion.acceleration = ReadVector3(*motion, "acceleration", emitter.motion.acceleration);
				emitter.motion.drag = ReadFloat(*motion, "drag", emitter.motion.drag);
			}

			if (const nlohmann::json* size = FindValue(emitterJson, "sizeOverLifetime")) {
				if (const nlohmann::json* value = FindValue(*size, "start")) {
					emitter.sizeOverLifetime.start = ReadVector2Range(*value, emitter.sizeOverLifetime.start);
				}
				if (const nlohmann::json* value = FindValue(*size, "end")) {
					emitter.sizeOverLifetime.end = ReadVector2Range(*value, emitter.sizeOverLifetime.end);
				}
			}

			if (const nlohmann::json* color = FindValue(emitterJson, "colorOverLifetime")) {
				if (const nlohmann::json* value = FindValue(*color, "start")) {
					emitter.colorOverLifetime.start = ReadVector4Range(*value, emitter.colorOverLifetime.start);
				}
				if (const nlohmann::json* value = FindValue(*color, "end")) {
					emitter.colorOverLifetime.end = ReadVector4Range(*value, emitter.colorOverLifetime.end);
				}
			}

			if (const nlohmann::json* renderer = FindValue(emitterJson, "renderer")) {
				emitter.renderer.textureName = ReadString(*renderer, "texture", emitter.renderer.textureName);
				emitter.renderer.blendMode = ParseBlendMode(ReadString(*renderer, "blendMode", "add"));
				emitter.renderer.sortMode = ParseSortMode(ReadString(*renderer, "sortMode", "none"));
			}

			emitters_.push_back(std::move(emitter));
		}

		Validate();
	}

	nlohmann::json ParticleEffectAsset::ToJson() const {
		nlohmann::json json;
		json["version"] = kCurrentVersion;
		json["name"] = name_;
		json["emitters"] = nlohmann::json::array();

		for (const EmitterConfig& emitter : emitters_) {
			nlohmann::json emitterJson;
			emitterJson["name"] = emitter.name;
			emitterJson["simulationSpace"] = ToString(emitter.simulationSpace);
			emitterJson["emission"] = {
				{ "maxParticles", emitter.emission.maxParticles },
				{ "ratePerSecond", emitter.emission.ratePerSecond },
				{ "duration", emitter.emission.duration },
				{ "startDelay", emitter.emission.startDelay },
				{ "isLoop", emitter.emission.isLoop },
				{ "bursts", nlohmann::json::array() },
			};
			for (const BurstConfig& burst : emitter.emission.bursts) {
				emitterJson["emission"]["bursts"].push_back({ { "time", burst.time }, { "count", burst.count } });
			}

			emitterJson["shape"] = WriteShape(emitter.shape);
			emitterJson["initial"] = {
				{ "lifeTime", WriteRange(emitter.initial.lifeTime) },
				{ "speed", WriteRange(emitter.initial.speed) },
				{ "direction", WriteRange(emitter.initial.direction) },
				{ "rotation", WriteRange(emitter.initial.rotation) },
				{ "angularVelocity", WriteRange(emitter.initial.angularVelocity) },
				{ "directionMode", ToString(emitter.initial.directionMode) },
			};
			emitterJson["motion"] = {
				{ "gravity", emitter.motion.gravity },
				{ "acceleration", emitter.motion.acceleration },
				{ "drag", emitter.motion.drag },
			};
			emitterJson["sizeOverLifetime"] = {
				{ "start", WriteRange(emitter.sizeOverLifetime.start) },
				{ "end", WriteRange(emitter.sizeOverLifetime.end) },
			};
			emitterJson["colorOverLifetime"] = {
				{ "start", WriteRange(emitter.colorOverLifetime.start) },
				{ "end", WriteRange(emitter.colorOverLifetime.end) },
			};
			emitterJson["renderer"] = {
				{ "texture", emitter.renderer.textureName },
				{ "blendMode", ToString(emitter.renderer.blendMode) },
				{ "sortMode", ToString(emitter.renderer.sortMode) },
			};
			json["emitters"].push_back(std::move(emitterJson));
		}

		return json;
	}

	void ParticleEffectAsset::Validate() {
		for (EmitterConfig& emitter : emitters_) {
			emitter.emission.maxParticles = (std::max)(1u, emitter.emission.maxParticles);
			emitter.emission.ratePerSecond = (std::max)(0.0f, emitter.emission.ratePerSecond);
			emitter.emission.duration = (std::max)(0.0f, emitter.emission.duration);
			emitter.emission.startDelay = (std::max)(0.0f, emitter.emission.startDelay);
			for (BurstConfig& burst : emitter.emission.bursts) {
				burst.time = std::clamp(burst.time, 0.0f, emitter.emission.duration);
				burst.count = (std::max)(1u, burst.count);
			}

			NormalizeRange(emitter.initial.lifeTime);
			NormalizeRange(emitter.initial.speed);
			NormalizeRange(emitter.initial.direction);
			NormalizeRange(emitter.initial.rotation);
			NormalizeRange(emitter.initial.angularVelocity);
			NormalizeRange(emitter.sizeOverLifetime.start);
			NormalizeRange(emitter.sizeOverLifetime.end);
			NormalizeRange(emitter.colorOverLifetime.start);
			NormalizeRange(emitter.colorOverLifetime.end);
			emitter.initial.lifeTime.min = (std::max)(0.001f, emitter.initial.lifeTime.min);
			emitter.initial.lifeTime.max = (std::max)(emitter.initial.lifeTime.min, emitter.initial.lifeTime.max);
			emitter.motion.drag = (std::max)(0.0f, emitter.motion.drag);

			std::visit([](auto& shape) {
				using ShapeType = std::decay_t<decltype(shape)>;
				if constexpr (std::is_same_v<ShapeType, SphereShape>) {
					shape.radius = (std::max)(0.0f, std::abs(shape.radius));
				} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
					shape.halfExtents.x = std::abs(shape.halfExtents.x);
					shape.halfExtents.y = std::abs(shape.halfExtents.y);
					shape.halfExtents.z = std::abs(shape.halfExtents.z);
				} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
					shape.halfExtents.x = std::abs(shape.halfExtents.x);
					shape.halfExtents.y = std::abs(shape.halfExtents.y);
					if (shape.normal.LengthSq() <= 0.000001f) {
						shape.normal = { 0.0f, 1.0f, 0.0f };
					}
				} else if constexpr (std::is_same_v<ShapeType, RingShape>) {
					shape.innerRadius = (std::max)(0.0f, std::abs(shape.innerRadius));
					shape.outerRadius = (std::max)(shape.innerRadius, std::abs(shape.outerRadius));
					if (shape.normal.LengthSq() <= 0.000001f) {
						shape.normal = { 0.0f, 1.0f, 0.0f };
					}
				}
			}, emitter.shape);

			if (emitter.renderer.textureName.empty()) {
				emitter.renderer.textureName = "white2x2";
			}
		}
	}

} // namespace MadoEngine::Particle
