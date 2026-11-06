#include "PostEffectSettings.h"
#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include "Utility/Json/JsonHeaders.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace MadoEngine::Editor {

namespace Detail {

const char* GetLayerEffectStageName(Render::LayerEffectStage stage) {
	switch (stage) {
	case Render::LayerEffectStage::Transparent:
		return "Transparent";
	case Render::LayerEffectStage::Overlay:
		return "Overlay";
	case Render::LayerEffectStage::Scene:
	default:
		return "Scene";
	}
}

const char* GetScreenEffectStageName(Render::ScreenEffectStage stage) {
	switch (stage) {
	case Render::ScreenEffectStage::Scene:
		return "Scene";
	case Render::ScreenEffectStage::Final:
	default:
		return "Final";
	}
}

const Render::PostEffectDefinition* FindPostEffectDefinition(const Render::PostEffectPass& pass) {
	const auto effectType = pass.GetPostEffectType();
	if (effectType) {
		return Render::PostEffectDefinitionRegistry::Find(*effectType);
	} else {
		return nullptr;
	}
}

std::filesystem::path CreateBackupJsonPath(const std::filesystem::path& filePath) {
	std::filesystem::path backupPath = filePath;
	backupPath += ".bak";
	return backupPath;
}

} // namespace Detail

namespace {


/// @brief 文字列からレイヤーポストエフェクトの適用段階を取得
/// @param value 変換する適用段階名
/// @return 変換した適用段階、未定義名の場合はScene
Render::LayerEffectStage LayerEffectStageFromString(const std::string& value) {
	if (value == "Transparent") {
		return Render::LayerEffectStage::Transparent;
	}
	if (value == "Overlay") {
		return Render::LayerEffectStage::Overlay;
	}

	return Render::LayerEffectStage::Scene;
}

/// @brief 文字列からフルスクリーンポストエフェクトの適用段階を取得
/// @param value 変換する適用段階名
/// @return 変換した適用段階、未定義名の場合はFinal
Render::ScreenEffectStage ScreenEffectStageFromString(const std::string& value) {
	if (value == "Scene") {
		return Render::ScreenEffectStage::Scene;
	} else {
		return Render::ScreenEffectStage::Final;
	}
}

/// @brief Passの適用先名を取得
/// @param scope 名前を取得する適用先
/// @return JSONへ保存する適用先名
const char* GetPostEffectPassScopeName(Render::PostEffectPassScope scope) {
	if (scope == Render::PostEffectPassScope::Screen) {
		return "Screen";
	} else {
		return "Layer";
	}
}

/// @brief JSONのEffect情報をRegistry定義へ解決
/// @param passJson Pass設定を保持したJSON
/// @return 解決したEffect定義、未定義の場合はCopyImage
const Render::PostEffectDefinition& ResolvePostEffectDefinition(const nlohmann::json& passJson) {
	const auto effectTypeIt = passJson.find("effectType");
	if (effectTypeIt != passJson.end() && effectTypeIt->is_string()) {
		if (const Render::PostEffectDefinition* definition =
			Render::PostEffectDefinitionRegistry::FindByTypeName(effectTypeIt->get<std::string>())) {
			return *definition;
		}
	}

	const std::string shaderKey = passJson.value(
		"effectShaderKey",
		std::string("PostEffect/CopyImage.PS")
	);
	if (const Render::PostEffectDefinition* definition =
		Render::PostEffectDefinitionRegistry::FindByShaderKey(shaderKey)) {
		return *definition;
	}

	Logger::Output("JSON内の未登録ポストエフェクトをCopyImageとして読み込みます: " + shaderKey, Logger::Level::Warning);
	return *Render::PostEffectDefinitionRegistry::Find(Render::PostEffectType::CopyImage);
}

/// @brief JSONから読み込んだパラメータ値をPassへ適用
/// @param manager 適用先Passを管理するManager
/// @param handle 適用先PassのHandle
/// @param parameters パラメータ値を保持したJSON
void ApplyPostEffectParameters(
	Render::PostEffectManager& manager,
	Render::PostEffectPassHandle handle,
	const nlohmann::json& parameters) {
	if (!parameters.is_object()) {
		return;
	}

	const Render::PostEffectPass* pass = manager.TryGet(handle);
	if (!pass) {
		return;
	}

	const Render::PostEffectDefinition* definition = Detail::FindPostEffectDefinition(*pass);
	if (!definition) {
		return;
	}

	for (const Render::PostEffectFloatParameterDefinition& parameter : definition->GetParameters()) {
		const auto parameterIt = parameters.find(std::string(parameter.key));
		if (parameterIt != parameters.end() && parameterIt->is_number()) {
			manager.SetFloatParameter(handle, std::string(parameter.key), parameterIt->get<float>());
		}
	}
}

/// @brief 既存Passを更新するか新規生成してJSON設定を反映
/// @param manager 更新対象のManager
/// @param passJson Pass設定を保持したJSON
/// @param scope 読み込むPassの適用先
/// @param index 既定keyへ使用する配列index
/// @return 更新または生成したPassのHandle
Render::PostEffectPassHandle UpsertPostEffectPassFromJson(
	Render::PostEffectManager& manager,
	const nlohmann::json& passJson,
	Render::PostEffectPassScope scope,
	std::size_t index) {
	if (!passJson.is_object()) {
		return {};
	}

	const bool isScreen = scope == Render::PostEffectPassScope::Screen;
	const std::string defaultKey =
		([&]() {
			if (isScreen) {
				return ("ScreenEffectPass_Loaded_");
			} else {
				return ("LayerEffectPass_Loaded_");
			}
		}()) + std::to_string(index + 1);
	const std::string defaultName =
		([&]() {
			if (isScreen) {
				return ("Screen Effect Loaded ");
			} else {
				return ("Layer Effect Loaded ");
			}
		}()) + std::to_string(index + 1);
	std::string key = passJson.value("key", defaultKey);
	std::string name = passJson.value("name", defaultName);
	if (key.empty()) {
		key = defaultKey;
	}
	if (name.empty()) {
		name = defaultName;
	}

	const Render::PostEffectDefinition& definition = ResolvePostEffectDefinition(passJson);
	Render::PostEffectPassHandle handle = manager.Find(key);
	bool wasCreated = false;
	if (handle.IsValid() && manager.GetScope(handle) != scope) {

		// Json読み込みを前FrameのGPU完了待機後かつ描画開始前だけに限定
		manager.Destroy(handle);
		handle = {};
	}

	if (!handle.IsValid()) {
		if (scope == Render::PostEffectPassScope::Layer) {
			Render::LayerPostEffectPassCreateDesc desc{};
			desc.key = key;
			desc.name = name;
			desc.targetLayerMask = passJson.value(
				"targetLayerMask",
				Render::ToRenderLayerMask(Render::RenderLayer::Default)
			);
			desc.layerEffectStage = LayerEffectStageFromString(
				passJson.value("layerEffectStage", std::string("Scene"))
			);
			desc.effectShaderKey = std::string(definition.shaderKey);
			desc.enabled = passJson.value("enabled", true);
			desc.ignoreDepthForMask = passJson.value("ignoreDepthForMask", false);
			handle = manager.CreateLayerPass(desc);
		} else {
			Render::ScreenPostEffectPassCreateDesc desc{};
			desc.key = key;
			desc.name = name;
			desc.screenEffectStage = ScreenEffectStageFromString(
				passJson.value("screenEffectStage", std::string("Final"))
			);
			desc.effectShaderKey = std::string(definition.shaderKey);
			desc.enabled = passJson.value("enabled", true);
			handle = manager.CreateScreenPass(desc);
		}
		wasCreated = handle.IsValid();
	}

	Render::PostEffectPass* pass = manager.TryGet(handle);
	if (!pass) {
		return {};
	}

	pass->SetName(name);
	pass->SetEnabled(passJson.value("enabled", true));
	if (scope == Render::PostEffectPassScope::Layer) {
		pass->SetTargetLayerMask(passJson.value(
			"targetLayerMask",
			Render::ToRenderLayerMask(Render::RenderLayer::Default)
		));
		pass->SetLayerEffectStage(LayerEffectStageFromString(
			passJson.value("layerEffectStage", std::string("Scene"))
		));
		pass->SetIgnoreDepthForMask(passJson.value("ignoreDepthForMask", false));
	} else {
		pass->SetScreenEffectStage(ScreenEffectStageFromString(
			passJson.value("screenEffectStage", std::string("Final"))
		));
	}

	if (!wasCreated) {

		// 既存Handleを維持しつつ欠落ParameterをRegistry既定値へ戻す旧形式互換
		manager.SetEffectType(handle, definition.type);
	}

	const auto parametersIt = passJson.find("parameters");
	if (parametersIt != passJson.end()) {
		ApplyPostEffectParameters(manager, handle, *parametersIt);
	}
	return handle;
}

/// @brief JSONのPass配列を差分更新して希望順序を取得
/// @param manager 更新対象のManager
/// @param passList Pass配列を保持したJSON
/// @param scope 読み込むPassの適用先
/// @param usedKeys 全scopeを通した重複検査と削除判定に使うkey集合
/// @return JSONに記録された有効なHandle順序
std::vector<Render::PostEffectPassHandle> LoadPostEffectPassListFromJson(
	Render::PostEffectManager& manager,
	const nlohmann::json& passList,
	Render::PostEffectPassScope scope,
	std::unordered_set<std::string>& usedKeys) {
	std::vector<Render::PostEffectPassHandle> order;
	if (!passList.is_array()) {
		return order;
	}

	order.reserve(passList.size());
	for (std::size_t index = 0; index < passList.size(); ++index) {
		const nlohmann::json& passJson = passList[index];
		if (!passJson.is_object()) {
			continue;
		}

		const bool isScreen = scope == Render::PostEffectPassScope::Screen;
		const std::string defaultKey =
			([&]() {
				if (isScreen) {
					return ("ScreenEffectPass_Loaded_");
				} else {
					return ("LayerEffectPass_Loaded_");
				}
			}()) + std::to_string(index + 1);
		std::string key = passJson.value("key", defaultKey);
		if (key.empty()) {
			key = defaultKey;
		}
		if (!usedKeys.emplace(key).second) {
			Logger::Output("JSON内でポストエフェクトPassの内部キーが重複しています: " + key, Logger::Level::Warning);
			continue;
		}

		const Render::PostEffectPassHandle handle =
			UpsertPostEffectPassFromJson(manager, passJson, scope, index);
		if (handle.IsValid()) {
			order.push_back(handle);
		}
	}
	return order;
}

/// @brief JSONの順序をManagerの実行順へ反映
/// @param manager 更新対象のManager
/// @param scope 反映するPassの適用先
/// @param order JSONから読み込んだHandle順序
void ApplyPostEffectPassOrder(
	Render::PostEffectManager& manager,
	Render::PostEffectPassScope scope,
	const std::vector<Render::PostEffectPassHandle>& order) {
	for (std::size_t index = 0; index < order.size(); ++index) {
		if (scope == Render::PostEffectPassScope::Layer) {
			manager.MoveLayerPass(order[index], index);
		} else {
			manager.MoveScreenPass(order[index], index);
		}
	}
}

/// @brief PostEffect Editorの状態をJSONから差分読み込み
/// @param manager 読み込み先のManager
/// @param root 読み込むPostEffect設定Json
/// @return 読み込みに成功した場合はtrue
bool RestorePostEffectEditorJsonInternal(
	Render::PostEffectManager& manager,
	const nlohmann::json& root) {
	if (!root.is_object()) {
		return false;
	}

	std::unordered_set<std::string> usedKeys;
	const std::vector<Render::PostEffectPassHandle> layerOrder = LoadPostEffectPassListFromJson(
		manager,
		root.value("layerPasses", nlohmann::json::array()),
		Render::PostEffectPassScope::Layer,
		usedKeys
	);
	const std::vector<Render::PostEffectPassHandle> screenOrder = LoadPostEffectPassListFromJson(
		manager,
		root.value("screenPasses", nlohmann::json::array()),
		Render::PostEffectPassScope::Screen,
		usedKeys
	);

	std::vector<Render::PostEffectPassHandle> existingHandles = manager.GetLayerPassHandles();
	existingHandles.insert(
		existingHandles.end(),
		manager.GetScreenPassHandles().begin(),
		manager.GetScreenPassHandles().end()
	);
	for (Render::PostEffectPassHandle handle : existingHandles) {
		const Render::PostEffectPass* pass = manager.TryGet(handle);
		if (pass && !usedKeys.contains(pass->GetKey())) {

			// Snapshotに存在しないPassを遅延破棄して描画中Resourceの寿命を維持
			manager.Destroy(handle);
		}
	}

	ApplyPostEffectPassOrder(manager, Render::PostEffectPassScope::Layer, layerOrder);
	ApplyPostEffectPassOrder(manager, Render::PostEffectPassScope::Screen, screenOrder);
	return true;
}

/// @brief PostEffect Editorの状態をJSONファイルから差分読み込み
/// @param manager 読み込み先のManager
/// @param filePath 読み込むJSONパス
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJsonInternal(
	Render::PostEffectManager& manager,
	const std::filesystem::path& filePath = kPostEffectEditorJsonPath) {
	nlohmann::json root;
	if (!Json::JsonFile::Load(filePath, root)) {
		return false;
	}
	return RestorePostEffectEditorJsonInternal(manager, root);
}

/// @brief PassのParameterをJSONへ変換
/// @param pass 保存対象のPass
/// @return パラメータkeyと値を保持したJSON
nlohmann::json SerializePostEffectParameters(const Render::PostEffectPass& pass) {
	nlohmann::json parameters = nlohmann::json::object();
	const Render::PostEffectDefinition* definition = Detail::FindPostEffectDefinition(pass);
	if (!definition) {
		return parameters;
	}

	for (const Render::PostEffectFloatParameterDefinition& parameter : definition->GetParameters()) {
		float value = 0.0f;
		if (pass.TryGetFloatParameter(parameter.offset, value)) {
			parameters[std::string(parameter.key)] = value;
		}
	}
	return parameters;
}

/// @brief PassのEditor設定をJSONへ変換
/// @param pass 保存対象のPass
/// @param scope Passの適用先
/// @return Pass設定を保持したJSON
nlohmann::json SerializePostEffectPass(
	const Render::PostEffectPass& pass,
	Render::PostEffectPassScope scope) {
	nlohmann::json passJson;
	passJson["key"] = pass.GetKey();
	passJson["name"] = pass.GetName();
	passJson["scope"] = GetPostEffectPassScopeName(scope);
	passJson["enabled"] = pass.IsEnabled();
	passJson["effectShaderKey"] = pass.GetEffectShaderKey();
	if (const Render::PostEffectDefinition* definition = Detail::FindPostEffectDefinition(pass)) {
		passJson["effectType"] = definition->typeName;
	}
	if (scope == Render::PostEffectPassScope::Layer) {
		passJson["targetLayerMask"] = pass.GetTargetLayerMask();
		passJson["layerEffectStage"] = Detail::GetLayerEffectStageName(pass.GetLayerEffectStage());
		passJson["ignoreDepthForMask"] = pass.IsIgnoreDepthForMask();
	} else {
		passJson["screenEffectStage"] = Detail::GetScreenEffectStageName(pass.GetScreenEffectStage());
	}
	passJson["parameters"] = SerializePostEffectParameters(pass);
	return passJson;
}

/// @brief Handle順序のPass一覧をJSONへ変換
/// @param manager 保存対象のManager
/// @param handles 保存するHandle順序
/// @param scope Passの適用先
/// @return Pass一覧を保持したJSON
nlohmann::json SerializePostEffectPassList(
	const Render::PostEffectManager& manager,
	const std::vector<Render::PostEffectPassHandle>& handles,
	Render::PostEffectPassScope scope) {
	nlohmann::json list = nlohmann::json::array();
	for (Render::PostEffectPassHandle handle : handles) {
		if (const Render::PostEffectPass* pass = manager.TryGet(handle)) {
			list.push_back(SerializePostEffectPass(*pass, scope));
		}
	}
	return list;
}

/// @brief PostEffect Editorの状態をJSONへ保存
/// @param manager 保存対象のManager
/// @return 保存に成功した場合はtrue
bool SavePostEffectEditorJsonInternal(const Render::PostEffectManager& manager) {
	nlohmann::json root;
	root["version"] = 4;
	root["layerPasses"] = SerializePostEffectPassList(
		manager,
		manager.GetLayerPassHandles(),
		Render::PostEffectPassScope::Layer
	);
	root["screenPasses"] = SerializePostEffectPassList(
		manager,
		manager.GetScreenPassHandles(),
		Render::PostEffectPassScope::Screen
	);
	return Json::JsonFile::Save(kPostEffectEditorJsonPath, root, 4, true);
}

} // namespace

std::string CapturePostEffectEditorState(const Render::PostEffectManager& postEffectManager) {
	nlohmann::json root;
	root["version"] = 4;
	root["layerPasses"] = SerializePostEffectPassList(
		postEffectManager,
		postEffectManager.GetLayerPassHandles(),
		Render::PostEffectPassScope::Layer
	);
	root["screenPasses"] = SerializePostEffectPassList(
		postEffectManager,
		postEffectManager.GetScreenPassHandles(),
		Render::PostEffectPassScope::Screen
	);
	return root.dump();
}

bool RestorePostEffectEditorState(
	Render::PostEffectManager& postEffectManager,
	const std::string& snapshot) {
	const nlohmann::json root = nlohmann::json::parse(snapshot, nullptr, false);
	return !root.is_discarded() && RestorePostEffectEditorJsonInternal(postEffectManager, root);
}

bool SavePostEffectEditorJsonToFile(const Render::PostEffectManager& postEffectManager) {
	return SavePostEffectEditorJsonInternal(postEffectManager);
}

bool LoadPostEffectEditorJson(Render::PostEffectManager& postEffectManager) {
	return LoadPostEffectEditorJsonInternal(postEffectManager);
}

bool LoadPostEffectEditorJsonFromFile(Render::PostEffectManager& postEffectManager) {
	return LoadPostEffectEditorJson(postEffectManager);
}

bool LoadPostEffectEditorJsonFromFile(
	Render::PostEffectManager& postEffectManager,
	const std::filesystem::path& filePath) {
	return LoadPostEffectEditorJsonInternal(postEffectManager, filePath);
}

} // namespace MadoEngine::Editor
