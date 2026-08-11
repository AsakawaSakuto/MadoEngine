#include "EffectSequenceAsset.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace {

	using JsonValue = nlohmann::json;
	using namespace MadoEngine::EffectSequence;

	/// @brief Filesystem PathをUTF-8文字列へ変換
	/// @param path 変換対象Path
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(value.data()), value.size());
	}

	/// @brief JSON Objectから子要素を安全に取得
	/// @param json 検索対象JSON
	/// @param key 検索Key
	/// @return 子要素、存在しない場合はnullptr
	const JsonValue* FindValue(const JsonValue& json, const char* key) {
		if (!json.is_object() || !json.contains(key) || json.at(key).is_null()) {
			return nullptr;
		}
		return &json.at(key);
	}

	/// @brief JSONからfloatを安全に読み込み
	/// @param json 読み込み元JSON
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	float ReadFloat(const JsonValue& json, const char* key, float fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_number() ? value->get<float>() : fallback;
	}

	/// @brief JSONから符号なし整数を安全に読み込み
	/// @param json 読み込み元JSON
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

	/// @brief JSONからboolを安全に読み込み
	/// @param json 読み込み元JSON
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	bool ReadBool(const JsonValue& json, const char* key, bool fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_boolean() ? value->get<bool>() : fallback;
	}

	/// @brief JSONから文字列を安全に読み込み
	/// @param json 読み込み元JSON
	/// @param key 読み込みKey
	/// @param fallback 読み込み失敗時の値
	/// @return 読み込んだ値
	std::string ReadString(const JsonValue& json, const char* key, const std::string& fallback) {
		const JsonValue* value = FindValue(json, key);
		return value && value->is_string() ? value->get<std::string>() : fallback;
	}

	/// @brief Node Type文字列をEnumへ変換
	/// @param value 変換元文字列
	/// @return 対応するNode Type、未対応の場合はCount
	EffectSequenceNodeType ParseNodeType(const std::string& value) {
		if (value == "particle") { return EffectSequenceNodeType::Particle; }
		if (value == "primitiveEffect") { return EffectSequenceNodeType::PrimitiveEffect; }
		if (value == "ribbon") { return EffectSequenceNodeType::Ribbon; }
		if (value == "beam") { return EffectSequenceNodeType::Beam; }
		return EffectSequenceNodeType::Count;
	}

	/// @brief Node Typeを文字列へ変換
	/// @param value 変換元Node Type
	/// @return JSON保存用文字列
	const char* ToString(EffectSequenceNodeType value) {
		switch (value) {
		case EffectSequenceNodeType::Particle: return "particle";
		case EffectSequenceNodeType::PrimitiveEffect: return "primitiveEffect";
		case EffectSequenceNodeType::Ribbon: return "ribbon";
		case EffectSequenceNodeType::Beam: return "beam";
		case EffectSequenceNodeType::Count:
		default: return "invalid";
		}
	}

	/// @brief Primitive Effect種類を文字列へ変換
	/// @param value 変換元種類
	/// @return JSON保存用文字列
	const char* ToString(PrimitiveEffectNodeKind value) {
		return value == PrimitiveEffectNodeKind::Cylinder ? "cylinder" : "invalid";
	}

	/// @brief RenderLayer文字列を安全に解析
	/// @param value 解析する文字列
	/// @return 有効なRenderLayer、未対応の場合はstd::nullopt
	std::optional<MadoEngine::Render::RenderLayer> ParseRenderLayer(const std::string& value) {
		for (uint32_t index = 0; index < MadoEngine::Render::kRenderLayerCount; ++index) {
			const auto layer = MadoEngine::Render::GetRenderLayerByIndex(index);
			if (value == MadoEngine::Render::GetRenderLayerName(layer)) {
				return layer;
			}
		}
		return std::nullopt;
	}

	/// @brief Vector3の全要素を安全な範囲へ補正
	/// @param value 補正対象Vector
	/// @param fallback 非有限値の場合の既定値
	/// @param minimum 最小値
	/// @param maximum 最大値
	void NormalizeVector3(
		Vector3& value,
		const Vector3& fallback,
		float minimum,
		float maximum) {
		value.x = std::clamp(std::isfinite(value.x) ? value.x : fallback.x, minimum, maximum);
		value.y = std::clamp(std::isfinite(value.y) ? value.y : fallback.y, minimum, maximum);
		value.z = std::clamp(std::isfinite(value.z) ? value.z : fallback.z, minimum, maximum);
	}

	/// @brief Transformを安全な範囲へ補正
	/// @param transform 補正対象Transform
	void NormalizeTransform(Transform3D& transform) {
		NormalizeVector3(transform.scale, { 1.0f, 1.0f, 1.0f }, 0.001f, 10000.0f);
		NormalizeVector3(transform.rotate, {}, -10000.0f, 10000.0f);
		NormalizeVector3(transform.translate, {}, -1000000.0f, 1000000.0f);
	}

	/// @brief Node Typeに対応する既定固有設定を生成
	/// @param nodeType Node Type
	/// @return 対応する固有設定
	EffectSequenceNodeSettings MakeDefaultSettings(EffectSequenceNodeType nodeType) {
		switch (nodeType) {
		case EffectSequenceNodeType::PrimitiveEffect: return PrimitiveEffectNodeSettings{};
		case EffectSequenceNodeType::Ribbon: return RibbonNodeSettings{};
		case EffectSequenceNodeType::Beam: return BeamNodeSettings{};
		case EffectSequenceNodeType::Particle:
		case EffectSequenceNodeType::Count:
		default: return ParticleNodeSettings{};
		}
	}

	/// @brief Node Typeと固有設定Variantが一致するか確認
	/// @param node 確認対象Node
	/// @return 一致する場合はtrue
	bool HasMatchingSettings(const EffectSequenceNode& node) {
		switch (node.nodeType) {
		case EffectSequenceNodeType::Particle:
			return std::holds_alternative<ParticleNodeSettings>(node.settings);
		case EffectSequenceNodeType::PrimitiveEffect:
			return std::holds_alternative<PrimitiveEffectNodeSettings>(node.settings);
		case EffectSequenceNodeType::Ribbon:
			return std::holds_alternative<RibbonNodeSettings>(node.settings);
		case EffectSequenceNodeType::Beam:
			return std::holds_alternative<BeamNodeSettings>(node.settings);
		case EffectSequenceNodeType::Count:
		default:
			return false;
		}
	}

} // namespace

namespace MadoEngine::EffectSequence {

	bool EffectSequenceAsset::LoadFromFile(const std::filesystem::path& filePath) {
		JsonValue json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}
		filePath_ = filePath;
		name_ = PathToUtf8String(filePath.stem());
		FromJson(json);
		return true;
	}

	bool EffectSequenceAsset::SaveToFile(
		const std::filesystem::path& filePath,
		bool createBackup) const {
		const std::filesystem::path outputPath = filePath.empty() ? filePath_ : filePath;
		if (outputPath.empty()) {
			Logger::Output("Effect Sequence Assetの保存先が指定されていません。", Logger::Level::Error);
			return false;
		}
		return MadoEngine::Json::JsonFile::Save(outputPath, ToJson(), 4, createBackup);
	}

	void EffectSequenceAsset::FromJson(const nlohmann::json& json) {

		// 読み込み前に既定構成へ戻して欠落Fieldへ前回値を残さない設計
		config_ = EffectSequenceConfig{};
		version_ = ReadUInt(json, "version", kCurrentVersion);
		if (version_ > kCurrentVersion) {
			Logger::Output(
				"未対応のEffect Sequence Asset Versionです: " + std::to_string(version_),
				Logger::Level::Warning
			);
		}

		config_.duration = ReadFloat(json, "duration", config_.duration);
		config_.isLoop = ReadBool(json, "isLoop", config_.isLoop);
		config_.playbackSpeed = ReadFloat(json, "playbackSpeed", config_.playbackSpeed);
		const JsonValue* nodes = FindValue(json, "nodes");
		if (!nodes || !nodes->is_array()) {
			Validate();
			return;
		}

		const std::size_t nodeCount = (std::min)(nodes->size(), kMaximumEffectSequenceNodeCount);
		config_.nodes.reserve(nodeCount);
		for (std::size_t index = 0; index < nodeCount; ++index) {
			const JsonValue& nodeJson = nodes->at(index);
			if (!nodeJson.is_object()) {
				continue;
			}

			EffectSequenceNode node;
			node.nodeId = ReadUInt(nodeJson, "nodeId", 0);
			node.displayName = ReadString(nodeJson, "displayName", "Effect Node");
			node.nodeType = ParseNodeType(ReadString(nodeJson, "nodeType", "invalid"));
			node.effectAssetName = ReadString(nodeJson, "effectAssetName", "");
			node.isEnabled = ReadBool(nodeJson, "isEnabled", true);
			node.startTime = ReadFloat(nodeJson, "startTime", 0.0f);
			node.playbackSpeed = ReadFloat(nodeJson, "playbackSpeed", 1.0f);
			if (const JsonValue* transform = FindValue(nodeJson, "localTransform")) {
				node.localTransform = MadoEngine::Json::JsonSerializer::ToTransform3D(*transform, {});
			}
			if (const JsonValue* parent = FindValue(nodeJson, "parentNodeId")) {
				if (parent->is_number_unsigned()) {
					const uint64_t parentId = parent->get<uint64_t>();
					if (parentId > 0 && parentId <= (std::numeric_limits<uint32_t>::max)()) {
						node.parentNodeId = static_cast<uint32_t>(parentId);
					}
				} else if (parent->is_number_integer()) {
					const int64_t parentId = parent->get<int64_t>();
					if (parentId > 0 && parentId <= (std::numeric_limits<uint32_t>::max)()) {
						node.parentNodeId = static_cast<uint32_t>(parentId);
					}
				}
			}
			if (const JsonValue* layer = FindValue(nodeJson, "renderLayer"); layer && layer->is_string()) {
				node.renderLayer = ParseRenderLayer(layer->get<std::string>());
			}

			node.settings = MakeDefaultSettings(node.nodeType);
			if (const JsonValue* settings = FindValue(nodeJson, "settings")) {
				switch (node.nodeType) {
				case EffectSequenceNodeType::PrimitiveEffect: {
					PrimitiveEffectNodeSettings primitive;
					primitive.kind = ReadString(*settings, "primitiveType", "cylinder") == "cylinder"
						? PrimitiveEffectNodeKind::Cylinder
						: PrimitiveEffectNodeKind::Count;
					node.settings = primitive;
					break;
				}
				case EffectSequenceNodeType::Ribbon: {
					RibbonNodeSettings ribbon;
					ribbon.overrideManualControlPoints = ReadBool(
						*settings,
						"overrideManualControlPoints",
						false
					);
					if (const JsonValue* points = FindValue(*settings, "controlPoints"); points && points->is_array()) {
						const std::size_t pointCount = (std::min)(
							points->size(),
							static_cast<std::size_t>(MadoEngine::Ribbon::kMaximumRibbonPointCount)
						);
						ribbon.controlPoints.reserve(pointCount);
						for (std::size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
							ribbon.controlPoints.push_back(
								MadoEngine::Json::JsonSerializer::ToVector3(points->at(pointIndex), {})
							);
						}
					}
					node.settings = std::move(ribbon);
					break;
				}
				case EffectSequenceNodeType::Beam: {
					BeamNodeSettings beam;
					if (const JsonValue* start = FindValue(*settings, "startPosition")) {
						beam.startPosition = MadoEngine::Json::JsonSerializer::ToVector3(*start, beam.startPosition);
					}
					if (const JsonValue* end = FindValue(*settings, "endPosition")) {
						beam.endPosition = MadoEngine::Json::JsonSerializer::ToVector3(*end, beam.endPosition);
					}
					node.settings = beam;
					break;
				}
				case EffectSequenceNodeType::Particle:
				case EffectSequenceNodeType::Count:
				default:
					break;
				}
			}
			config_.nodes.push_back(std::move(node));
		}
		Validate();
	}

	nlohmann::json EffectSequenceAsset::ToJson() const {
		JsonValue json{
			{ "version", kCurrentVersion },
			{ "duration", config_.duration },
			{ "isLoop", config_.isLoop },
			{ "playbackSpeed", config_.playbackSpeed },
			{ "nodes", JsonValue::array() },
		};
		for (const EffectSequenceNode& node : config_.nodes) {
			JsonValue nodeJson{
				{ "nodeId", node.nodeId },
				{ "displayName", node.displayName },
				{ "nodeType", ToString(node.nodeType) },
				{ "effectAssetName", node.effectAssetName },
				{ "isEnabled", node.isEnabled },
				{ "startTime", node.startTime },
				{ "playbackSpeed", node.playbackSpeed },
				{ "localTransform", MadoEngine::Json::JsonSerializer::ToJson(node.localTransform) },
			};
			nodeJson["parentNodeId"] = node.parentNodeId.has_value()
				? JsonValue(node.parentNodeId.value())
				: JsonValue(nullptr);
			nodeJson["renderLayer"] = node.renderLayer.has_value()
				? JsonValue(MadoEngine::Render::GetRenderLayerName(node.renderLayer.value()))
				: JsonValue(nullptr);

			JsonValue settings = JsonValue::object();
			if (const auto* primitive = std::get_if<PrimitiveEffectNodeSettings>(&node.settings)) {
				settings["primitiveType"] = ToString(primitive->kind);
			} else if (const auto* ribbon = std::get_if<RibbonNodeSettings>(&node.settings)) {
				settings["overrideManualControlPoints"] = ribbon->overrideManualControlPoints;
				settings["controlPoints"] = JsonValue::array();
				for (const Vector3& point : ribbon->controlPoints) {
					settings["controlPoints"].push_back(MadoEngine::Json::JsonSerializer::ToJson(point));
				}
			} else if (const auto* beam = std::get_if<BeamNodeSettings>(&node.settings)) {
				settings["startPosition"] = MadoEngine::Json::JsonSerializer::ToJson(beam->startPosition);
				settings["endPosition"] = MadoEngine::Json::JsonSerializer::ToJson(beam->endPosition);
			}
			nodeJson["settings"] = std::move(settings);
			json["nodes"].push_back(std::move(nodeJson));
		}
		return json;
	}

	uint32_t EffectSequenceAsset::GenerateNodeId() const {
		std::unordered_set<uint32_t> usedIds;
		for (const EffectSequenceNode& node : config_.nodes) {
			usedIds.insert(node.nodeId);
		}
		for (uint32_t candidate = 1; candidate != 0; ++candidate) {
			if (!usedIds.contains(candidate)) {
				return candidate;
			}
		}
		return 0;
	}

	void EffectSequenceAsset::Validate() {

		// 再生時間とNode数をRuntime上限へ制限して極端な入力を排除
		version_ = kCurrentVersion;
		config_.duration = std::clamp(
			std::isfinite(config_.duration) ? config_.duration : 1.0f,
			kMinimumEffectSequenceDuration,
			kMaximumEffectSequenceDuration
		);
		config_.playbackSpeed = std::clamp(
			std::isfinite(config_.playbackSpeed) ? config_.playbackSpeed : 1.0f,
			kMinimumEffectSequencePlaybackSpeed,
			kMaximumEffectSequencePlaybackSpeed
		);
		if (config_.nodes.size() > kMaximumEffectSequenceNodeCount) {
			config_.nodes.resize(kMaximumEffectSequenceNodeCount);
		}

		std::unordered_set<uint32_t> usedIds;

		// 0または重複IDを再採番してNode単位の発火管理を一意化
		for (EffectSequenceNode& node : config_.nodes) {
			if (node.nodeId == 0 || usedIds.contains(node.nodeId)) {
				uint32_t candidateId = 1;
				while (usedIds.contains(candidateId)) {
					++candidateId;
				}
				node.nodeId = candidateId;
			}
			usedIds.insert(node.nodeId);

			if (node.nodeType >= EffectSequenceNodeType::Count) {
				node.nodeType = EffectSequenceNodeType::Particle;
				node.isEnabled = false;
			}
			if (!HasMatchingSettings(node)) {
				node.settings = MakeDefaultSettings(node.nodeType);
			}
			if (node.displayName.empty()) {
				node.displayName = "Node " + std::to_string(node.nodeId);
			}
			if (node.effectAssetName.empty()) {
				node.isEnabled = false;
			}
			node.startTime = std::clamp(
				std::isfinite(node.startTime) ? node.startTime : 0.0f,
				0.0f,
				config_.duration
			);
			node.playbackSpeed = std::clamp(
				std::isfinite(node.playbackSpeed) ? node.playbackSpeed : 1.0f,
				kMinimumEffectSequencePlaybackSpeed,
				kMaximumEffectSequencePlaybackSpeed
			);
			NormalizeTransform(node.localTransform);
			if (node.renderLayer.has_value() && !MadoEngine::Render::IsValidRenderLayer(node.renderLayer.value())) {
				node.renderLayer.reset();
			}

			if (auto* primitive = std::get_if<PrimitiveEffectNodeSettings>(&node.settings)) {
				if (primitive->kind >= PrimitiveEffectNodeKind::Count) {
					primitive->kind = PrimitiveEffectNodeKind::Cylinder;
					node.isEnabled = false;
				}
			} else if (auto* ribbon = std::get_if<RibbonNodeSettings>(&node.settings)) {
				if (ribbon->controlPoints.size() > MadoEngine::Ribbon::kMaximumRibbonPointCount) {
					ribbon->controlPoints.resize(MadoEngine::Ribbon::kMaximumRibbonPointCount);
				}
				for (Vector3& point : ribbon->controlPoints) {
					NormalizeVector3(point, {}, -1000000.0f, 1000000.0f);
				}
			} else if (auto* beam = std::get_if<BeamNodeSettings>(&node.settings)) {
				NormalizeVector3(beam->startPosition, {}, -1000000.0f, 1000000.0f);
				NormalizeVector3(beam->endPosition, { 0.0f, 1.0f, 0.0f }, -1000000.0f, 1000000.0f);
			}
		}

		std::unordered_map<uint32_t, std::size_t> nodeIndices;
		for (std::size_t index = 0; index < config_.nodes.size(); ++index) {
			nodeIndices[config_.nodes[index].nodeId] = index;
		}
		for (EffectSequenceNode& node : config_.nodes) {
			if (
				node.parentNodeId.has_value() &&
				(node.parentNodeId.value() == node.nodeId || !nodeIndices.contains(node.parentNodeId.value()))) {
				node.parentNodeId.reset();
			}
		}

		std::vector<uint8_t> visitStates(config_.nodes.size(), 0);
		std::function<void(std::size_t)> validateParent = [&](std::size_t nodeIndex) {
			if (visitStates[nodeIndex] == 2) {
				return;
			}
			visitStates[nodeIndex] = 1;
			EffectSequenceNode& node = config_.nodes[nodeIndex];
			if (node.parentNodeId.has_value()) {
				const std::size_t parentIndex = nodeIndices.at(node.parentNodeId.value());
				if (visitStates[parentIndex] == 1) {
					node.parentNodeId.reset();
				} else {
					validateParent(parentIndex);
				}
			}
			visitStates[nodeIndex] = 2;
		};
		for (std::size_t index = 0; index < config_.nodes.size(); ++index) {
			validateParent(index);
		}
	}

} // namespace MadoEngine::EffectSequence
