#include "ParameterSerializer.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <cmath>
#include <limits>
#include <set>
#include <type_traits>

namespace {

using MadoEngine::ParameterValue;

/// @brief UTF-8のルート名から保存ファイルのパスを構築
/// @param directory 保存先ディレクトリ
/// @param root ルートグループ名
/// @return 日本語名を保持したJSONファイルパス
std::filesystem::path GetRootFilePath(const std::filesystem::path& directory, const std::string& root) {
	std::u8string name(root.begin(), root.end());
	name += u8".json";
	return directory / std::filesystem::path(name);
}

/// @brief JSON数値を有限なfloatとして検証
/// @param json 検証するJSON値
/// @param value 読み込んだ値の出力先
/// @return floatの表現範囲内の場合はtrue
bool ReadFloat(const nlohmann::json& json, float& value) {
	if (!json.is_number()) {
		return false;
	}
	const double number = json.get<double>();
	if (!std::isfinite(number) || std::abs(number) > static_cast<double>((std::numeric_limits<float>::max)())) {
		return false;
	}
	value = static_cast<float>(number);
	return true;
}

/// @brief 型名と値の組からパラメータ値を厳密に復元
/// @param json 型名と値を含むJSON
/// @param value 復元値の出力先
/// @return 型、成分数、数値範囲が有効な場合はtrue
bool ReadValue(const nlohmann::json& json, ParameterValue& value) {
	if (!json.is_object() || !json.contains("type") || !json["type"].is_string() || !json.contains("value")) {
		return false;
	}
	const std::string type = json["type"].get<std::string>();
	const auto& source = json["value"];
	if (type == "enum") {
		if (!source.is_string() || !json.contains("enumType") || !json["enumType"].is_string()) {
			return false;
		}
		value = MadoEngine::ParameterEnumValue{ json["enumType"].get<std::string>(), source.get<std::string>() };
		return true;
	}
	if (type.ends_with("Range")) {
		const std::string componentType = type.substr(0, type.size() - 5);
		if (componentType == "bool" || componentType.ends_with("Range") || !source.is_array() || source.size() != 2) {
			return false;
		}
		ParameterValue minimum;
		ParameterValue maximum;
		if (!ReadValue({ { "type", componentType }, { "value", source[0] } }, minimum) ||
			!ReadValue({ { "type", componentType }, { "value", source[1] } }, maximum)) {
			return false;
		}

		// 両端を同じ型で読み込み、一つの設定値としてStoreへ引き渡し
		return std::visit([&maximum, &value](const auto& typedMinimum) {
			using T = std::decay_t<decltype(typedMinimum)>;
			if constexpr (!std::is_same_v<T, bool> && !MadoEngine::kIsParameterRange<T> &&
				!std::is_same_v<T, MadoEngine::ParameterEnumValue>) {
				const T* typedMaximum = std::get_if<T>(&maximum);
				if (!typedMaximum) {
					return false;
				}
				value = MadoEngine::ParameterRange<T>{ typedMinimum, *typedMaximum };
				return true;
			} else {
				return false;
			}
		}, minimum);
	}
	if (type == "int32_t" || type == "uint32_t") {
		if (!source.is_number_integer()) {
			return false;
		}

		// 符号を確認してから変換し、大きなunsigned値の巻き戻りを防止
		if (source.is_number_unsigned()) {
			const std::uint64_t number = source.get<std::uint64_t>();
			if (type == "int32_t" && number <= static_cast<std::uint64_t>((std::numeric_limits<std::int32_t>::max)())) {
				value = static_cast<std::int32_t>(number);
				return true;
			}
			if (type == "uint32_t" && number <= (std::numeric_limits<std::uint32_t>::max)()) {
				value = static_cast<std::uint32_t>(number);
				return true;
			}
		} else {
			const std::int64_t number = source.get<std::int64_t>();
			if (type == "int32_t" && number >= (std::numeric_limits<std::int32_t>::min)() && number <= (std::numeric_limits<std::int32_t>::max)()) {
				value = static_cast<std::int32_t>(number);
				return true;
			}
			if (type == "uint32_t" && number >= 0 && number <= (std::numeric_limits<std::uint32_t>::max)()) {
				value = static_cast<std::uint32_t>(number);
				return true;
			}
		}
		return false;
	}
	if (type == "bool") {
		if (!source.is_boolean()) {
			return false;
		}
		value = source.get<bool>();
		return true;
	}
	if (type == "float") {
		float number = 0.0f;
		if (!ReadFloat(source, number)) {
			return false;
		}
		value = number;
		return true;
	}
	std::size_t components = 0;
	if (type == "Vector2") {
		components = 2;
	} else if (type == "Vector3") {
		components = 3;
	} else if (type == "Vector4") {
		components = 4;
	} else {
		return false;
	}
	if (!source.is_array() || source.size() != components) {
		return false;
	}
	float numbers[4] = {};
	for (std::size_t index = 0; index < components; ++index) {
		if (!ReadFloat(source[index], numbers[index])) {
			return false;
		}
	}
	if (components == 2) {
		value = Vector2{ numbers[0], numbers[1] };
	} else if (components == 3) {
		value = Vector3{ numbers[0], numbers[1], numbers[2] };
	} else {
		value = Vector4{ numbers[0], numbers[1], numbers[2], numbers[3] };
	}
	return true;
}

/// @brief パラメータ値を型情報付きJSONへ変換
/// @param value 変換する値
/// @return 型名と値を持つJSON
nlohmann::json WriteValue(const ParameterValue& value) {
	return std::visit([](const auto& typedValue) -> nlohmann::json {
		using T = std::decay_t<decltype(typedValue)>;
		if constexpr (MadoEngine::kIsParameterRange<T>) {

			// 表示属性を保存せず、端点の型と保存単位だけで範囲を表現
			const nlohmann::json minimum = WriteValue(ParameterValue{ typedValue.minimum });
			const nlohmann::json maximum = WriteValue(ParameterValue{ typedValue.maximum });
			return { { "type", minimum["type"].get<std::string>() + "Range" },
				{ "value", nlohmann::json::array({ minimum["value"], maximum["value"] }) } };
		} else if constexpr (std::is_same_v<T, MadoEngine::ParameterEnumValue>) {
			return { { "type", "enum" }, { "enumType", typedValue.type }, { "value", typedValue.name } };
		} else if constexpr (std::is_same_v<T, std::int32_t>) {
			return { { "type", "int32_t" }, { "value", typedValue } };
		} else if constexpr (std::is_same_v<T, std::uint32_t>) {
			return { { "type", "uint32_t" }, { "value", typedValue } };
		} else if constexpr (std::is_same_v<T, float>) {
			return { { "type", "float" }, { "value", typedValue } };
		} else if constexpr (std::is_same_v<T, bool>) {
			return { { "type", "bool" }, { "value", typedValue } };
		} else if constexpr (std::is_same_v<T, Vector2>) {
			return { { "type", "Vector2" }, { "value", MadoEngine::Json::JsonSerializer::ToJson(typedValue) } };
		} else if constexpr (std::is_same_v<T, Vector3>) {
			return { { "type", "Vector3" }, { "value", MadoEngine::Json::JsonSerializer::ToJson(typedValue) } };
		} else {
			return { { "type", "Vector4" }, { "value", MadoEngine::Json::JsonSerializer::ToJson(typedValue) } };
		}
	}, value);
}

}

namespace MadoEngine {

bool ParameterSerializer::SaveAll(const ParameterStore& store, const std::filesystem::path& directory) {
	try {
		bool succeeded = true;
		for (const std::string& root : store.GetRoots()) {
			const auto path = GetRootFilePath(directory, root);
			succeeded = Json::JsonFile::Save(path, ToJson(store, root), 4, true) && succeeded;
		}
		return succeeded;
	} catch (const std::exception& exception) {

		// パス変換やJSON生成の例外もUIへ伝播させず保存失敗として通知
		Logger::Output("調整パラメータの保存中に例外が発生しました: " + std::string(exception.what()), Logger::Level::Error);
		return false;
	}
}

bool ParameterSerializer::LoadAll(ParameterStore& store, const std::filesystem::path& directory) {
	try {
		std::set<std::string> roots;
		for (const std::string& root : store.GetRoots()) {
			roots.insert(root);
		}
		if (std::filesystem::exists(directory)) {
			for (const auto& file : std::filesystem::directory_iterator(directory)) {
				if (file.is_regular_file() && file.path().extension() == ".json") {
					const auto name = file.path().stem().u8string();
					roots.insert(std::string(reinterpret_cast<const char*>(name.data()), name.size()));
				}
			}
		}
		bool succeeded = true;
		for (const std::string& root : roots) {
			if (!ParameterStore::IsValidGroup(root) || ParameterStore::GetRoot(root) != root) {
				succeeded = false;
				continue;
			}
			const auto path = GetRootFilePath(directory, root);
			if (!Json::JsonFile::Exists(path)) {

				// 未保存のルートはコード側の既定値で補完し、初回起動を許容
				store.ResetRoot(root);
				continue;
			}
			nlohmann::json json;
			if (!Json::JsonFile::Load(path, json) || !FromJson(store, root, json)) {
				Logger::Output("調整パラメータの読み込みに失敗しました: " + root, Logger::Level::Warning);
				succeeded = false;
			}
		}
		return succeeded;
	} catch (const std::exception& exception) {
		Logger::Output("調整パラメータの読み込み中に例外が発生しました: " + std::string(exception.what()), Logger::Level::Error);
		return false;
	}
}

nlohmann::json ParameterSerializer::ToJson(const ParameterStore& store, const std::string& root) {
	nlohmann::json groups = nlohmann::json::object();
	for (const auto& [key, entry] : store.GetEntries()) {
		if (ParameterStore::GetRoot(key.group) == root) {
			groups[key.group][key.name] = WriteValue(entry.value);
		}
	}
	return { { "version", 1 }, { "root", root }, { "groups", groups } };
}

bool ParameterSerializer::FromJson(ParameterStore& store, const std::string& root, const nlohmann::json& json) {
	try {
		if (!json.is_object() || !json.contains("version") || !json["version"].is_number_integer() || json["version"] != 1 ||
			!json.contains("root") || !json["root"].is_string() || json["root"] != root ||
			!json.contains("groups") || !json["groups"].is_object()) {
			return false;
		}
		std::map<ParameterKey, ParameterValue> values;
		for (const auto& [group, items] : json["groups"].items()) {
			if (!ParameterStore::IsValidGroup(group) || ParameterStore::GetRoot(group) != root || !items.is_object()) {
				return false;
			}
			for (const auto& [name, source] : items.items()) {
				ParameterValue value;
				if (name.empty() || !ReadValue(source, value)) {
					return false;
				}
				values.emplace(ParameterKey{ group, name }, value);
			}
		}

		// 欠損項目の既定値補完と型整合性の検証はStore側へ集約
		return store.RestoreRoot(root, values);
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

std::string ParameterSerializer::CaptureState(const ParameterStore& store) {
	nlohmann::json json = nlohmann::json::object();
	for (const std::string& root : store.GetRoots()) {
		json[root] = ToJson(store, root);
	}
	return json.dump();
}

bool ParameterSerializer::RestoreState(ParameterStore& store, const std::string& snapshot) {
	const nlohmann::json json = nlohmann::json::parse(snapshot, nullptr, false);
	if (!json.is_object()) {
		return false;
	}
	bool succeeded = true;
	for (const auto& [root, source] : json.items()) {
		succeeded = FromJson(store, root, source) && succeeded;
	}
	return succeeded;
}

}
