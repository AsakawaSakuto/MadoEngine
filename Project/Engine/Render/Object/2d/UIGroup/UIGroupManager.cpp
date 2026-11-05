#include "UIGroupManager.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/Text/TextManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include <objbase.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <unordered_set>

namespace MadoEngine {
namespace {

struct ParsedGroup {
	std::string id;
	std::string name;
	SceneType scene = SceneType::None;
	TransformNode2D::AuthoringState state;
	std::vector<UIGroupElementInfo> elements;
};

/// @brief 同名Groupをシーン単位で区別する索引キーを生成
/// @param name Groupの表示名
/// @param scene 所属シーン
/// @return シーン名と表示名を連結した索引キー
std::string NameKey(const std::string& name, SceneType scene) {
	return SceneTypeToString(scene) + "\n" + name;
}

/// @brief シーンの絞り込み条件を判定
/// @param scene 判定する所属シーン
/// @param filter 対象シーン、未指定の場合は全シーン
/// @return 共通シーンまたは対象シーンの場合はtrue
bool MatchesScene(SceneType scene, std::optional<SceneType> filter) {
	return !filter || scene == SceneType::None || scene == *filter;
}

/// @brief 再起動と表示名変更に依存しないIDを生成
/// @return GUID文字列、生成に失敗した場合は空文字列
std::string CreatePersistentId() {
	GUID guid{};
	if (FAILED(CoCreateGuid(&guid))) {
		return {};
	}
	return std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

/// @brief 編集用ローカル設定をJsonへ変換
/// @param state 保存する設定
/// @return 編集用設定のJson
nlohmann::json EncodeState(const TransformNode2D::AuthoringState& state) {
	return {
		{ "position", { state.transform.translate.x, state.transform.translate.y } },
		{ "scale", { state.transform.scale.x, state.transform.scale.y } },
		{ "rotation", state.transform.rotate }, { "visible", state.visible }, { "opacity", state.opacity },
	};
}

/// @brief 有限値の編集用ローカル設定をJsonから取得
/// @param json 読込元のJson
/// @param state 読込結果
/// @return 必須項目と値が有効な場合はtrue
bool DecodeState(const nlohmann::json& json, TransformNode2D::AuthoringState& state) {
	const auto& position = json.at("position");
	const auto& scale = json.at("scale");

	// 配列長と値域を先に検証し、部分的な設定変更を防止
	if (!position.is_array() || position.size() != 2 || !scale.is_array() || scale.size() != 2) {
		return false;
	}
	state.transform.translate = { position.at(0).get<float>(), position.at(1).get<float>() };
	state.transform.scale = { scale.at(0).get<float>(), scale.at(1).get<float>() };
	state.transform.rotate = json.at("rotation").get<float>();
	state.visible = json.at("visible").get<bool>();
	state.opacity = json.at("opacity").get<float>();
	TransformNode2D validation;
	return validation.SetAuthoringState(state) && state.opacity >= 0.0f && state.opacity <= 1.0f;
}

/// @brief 子要素の種類を保存用文字列へ変換
/// @param type 要素の種類
/// @return 保存用の種類名
const char* TypeName(UIGroupElementType type) {
	switch (type) {
	case UIGroupElementType::Sprite: return "Sprite";
	case UIGroupElementType::Text: return "Text";
	case UIGroupElementType::Group: return "Group";
	}
	return "Invalid";
}

/// @brief Json全体を検証して置換前の構成を構築
/// @param json 読込元のJson
/// @param groups 検証済みの構成
/// @return スキーマ・一意性・親子関係が有効な場合はtrue
bool ParseDocument(const nlohmann::json& json, std::vector<ParsedGroup>& groups) {
	try {
		if (!json.is_object() || json.value("version", 0) != 1 || !json.at("groups").is_array()) {
			return false;
		}
		std::unordered_set<std::string> ids;
		std::unordered_set<std::string> names;
		std::unordered_set<std::string> memberships;

		// 他シーンを含めて検証し、部分保存でも壊れた構成を引き継がないための事前解析
		for (const auto& item : json.at("groups")) {
			ParsedGroup group;
			group.id = item.at("id").get<std::string>();
			group.name = item.at("name").get<std::string>();
			const auto scene = item.at("scene").get<std::string>();
			group.scene = SceneTypeFromString(scene);
			if (group.id.empty() || group.name.empty() || !ids.insert(group.id).second ||
				!names.insert(NameKey(group.name, group.scene)).second || SceneTypeToString(group.scene) != scene ||
				!DecodeState(item.at("state"), group.state) || !item.at("children").is_array()) {
				return false;
			}
			std::unordered_set<std::string> aliases;
			for (const auto& child : item.at("children")) {
				UIGroupElementInfo element;
				element.name = child.at("name").get<std::string>();
				element.target = child.at("target").get<std::string>();
				const auto type = child.at("type").get<std::string>();
				if (type == "Sprite") {
					element.type = UIGroupElementType::Sprite;
				} else if (type == "Text") {
					element.type = UIGroupElementType::Text;
				} else if (type == "Group") {
					element.type = UIGroupElementType::Group;
				} else {
					return false;
				}
				if (element.name.empty() || element.target.empty() || !aliases.insert(element.name).second ||
					!memberships.insert(type + "\n" + element.target).second) {
					return false;
				}
				if (element.type != UIGroupElementType::Group && !DecodeState(child.at("state"), element.state)) {
					return false;
				}
				group.elements.push_back(std::move(element));
			}
			groups.push_back(std::move(group));
		}

		// 各Groupの親をたどり、自己参照・循環・シーンをまたぐ接続を適用前に拒否
		std::unordered_map<std::string, const ParsedGroup*> byId;
		std::unordered_map<std::string, std::string> parents;
		for (const auto& group : groups) {
			byId[group.id] = &group;
			for (const auto& element : group.elements) {
				if (element.type == UIGroupElementType::Group) {
					parents[element.target] = group.id;
				}
			}
		}
		for (const auto& group : groups) {
			std::unordered_set<std::string> visited;
			std::string current = group.id;
			while (parents.contains(current)) {
				if (!visited.insert(current).second) {
					return false;
				}
				current = parents.at(current);
				if (byId.contains(current) && byId.at(current)->scene != group.scene) {
					return false;
				}
			}
		}
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

} // namespace

UIGroupManager& UIGroupManager::GetInstance() {
	static UIGroupManager manager;
	return manager;
}

UIGroupHandle UIGroupManager::Create(const std::string& name, SceneType sceneType) {
	if (name.empty() || names_.contains(NameKey(name, sceneType)) || SceneTypeToString(sceneType) == "Unknown") {
		return {};
	}
	const std::string id = CreatePersistentId();
	if (id.empty()) {
		return {};
	}

	// 破棄済みSlotを世代更新後に再利用し、選択中の旧Handleを無効化
	uint32_t index = 0;
	if (freeSlots_.empty()) {
		index = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	} else {
		index = freeSlots_.back();
		freeSlots_.pop_back();
	}
	auto& slot = slots_[index];
	slot.group = std::make_unique<UIGroup>();
	slot.group->SetAuthoringState({});
	slot.id = id;
	slot.name = name;
	slot.scene = sceneType;
	const UIGroupHandle handle{ index, slot.generation };
	names_[NameKey(name, sceneType)] = handle;
	ids_[id] = handle;
	return handle;
}

UIGroup* UIGroupManager::TryGet(UIGroupHandle handle) {
	return const_cast<UIGroup*>(std::as_const(*this).TryGet(handle));
}

const UIGroup* UIGroupManager::TryGet(UIGroupHandle handle) const {
	if (!handle.IsValid() || handle.index >= slots_.size() || slots_[handle.index].generation != handle.generation) {
		return nullptr;
	}
	return slots_[handle.index].group.get();
}

UIGroupHandle UIGroupManager::Find(const std::string& name) const {
	UIGroupHandle result;

	// シーン指定なしの検索で曖昧な名前を別Groupへ誤接続しないための一意性確認
	for (const auto& slot : slots_) {
		if (slot.group && slot.name == name) {
			if (result.IsValid()) {
				return {};
			}
			result = ids_.at(slot.id);
		}
	}
	return result;
}

UIGroupHandle UIGroupManager::Find(const std::string& name, SceneType sceneType) const {
	const auto it = names_.find(NameKey(name, sceneType));
	if (it != names_.end()) {
		return it->second;
	}
	return {};
}

UIGroupHandle UIGroupManager::FindById(const std::string& id) const {
	const auto it = ids_.find(id);
	if (it != ids_.end()) {
		return it->second;
	}
	return {};
}

std::string UIGroupManager::GetName(UIGroupHandle handle) const {
	if (TryGet(handle)) {
		return slots_[handle.index].name;
	}
	return {};
}

std::string UIGroupManager::GetId(UIGroupHandle handle) const {
	if (TryGet(handle)) {
		return slots_[handle.index].id;
	}
	return {};
}

SceneType UIGroupManager::GetSceneType(UIGroupHandle handle) const {
	if (TryGet(handle)) {
		return slots_[handle.index].scene;
	}
	return SceneType::None;
}

std::vector<UIGroupHandle> UIGroupManager::GetHandles() const {
	std::vector<UIGroupHandle> result;
	for (const auto& [name, handle] : names_) {
		(void)name;
		result.push_back(handle);
	}
	std::sort(result.begin(), result.end(), [this](auto lhs, auto rhs) {
		return NameKey(GetName(lhs), GetSceneType(lhs)) < NameKey(GetName(rhs), GetSceneType(rhs));
	});
	return result;
}

bool UIGroupManager::Rename(UIGroupHandle handle, const std::string& name) {

	// 表示名の変更から永続IDを独立させ、親からの参照を維持
	if (!TryGet(handle) || name.empty() || names_.contains(NameKey(name, GetSceneType(handle)))) {
		return false;
	}
	auto& slot = slots_[handle.index];
	names_.erase(NameKey(slot.name, slot.scene));
	slot.name = name;
	names_[NameKey(name, slot.scene)] = handle;
	return true;
}

bool UIGroupManager::Destroy(UIGroupHandle handle) {
	if (!TryGet(handle)) {
		return false;
	}

	// 親側の参照と直接の子だけを解除し、子Group・Sprite・Textの寿命を維持
	const auto id = GetId(handle);
	for (auto parent : GetHandles()) {
		for (const auto& element : GetElements(parent)) {
			if (element.type == UIGroupElementType::Group && element.target == id) {
				RemoveElement(parent, element.name);
			}
		}
	}
	for (const auto& element : GetElements(handle)) {
		RemoveElement(handle, element.name);
	}
	auto& slot = slots_[handle.index];
	names_.erase(NameKey(slot.name, slot.scene));
	ids_.erase(slot.id);
	slot.group.reset();
	slot.elements.clear();
	slot.name.clear();
	slot.id.clear();
	slot.generation = NextObjectGeneration(slot.generation);
	freeSlots_.push_back(handle.index);
	return true;
}

void UIGroupManager::DestroyByScene(SceneType sceneType) {

	// 共通Groupは他シーンでも利用するため、指定シーンとの完全一致だけを削除
	for (auto handle : GetHandles()) {
		if (GetSceneType(handle) == sceneType) {
			Destroy(handle);
		}
	}
}

void UIGroupManager::Finalize() {
	for (auto handle : GetHandles()) {
		Destroy(handle);
	}
}

bool UIGroupManager::AddElement(UIGroupHandle handle, const std::string& name, UIGroupElementType type, const std::string& target) {
	auto* group = TryGet(handle);
	if (!group || name.empty() || target.empty() || group->ContainsElement(name)) {
		return false;
	}
	UIGroupElementInfo element;
	element.name = name;
	element.type = type;
	element.target = target;
	TransformNode2D* node = nullptr;
	if (type == UIGroupElementType::Sprite) {
		node = SpriteManager::GetInstance().TryGet(SpriteManager::GetInstance().Find(target));
	} else if (type == UIGroupElementType::Text) {
		node = TextManager::GetInstance().TryGet(TextManager::GetInstance().Find(target));
	} else if (type == UIGroupElementType::Group) {
		node = TryGet(FindById(target));
	}

	// 登録時だけ現在の配置を編集用の初期値として採用
	if (!node || node->HasParent()) {
		return false;
	}
	element.state = node->GetAuthoringState();
	auto& slot = slots_[handle.index];
	if (std::any_of(slot.elements.begin(), slot.elements.end(), [&](const auto& item) { return item.name == name; })) {
		return false;
	}
	slot.elements.push_back(element);
	if (!Resolve(slot, slot.elements.back())) {
		slot.elements.pop_back();
		return false;
	}
	return true;
}

bool UIGroupManager::RenameElement(UIGroupHandle handle, const std::string& name, const std::string& newName) {
	if (!TryGet(handle) || newName.empty()) {
		return false;
	}
	auto& slot = slots_[handle.index];
	for (const auto& element : slot.elements) {
		if (element.name == newName) {
			return false;
		}
	}

	// 未解決の参照も名前変更を許可し、対象が復元された際に新しい登録名を使用
	for (auto& element : slot.elements) {
		if (element.name == name) {
			if (slot.group->ContainsElement(name) && !slot.group->RenameElement(name, newName)) {
				return false;
			}
			element.name = newName;
			return true;
		}
	}
	return false;
}

bool UIGroupManager::CanCreateElement(UIGroupHandle handle, const std::string& name) const {
	const auto* group = TryGet(handle);

	// リソース生成前に重複を確認し、追加失敗による未所属オブジェクトの残存を回避
	if (!group || name.empty() || group->ContainsElement(name)) {
		return false;
	}
	const auto& elements = slots_[handle.index].elements;
	return std::none_of(elements.begin(), elements.end(), [&](const auto& element) { return element.name == name; });
}

SpriteHandle UIGroupManager::CreateSpriteElement(UIGroupHandle handle, const std::string& name, const std::string& textureName, std::optional<float> displaySize) {
	if (!CanCreateElement(handle, name) || textureName.empty() ||
		(displaySize && (!std::isfinite(*displaySize) || *displaySize <= 0.0f))) {
		return {};
	}
	const auto id = CreatePersistentId();
	if (id.empty()) {
		return {};
	}

	// Group内の名前とManagerの識別名を分離し、別Groupでも同じ要素名を使用
	auto& manager = SpriteManager::GetInstance();
	const auto target = "UIGroupSprite_" + id;
	const auto created = manager.Create(target, textureName, GetSceneType(handle), EditorManagementMode::EditorManaged);
	auto* element = manager.TryGet(created);
	if (!element) {
		return {};
	}
	const auto size = element->GetTextureSize();
	if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f) {
		manager.Destroy(created);
		return {};
	}

	// 通常は等倍を維持し、表示サイズの指定時だけ縦横比を保った拡縮を適用
	float scale = 1.0f;
	if (displaySize) {
		scale = *displaySize / (std::max)(size.x, size.y);
	}
	element->SetScale({ scale, scale });
	element->SetRenderLayer(Render::RenderLayer::UI);
	if (!AddElement(handle, name, UIGroupElementType::Sprite, target)) {

		// まだ描画へ使用していない生成直後のリソースを接続失敗時に回収
		manager.Destroy(created);
		return {};
	}
	return created;
}

TextHandle UIGroupManager::CreateTextElement(UIGroupHandle handle, const std::string& name, const std::string& text) {
	if (!CanCreateElement(handle, name)) {
		return {};
	}
	const auto id = CreatePersistentId();
	if (id.empty()) {
		return {};
	}

	// 本文と所属シーンを生成時に確定し、別Editorを経由しない作成フローへ統一
	auto& manager = TextManager::GetInstance();
	const auto target = "UIGroupText_" + id;
	const auto created = manager.Create(target, GetSceneType(handle), EditorManagementMode::EditorManaged);
	auto* element = manager.TryGet(created);
	if (!element) {
		return {};
	}
	element->SetText(text);

	// 後段のUI背景に隠れないよう、Spriteと同じUIレイヤーを初期値として使用
	element->SetRenderLayer(Render::RenderLayer::UI);
	if (!AddElement(handle, name, UIGroupElementType::Text, target)) {

		// 初期本文の設定後に接続できなかったTextを残さないための回収
		manager.Destroy(created);
		return {};
	}
	return created;
}

UIGroupHandle UIGroupManager::CreateGroupElement(UIGroupHandle handle, const std::string& name) {
	if (!CanCreateElement(handle, name)) {
		return {};
	}
	const auto scene = GetSceneType(handle);
	std::string displayName = name;
	uint32_t suffix = 1;

	// 親内の要素名を維持したまま、シーン内の表示名だけを一意化
	while (Find(displayName, scene).IsValid()) {
		displayName = name + std::to_string(suffix++);
	}
	const auto created = Create(displayName, scene);
	if (!TryGet(created)) {
		return {};
	}
	if (!AddElement(handle, name, UIGroupElementType::Group, GetId(created))) {
		Destroy(created);
		return {};
	}
	return created;
}

bool UIGroupManager::RemoveElement(UIGroupHandle handle, const std::string& name) {
	if (!TryGet(handle)) {
		return false;
	}
	auto& slot = slots_[handle.index];
	const auto it = std::find_if(slot.elements.begin(), slot.elements.end(), [&](const auto& element) { return element.name == name; });
	if (it == slot.elements.end()) {
		return false;
	}

	// 外部から所属を変更した要素や子Groupの編集用設定を誤って解除しないための限定
	if (it->type != UIGroupElementType::Group) {
		if (auto* node = TryGetElement(handle, name)) {
			node->ClearAuthoringState();
		}
	}
	slot.group->Remove(name);
	slot.elements.erase(it);
	return true;
}

TransformNode2D* UIGroupManager::TryGetElement(UIGroupHandle handle, const std::string& name) {
	auto* group = TryGet(handle);
	if (!group) {
		return nullptr;
	}
	if (auto* node = group->TryGetSprite(name)) {
		return node;
	}
	if (auto* node = group->TryGetText(name)) {
		return node;
	}
	return group->TryGetGroup(name);
}

bool UIGroupManager::DestroyElement(UIGroupHandle handle, const std::string& name) {
	const auto elements = GetElements(handle);
	const auto it = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.name == name; });
	if (it == elements.end() || !RemoveElement(handle, name)) {
		return false;
	}

	// 未接続の旧参照を削除しても、別Groupへ所属変更された対象を巻き込まないための限定
	if (!it->resolved) {
		return true;
	}
	if (it->type == UIGroupElementType::Sprite) {
		SpriteManager::GetInstance().RequestDestroy(it->sprite);
	} else if (it->type == UIGroupElementType::Text) {
		TextManager::GetInstance().RequestDestroy(it->text);
	} else {
		Destroy(it->group);
	}
	return true;
}

bool UIGroupManager::MoveElement(UIGroupHandle source, const std::string& name, UIGroupHandle destination) {
	if (source == destination || !TryGet(source) || !TryGet(destination) ||
		GetSceneType(source) != GetSceneType(destination) || TryGet(destination)->ContainsElement(name)) {
		return false;
	}
	RefreshBindings();
	const auto original = slots_[source.index].elements;
	const auto it = std::find_if(original.begin(), original.end(), [&](const auto& element) { return element.name == name; });
	if (it == original.end() || !TryGetElement(source, name)) {
		return false;
	}

	// 循環などで移動に失敗した際は登録順も含めて元の構成へ復元
	RemoveElement(source, name);
	if (AddElement(destination, name, it->type, it->target)) {
		return true;
	}
	auto& slot = slots_[source.index];
	slot.elements = original;
	for (auto& element : slot.elements) {
		Resolve(slot, element);
	}
	return false;
}

std::vector<UIGroupElementInfo> UIGroupManager::GetElements(UIGroupHandle handle) const {
	const auto* group = TryGet(handle);
	if (!group) {
		return {};
	}
	auto elements = slots_[handle.index].elements;

	// 保存時は実行時値ではなく接続先の編集用設定と現在の管理名だけを取得
	for (auto& element : elements) {
		const TransformNode2D* node = nullptr;
		if (element.type == UIGroupElementType::Sprite) {
			node = group->TryGetSprite(element.name);
			if (node != SpriteManager::GetInstance().TryGet(element.sprite)) {
				node = nullptr;
			}
			if (node) {
				element.target = SpriteManager::GetInstance().GetName(element.sprite);
			}
		} else if (element.type == UIGroupElementType::Text) {
			node = group->TryGetText(element.name);
			if (node != TextManager::GetInstance().TryGet(element.text)) {
				node = nullptr;
			}
			if (node) {
				element.target = TextManager::GetInstance().GetName(element.text);
			}
		} else {
			node = group->TryGetGroup(element.name);
			if (node != TryGet(FindById(element.target))) {
				node = nullptr;
			}
		}
		element.resolved = node != nullptr;
		if (node) {
			element.state = node->GetAuthoringState();
		}
	}
	return elements;
}

TransformNode2D* UIGroupManager::Resolve(Slot& slot, UIGroupElementInfo& element) {
	TransformNode2D* node = nullptr;
	if (element.type == UIGroupElementType::Sprite) {
		auto& manager = SpriteManager::GetInstance();
		auto* attached = slot.group->TryGetSprite(element.name);

		// 実行側から所属シーンが変更された場合も、別シーンの親を引き継がないための解除
		if (attached && attached == manager.TryGet(element.sprite) &&
			(!manager.IsEditorManaged(element.sprite) || attached->GetSceneType() != slot.scene)) {
			element.state = attached->GetAuthoringState();
			attached->ClearAuthoringState();
			slot.group->Remove(element.name);
		}
		if (slot.group->TryGetSprite(element.name) == manager.TryGet(element.sprite) && manager.TryGet(element.sprite)) {
			element.target = manager.GetName(element.sprite);
			node = slot.group->TryGetSprite(element.name);
		} else {
			element.sprite = manager.Find(element.target);
			auto* sprite = manager.TryGet(element.sprite);
			if (sprite && manager.IsEditorManaged(element.sprite) && sprite->GetSceneType() == slot.scene && !sprite->HasParent()) {
				slot.group->Remove(element.name);
				if (slot.group->AddSprite(element.name, element.sprite)) {
					sprite->SetAuthoringState(element.state);
					node = sprite;
				}
			}
		}
	} else if (element.type == UIGroupElementType::Text) {
		auto& manager = TextManager::GetInstance();
		auto* attached = slot.group->TryGetText(element.name);

		// シーンと管理方式の条件をSpriteと統一し、保存対象外のTextへの接続を防止
		if (attached && attached == manager.TryGet(element.text) &&
			(!manager.IsEditorManaged(element.text) || attached->GetSceneType() != slot.scene)) {
			element.state = attached->GetAuthoringState();
			attached->ClearAuthoringState();
			slot.group->Remove(element.name);
		}
		if (slot.group->TryGetText(element.name) == manager.TryGet(element.text) && manager.TryGet(element.text)) {
			element.target = manager.GetName(element.text);
			node = slot.group->TryGetText(element.name);
		} else {
			element.text = manager.Find(element.target);
			auto* text = manager.TryGet(element.text);
			if (text && manager.IsEditorManaged(element.text) && text->GetSceneType() == slot.scene && !text->HasParent()) {
				slot.group->Remove(element.name);
				if (slot.group->AddText(element.name, element.text)) {
					text->SetAuthoringState(element.state);
					node = text;
				}
			}
		}
	} else if (element.type == UIGroupElementType::Group) {
		element.group = FindById(element.target);
		auto* child = TryGet(element.group);
		if (child && slot.group->TryGetGroup(element.name) == child) {
			node = child;
		} else if (child && GetSceneType(element.group) == slot.scene && !child->HasParent()) {
			slot.group->Remove(element.name);
			if (slot.group->AddGroup(element.name, *child)) {
				node = child;
			}
		}
	}

	// 毎フレームの再接続では通常値だけを記録し、アニメーション値の保存を回避
	element.resolved = node != nullptr;
	if (node) {
		element.state = node->GetAuthoringState();
	}
	return node;
}

void UIGroupManager::RefreshBindings() {
	for (auto& slot : slots_) {
		if (slot.group) {
			for (auto& element : slot.elements) {
				Resolve(slot, element);
			}
		}
	}
}

nlohmann::json UIGroupManager::ToJson() const {
	nlohmann::json groups = nlohmann::json::array();
	for (auto handle : GetHandles()) {
		nlohmann::json children = nlohmann::json::array();
		for (const auto& element : GetElements(handle)) {
			nlohmann::json child = { { "name", element.name }, { "type", TypeName(element.type) }, { "target", element.target } };
			if (element.type != UIGroupElementType::Group) {
				child["state"] = EncodeState(element.state);
			}
			children.push_back(std::move(child));
		}
		groups.push_back({ { "id", GetId(handle) }, { "name", GetName(handle) },
			{ "scene", SceneTypeToString(GetSceneType(handle)) },
			{ "state", EncodeState(TryGet(handle)->GetAuthoringState()) }, { "children", children } });
	}
	return { { "version", 1 }, { "groups", groups } };
}

bool UIGroupManager::FromJson(const nlohmann::json& json, std::optional<SceneType> sceneType) {
	std::vector<ParsedGroup> parsed;
	if (!ParseDocument(json, parsed)) {
		return false;
	}
	std::unordered_set<std::string> selectedIds;
	for (const auto& group : parsed) {
		if (!MatchesScene(group.scene, sceneType)) {
			continue;
		}
		selectedIds.insert(group.id);
		const auto existing = Find(group.name, group.scene);
		const auto sameId = FindById(group.id);
		if ((TryGet(existing) && !MatchesScene(GetSceneType(existing), sceneType)) ||
			(TryGet(sameId) && !MatchesScene(GetSceneType(sameId), sceneType))) {
			return false;
		}
	}

	// 全体の検証後に対象シーンだけを解除し、他シーンと既存Handleを可能な限り維持
	for (auto handle : GetHandles()) {
		if (!MatchesScene(GetSceneType(handle), sceneType)) {
			continue;
		}
		if (!selectedIds.contains(GetId(handle))) {
			Destroy(handle);
		} else {
			for (const auto& element : GetElements(handle)) {
				RemoveElement(handle, element.name);
			}
			names_.erase(NameKey(GetName(handle), GetSceneType(handle)));
		}
	}
	for (const auto& group : parsed) {
		if (!MatchesScene(group.scene, sceneType)) {
			continue;
		}
		auto handle = FindById(group.id);
		if (!TryGet(handle)) {
			handle = Create(group.name, group.scene);
			if (!TryGet(handle)) {
				return false;
			}
			ids_.erase(GetId(handle));
		}
		auto& slot = slots_[handle.index];
		slot.id = group.id;
		slot.name = group.name;
		slot.scene = group.scene;
		slot.elements = group.elements;
		slot.group->SetAuthoringState(group.state);
		ids_[slot.id] = handle;
		names_[NameKey(slot.name, slot.scene)] = handle;
	}
	RefreshBindings();
	return true;
}

bool UIGroupManager::SaveToFile(const std::filesystem::path& path, SceneType sceneType) const {
	nlohmann::json output = { { "version", 1 }, { "groups", nlohmann::json::array() } };
	if (Json::JsonFile::Exists(path)) {
		nlohmann::json existing;
		std::vector<ParsedGroup> validated;
		if (!Json::JsonFile::Load(path, existing) || !ParseDocument(existing, validated)) {
			return false;
		}

		// 編集対象外のシーンをファイル内へ残し、現在シーンと共通Groupだけを置換
		for (const auto& item : existing.at("groups")) {
			if (!MatchesScene(SceneTypeFromString(item.at("scene").get<std::string>()), sceneType)) {
				output["groups"].push_back(item);
			}
		}
	}
	const auto current = ToJson();
	for (const auto& item : current.at("groups")) {
		if (MatchesScene(SceneTypeFromString(item.at("scene").get<std::string>()), sceneType)) {
			output["groups"].push_back(item);
		}
	}
	std::vector<ParsedGroup> validated;
	if (!ParseDocument(output, validated)) {
		return false;
	}
	return Json::JsonFile::Save(path, output, 4, true);
}

bool UIGroupManager::LoadFromFile(const std::filesystem::path& path, SceneType sceneType) {
	nlohmann::json json;
	return Json::JsonFile::Load(path, json) && FromJson(json, sceneType);
}

} // namespace MadoEngine
