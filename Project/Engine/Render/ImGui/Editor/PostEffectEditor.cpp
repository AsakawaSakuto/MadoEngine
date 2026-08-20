#include "PostEffectEditor.h"
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

namespace {

// 既存設定との後方互換性を維持する固定Json File名
const std::filesystem::path kPostEffectEditorJsonPath = "Assets/Json/LayerEffectPassEditor.json";

/// @brief レイヤーポストエフェクトの適用段階名を取得
/// @param stage 名前を取得する適用段階
/// @return 適用段階名
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

/// @brief フルスクリーンポストエフェクトの適用段階名を取得
/// @param stage 名前を取得する適用段階
/// @return 適用段階名
const char* GetScreenEffectStageName(Render::ScreenEffectStage stage) {
	switch (stage) {
	case Render::ScreenEffectStage::Scene:
		return "Scene";
	case Render::ScreenEffectStage::Final:
	default:
		return "Final";
	}
}

/// @brief 文字列からフルスクリーンポストエフェクトの適用段階を取得
/// @param value 変換する適用段階名
/// @return 変換した適用段階、未定義名の場合はFinal
Render::ScreenEffectStage ScreenEffectStageFromString(const std::string& value) {
	return value == "Scene" ? Render::ScreenEffectStage::Scene : Render::ScreenEffectStage::Final;
}

/// @brief Passの適用先名を取得
/// @param scope 名前を取得する適用先
/// @return JSONへ保存する適用先名
const char* GetPostEffectPassScopeName(Render::PostEffectPassScope scope) {
	return scope == Render::PostEffectPassScope::Screen ? "Screen" : "Layer";
}

/// @brief Passに設定された登録済みポストエフェクト定義を取得
/// @param pass 検索対象のPass
/// @return 登録済み定義、未登録Shaderの場合はnullptr
const Render::PostEffectDefinition* FindPostEffectDefinition(const Render::PostEffectPass& pass) {
	const auto effectType = pass.GetPostEffectType();
	return effectType ? Render::PostEffectDefinitionRegistry::Find(*effectType) : nullptr;
}

/// @brief 指定したJSONパスのバックアップパスを作成
/// @param filePath バックアップ元のJSONパス
/// @return .bakを付けたバックアップJSONパス
std::filesystem::path CreateBackupJsonPath(const std::filesystem::path& filePath) {
	std::filesystem::path backupPath = filePath;
	backupPath += ".bak";
	return backupPath;
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
	const nlohmann::json& parameters)
{
	if (!parameters.is_object()) {
		return;
	}

	const Render::PostEffectPass* pass = manager.TryGet(handle);
	if (!pass) {
		return;
	}

	const Render::PostEffectDefinition* definition = FindPostEffectDefinition(*pass);
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
	std::size_t index)
{
	if (!passJson.is_object()) {
		return {};
	}

	const bool isScreen = scope == Render::PostEffectPassScope::Screen;
	const std::string defaultKey =
		(isScreen ? "ScreenEffectPass_Loaded_" : "LayerEffectPass_Loaded_") + std::to_string(index + 1);
	const std::string defaultName =
		(isScreen ? "Screen Effect Loaded " : "Layer Effect Loaded ") + std::to_string(index + 1);
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
	std::unordered_set<std::string>& usedKeys)
{
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
			(isScreen ? "ScreenEffectPass_Loaded_" : "LayerEffectPass_Loaded_") + std::to_string(index + 1);
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
	const std::vector<Render::PostEffectPassHandle>& order)
{
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
/// @param filePath 読み込むJSONパス
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJsonInternal(
	Render::PostEffectManager& manager,
	const std::filesystem::path& filePath = kPostEffectEditorJsonPath)
{
	nlohmann::json root;
	if (!Json::JsonFile::Load(filePath, root)) {
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

			// GPU完了待機後の読み込みによる差分削除の即時反映
			manager.Destroy(handle);
		}
	}

	ApplyPostEffectPassOrder(manager, Render::PostEffectPassScope::Layer, layerOrder);
	ApplyPostEffectPassOrder(manager, Render::PostEffectPassScope::Screen, screenOrder);
	return true;
}

#ifdef USE_IMGUI

/// @brief PassのParameterをJSONへ変換
/// @param pass 保存対象のPass
/// @return パラメータkeyと値を保持したJSON
nlohmann::json SerializePostEffectParameters(const Render::PostEffectPass& pass) {
	nlohmann::json parameters = nlohmann::json::object();
	const Render::PostEffectDefinition* definition = FindPostEffectDefinition(pass);
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
	Render::PostEffectPassScope scope)
{
	nlohmann::json passJson;
	passJson["key"] = pass.GetKey();
	passJson["name"] = pass.GetName();
	passJson["scope"] = GetPostEffectPassScopeName(scope);
	passJson["enabled"] = pass.IsEnabled();
	passJson["effectShaderKey"] = pass.GetEffectShaderKey();
	if (const Render::PostEffectDefinition* definition = FindPostEffectDefinition(pass)) {
		passJson["effectType"] = definition->typeName;
	}
	if (scope == Render::PostEffectPassScope::Layer) {
		passJson["targetLayerMask"] = pass.GetTargetLayerMask();
		passJson["layerEffectStage"] = GetLayerEffectStageName(pass.GetLayerEffectStage());
		passJson["ignoreDepthForMask"] = pass.IsIgnoreDepthForMask();
	} else {
		passJson["screenEffectStage"] = GetScreenEffectStageName(pass.GetScreenEffectStage());
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
	Render::PostEffectPassScope scope)
{
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
bool SavePostEffectEditorJson(const Render::PostEffectManager& manager) {
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

enum class PostEffectEditorOperationType {
	ChangeEffect,
	MovePass,
	LoadSettings,
	Count,
};

struct PostEffectEditorOperation {
	PostEffectEditorOperationType type = PostEffectEditorOperationType::ChangeEffect;
	Render::PostEffectPassHandle handle{};
	Render::PostEffectType effectType = Render::PostEffectType::CopyImage;
	Render::PostEffectPassScope scope = Render::PostEffectPassScope::Layer;
	std::size_t newIndex = 0;
	std::filesystem::path filePath;
};

/// @brief 次フレームへ予約されたEditor操作一覧を取得
/// @return 予約操作一覧
std::vector<PostEffectEditorOperation>& GetPendingPostEffectEditorOperations() {
	static std::vector<PostEffectEditorOperation> operations;
	return operations;
}

/// @brief Effect変更を次フレームへ予約
/// @param handle 変更対象のPass Handle
/// @param effectType 適用するEffect種別
void ReservePostEffectChange(Render::PostEffectPassHandle handle, Render::PostEffectType effectType) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::ChangeEffect;
	operation.handle = handle;
	operation.effectType = effectType;
	GetPendingPostEffectEditorOperations().push_back(operation);
}

/// @brief Pass並べ替えを次フレームへ予約
/// @param handle 移動対象のPass Handle
/// @param scope Passの適用先
/// @param newIndex 移動先index
void ReservePostEffectPassMove(
	Render::PostEffectPassHandle handle,
	Render::PostEffectPassScope scope,
	std::size_t newIndex)
{
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::MovePass;
	operation.handle = handle;
	operation.scope = scope;
	operation.newIndex = newIndex;
	GetPendingPostEffectEditorOperations().push_back(operation);
}

/// @brief JSON設定読込を次フレームへ予約
/// @param filePath 読み込むJSONパス
void ReservePostEffectSettingsLoad(const std::filesystem::path& filePath) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::LoadSettings;
	operation.filePath = filePath;
	GetPendingPostEffectEditorOperations().push_back(std::move(operation));
}

/// @brief 重複しないPassキーを作成
/// @param manager 重複確認に使用するManager
/// @param prefix Passキーの接頭辞
/// @param nextId 次に試すID
/// @return 重複しないPassキー
std::string CreateUniquePostEffectPassKey(
	const Render::PostEffectManager& manager,
	const char* prefix,
	int& nextId)
{
	std::string key;
	do {
		key = std::string(prefix) + std::to_string(nextId++);
	} while (manager.Find(key).IsValid());
	return key;
}

/// @brief EditorからPassを追加
/// @param manager 追加先のManager
/// @param scope 追加するPassの適用先
/// @return 追加したPassのHandle
Render::PostEffectPassHandle AddPostEffectPassFromEditor(
	Render::PostEffectManager& manager,
	Render::PostEffectPassScope scope)
{
	static int nextLayerPassId = 1;
	static int nextScreenPassId = 1;
	if (scope == Render::PostEffectPassScope::Screen) {
		Render::ScreenPostEffectPassCreateDesc desc{};
		desc.key = CreateUniquePostEffectPassKey(manager, "ScreenEffectPass_", nextScreenPassId);
		desc.name = "Screen Effect " + std::to_string(nextScreenPassId - 1);
		return manager.CreateScreenPass(desc);
	}

	Render::LayerPostEffectPassCreateDesc desc{};
	desc.key = CreateUniquePostEffectPassKey(manager, "LayerEffectPass_", nextLayerPassId);
	desc.name = "Layer Effect " + std::to_string(nextLayerPassId - 1);
	return manager.CreateLayerPass(desc);
}

/// @brief 対象Layerの選択Comboを描画
/// @param pass 編集対象のPass
void DrawLayerSelectionCombo(Render::PostEffectPass& pass) {
	const Render::RenderLayerMask currentLayerMask = pass.GetTargetLayerMask();
	ImGui::SetNextItemWidth(-1.0f);
	if (!ImGui::BeginCombo("##TargetLayer", Render::GetRenderLayerMaskName(currentLayerMask))) {
		return;
	}

	for (uint32_t index = 0; index < Render::kRenderLayerCount; ++index) {
		const Render::RenderLayer layer = Render::GetRenderLayerByIndex(index);
		const Render::RenderLayerMask layerMask = Render::ToRenderLayerMask(layer);
		const bool selected = layerMask == currentLayerMask;
		if (ImGui::Selectable(Render::GetRenderLayerName(layer), selected)) {
			pass.SetTargetLayerMask(layerMask);
		}
		if (selected) {
			ImGui::SetItemDefaultFocus();
		}
	}

	ImGui::EndCombo();
}

/// @brief Effect選択Comboを描画
/// @param handle 編集対象のPass Handle
/// @param pass 編集対象の一時参照
void DrawPostEffectSelectionCombo(
	Render::PostEffectPassHandle handle,
	const Render::PostEffectPass& pass)
{
	const Render::PostEffectDefinition* currentDefinition = FindPostEffectDefinition(pass);
	const char* preview = currentDefinition ? currentDefinition->displayName.data() : "Unknown";
	ImGui::SetNextItemWidth(-1.0f);
	if (!ImGui::BeginCombo("##PostEffect", preview)) {
		return;
	}

	for (const Render::PostEffectDefinition& definition : Render::PostEffectDefinitionRegistry::GetAll()) {
		const bool selected = currentDefinition && definition.type == currentDefinition->type;
		if (ImGui::Selectable(definition.displayName.data(), selected)) {
			ReservePostEffectChange(handle, definition.type);
		}
		if (selected) {
			ImGui::SetItemDefaultFocus();
		}
	}
	ImGui::EndCombo();
}

/// @brief パラメータkeyが指定文字で終わるか判定
/// @param value 判定する文字列
/// @param suffix 末尾文字
/// @return 指定文字で終わる場合はtrue
bool EndsWith(std::string_view value, char suffix) {
	return !value.empty() && value.back() == suffix;
}

/// @brief 色成分keyから共通部分を取得
/// @param value 色成分key
/// @return 末尾のRGBAを除いたkey
std::string GetColorBase(std::string_view value) {
	if (EndsWith(value, 'R') || EndsWith(value, 'G') || EndsWith(value, 'B') || EndsWith(value, 'A')) {
		return std::string(value.substr(0, value.size() - 1));
	}
	return std::string(value);
}

/// @brief Registry定義から色編集可能な連続成分数を取得
/// @param parameters Parameter定義一覧
/// @param startIndex 確認開始index
/// @return RGBなら3、RGBAなら4、色でない場合は0
int GetColorComponentCount(
	std::span<const Render::PostEffectFloatParameterDefinition> parameters,
	std::size_t startIndex)
{
	if (startIndex + 2 >= parameters.size() || !EndsWith(parameters[startIndex].key, 'R')) {
		return 0;
	}

	const std::string base = GetColorBase(parameters[startIndex].key);
	const std::size_t offset = parameters[startIndex].offset;
	for (std::size_t component = 1; component < 3; ++component) {
		const char suffix = component == 1 ? 'G' : 'B';
		if (parameters[startIndex + component].key != base + suffix ||
			parameters[startIndex + component].offset != offset + sizeof(float) * component) {
			return 0;
		}
	}

	if (startIndex + 3 < parameters.size() &&
		parameters[startIndex + 3].key == base + 'A' &&
		parameters[startIndex + 3].offset == offset + sizeof(float) * 3) {
		return 4;
	}
	return 3;
}

/// @brief Registry定義を使って型非依存のParameter編集UIを描画
/// @param pass 編集対象のPass
void DrawPostEffectParameterRows(Render::PostEffectPass& pass) {
	const Render::PostEffectDefinition* definition = FindPostEffectDefinition(pass);
	if (!definition || definition->parameterCount == 0) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::TextDisabled("調整項目はありません");
		return;
	}

	const std::span<const Render::PostEffectFloatParameterDefinition> parameters = definition->GetParameters();
	for (std::size_t index = 0; index < parameters.size(); ++index) {
		const Render::PostEffectFloatParameterDefinition& parameter = parameters[index];
		const int colorComponentCount = GetColorComponentCount(parameters, index);
		if (colorComponentCount > 0) {
			float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			bool readable = true;
			for (int component = 0; component < colorComponentCount; ++component) {
				readable &= pass.TryGetFloatParameter(parameters[index + component].offset, color[component]);
			}
			if (readable) {
				std::string label(parameter.label);
				if (!label.empty()) {
					label.pop_back();
				}
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(label.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::SetNextItemWidth(-1.0f);
				const std::string id = "##" + GetColorBase(parameter.key);
				const bool changed = colorComponentCount == 4 ?
					ImGui::ColorEdit4(id.c_str(), color) : ImGui::ColorEdit3(id.c_str(), color);
				if (changed) {
					for (int component = 0; component < colorComponentCount; ++component) {
						pass.SetFloatParameter(parameters[index + component].offset, color[component]);
					}
				}
			}
			index += static_cast<std::size_t>(colorComponentCount - 1);
			continue;
		}

		float value = 0.0f;
		if (!pass.TryGetFloatParameter(parameter.offset, value)) {
			continue;
		}
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(parameter.label.data());
		ImGui::TableSetColumnIndex(2);
		ImGui::SetNextItemWidth(-1.0f);
		const std::string id = "##" + std::string(parameter.key);
		const std::span<const std::string_view> selectionOptions = parameter.GetSelectionOptions();
		if (!selectionOptions.empty()) {

			// Shaderへ渡すfloat値を整数Indexとして扱い選択肢以外の値をEditorから生成しない制約
			const int selectedIndex = std::clamp(
				static_cast<int>(std::round(value)),
				0,
				static_cast<int>(selectionOptions.size() - 1)
			);
			if (ImGui::BeginCombo(id.c_str(), selectionOptions[selectedIndex].data())) {
				for (std::size_t optionIndex = 0; optionIndex < selectionOptions.size(); ++optionIndex) {
					const bool selected = static_cast<int>(optionIndex) == selectedIndex;
					if (ImGui::Selectable(selectionOptions[optionIndex].data(), selected)) {
						pass.SetFloatParameter(parameter.offset, static_cast<float>(optionIndex));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			continue;
		}
		if (ImGui::DragFloat(id.c_str(), &value, parameter.speed, parameter.minValue, parameter.maxValue)) {
			pass.SetFloatParameter(parameter.offset, value);
		}
	}
}

/// @brief Handleの現在の実行順indexを取得
/// @param handles 検索対象のHandle順序
/// @param handle 検索するHandle
/// @return 見つかったindex、存在しない場合はhandles.size()
std::size_t FindHandleIndex(
	const std::vector<Render::PostEffectPassHandle>& handles,
	Render::PostEffectPassHandle handle)
{
	const auto it = std::find(handles.begin(), handles.end(), handle);
	return it == handles.end() ? handles.size() : static_cast<std::size_t>(std::distance(handles.begin(), it));
}

#endif // USE_IMGUI

} // namespace

bool LoadPostEffectEditorJson(Render::PostEffectManager& postEffectManager) {
	return LoadPostEffectEditorJsonInternal(postEffectManager);
}

bool LoadPostEffectEditorJsonFromFile(Render::PostEffectManager& postEffectManager) {
	return LoadPostEffectEditorJson(postEffectManager);
}

#ifdef USE_IMGUI

void ApplyPendingPostEffectEditorOperations(Render::PostEffectManager& postEffectManager) {
	std::vector<PostEffectEditorOperation>& pending = GetPendingPostEffectEditorOperations();
	if (pending.empty()) {
		return;
	}

	std::vector<PostEffectEditorOperation> operations = std::move(pending);
	pending.clear();
	for (const PostEffectEditorOperation& operation : operations) {
		switch (operation.type) {
		case PostEffectEditorOperationType::ChangeEffect:
			if (postEffectManager.IsValid(operation.handle)) {
				postEffectManager.SetEffectType(operation.handle, operation.effectType);
			}
			break;
		case PostEffectEditorOperationType::MovePass:
			if (operation.scope == Render::PostEffectPassScope::Layer) {
				postEffectManager.MoveLayerPass(operation.handle, operation.newIndex);
			} else if (operation.scope == Render::PostEffectPassScope::Screen) {
				postEffectManager.MoveScreenPass(operation.handle, operation.newIndex);
			}
			break;
		case PostEffectEditorOperationType::LoadSettings:
			LoadPostEffectEditorJsonInternal(postEffectManager, operation.filePath);
			break;
		case PostEffectEditorOperationType::Count:
		default:
			break;
		}
	}
}

void DrawPostEffectEditorUI(Render::PostEffectManager& postEffectManager) {
	static Render::PostEffectPassHandle selectedHandle{};

	ImGui::Begin("Post Effect Editor");
	if (ImGui::Button("レイヤー追加")) {
		selectedHandle = AddPostEffectPassFromEditor(postEffectManager, Render::PostEffectPassScope::Layer);
	}
	ImGui::SameLine();
	if (ImGui::Button("フルスクリーン追加")) {
		selectedHandle = AddPostEffectPassFromEditor(postEffectManager, Render::PostEffectPassScope::Screen);
	}

	if (ImGui::Button("保存")) {
		SavePostEffectEditorJson(postEffectManager);
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		ReservePostEffectSettingsLoad(kPostEffectEditorJsonPath);
		selectedHandle = {};
	}
	ImGui::SameLine();
	if (ImGui::Button("復元")) {
		ReservePostEffectSettingsLoad(CreateBackupJsonPath(kPostEffectEditorJsonPath));
		selectedHandle = {};
	}

	ImGui::Separator();
	Render::PostEffectPassHandle removeRequest{};
	ImGui::BeginChild("PostEffectPassList", ImVec2(260.0f, 0.0f), true);
	auto drawList = [&](const std::vector<Render::PostEffectPassHandle>& handles, const char* scopeLabel) {
		for (Render::PostEffectPassHandle handle : handles) {
			Render::PostEffectPass* pass = postEffectManager.TryGet(handle);
			if (!pass) {
				continue;
			}

			ImGui::PushID(static_cast<int>(handle.index));
			bool enabled = pass->IsEnabled();
			if (ImGui::Checkbox("##Enabled", &enabled)) {
				pass->SetEnabled(enabled);
			}
			ImGui::SameLine();

			const std::string label = std::string("[") + scopeLabel + "] " + pass->GetName();
			const float deleteButtonWidth =
				ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float selectableWidth =
				ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x;
			if (selectableWidth < 1.0f) {
				selectableWidth = 1.0f;
			}
			if (ImGui::Selectable(
				label.c_str(),
				selectedHandle == handle,
				0,
				ImVec2(selectableWidth, 0.0f))) {
				selectedHandle = handle;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("削除")) {
				removeRequest = handle;
			}
			ImGui::PopID();
		}
	};
	drawList(postEffectManager.GetLayerPassHandles(), "Layer");
	drawList(postEffectManager.GetScreenPassHandles(), "Screen");
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("PostEffectPassProperties", ImVec2(0.0f, 0.0f), true);
	Render::PostEffectPass* selectedPass = postEffectManager.TryGet(selectedHandle);
	if (!selectedPass) {
		selectedHandle = {};
		ImGui::TextDisabled("PostEffectPassを選択してください。");
	} else {
		ImGui::PushID(static_cast<int>(selectedHandle.index));
		std::array<char, 128> nameBuffer{};
		std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selectedPass->GetName().c_str());
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()) && nameBuffer[0] != '\0') {
			selectedPass->SetName(nameBuffer.data());
		}
		ImGui::Text("Key: %s", selectedPass->GetKey().c_str());

		ImGui::TextUnformatted("Effect");
		DrawPostEffectSelectionCombo(selectedHandle, *selectedPass);

		const Render::PostEffectPassScope scope = postEffectManager.GetScope(selectedHandle);
		if (scope == Render::PostEffectPassScope::Layer) {
			ImGui::TextUnformatted("Layer");
			DrawLayerSelectionCombo(*selectedPass);
			int stageIndex = static_cast<int>(selectedPass->GetLayerEffectStage());
			const char* stages[] = { "Scene", "Transparent", "Overlay" };
			if (ImGui::Combo("Stage", &stageIndex, stages, 3)) {
				selectedPass->SetLayerEffectStage(static_cast<Render::LayerEffectStage>(stageIndex));
			}
			bool ignoreDepth = selectedPass->IsIgnoreDepthForMask();
			if (ImGui::Checkbox("Ignore Depth", &ignoreDepth)) {
				selectedPass->SetIgnoreDepthForMask(ignoreDepth);
			}
		} else {
			int stageIndex = static_cast<int>(selectedPass->GetScreenEffectStage());
			const char* stages[] = { "Scene", "Final" };
			if (ImGui::Combo("Stage", &stageIndex, stages, 2)) {
				selectedPass->SetScreenEffectStage(static_cast<Render::ScreenEffectStage>(stageIndex));
			}
		}

		const std::vector<Render::PostEffectPassHandle>& order =
			scope == Render::PostEffectPassScope::Layer ?
			postEffectManager.GetLayerPassHandles() : postEffectManager.GetScreenPassHandles();
		const std::size_t currentIndex = FindHandleIndex(order, selectedHandle);
		ImGui::BeginDisabled(currentIndex == 0 || currentIndex >= order.size());
		if (ImGui::Button("上へ")) {
			ReservePostEffectPassMove(selectedHandle, scope, currentIndex - 1);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(currentIndex + 1 >= order.size());
		if (ImGui::Button("下へ")) {
			ReservePostEffectPassMove(selectedHandle, scope, currentIndex + 1);
		}
		ImGui::EndDisabled();

		ImGui::SeparatorText("Parameters");
		constexpr ImGuiTableFlags propertyTableFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("PostEffectPassPropertyTable", 3, propertyTableFlags)) {
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 1.0f);
			ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
			DrawPostEffectParameterRows(*selectedPass);
			ImGui::EndTable();
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (removeRequest.IsValid()) {

		// GPU ResourceをPostDrawの完了待機後まで保持して次Frame開始前に破棄
		postEffectManager.RequestDestroy(removeRequest);
		if (selectedHandle == removeRequest) {
			selectedHandle = {};
		}
	}
	ImGui::End();
}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
