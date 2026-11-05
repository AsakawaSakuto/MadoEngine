#include "UIGroup.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/Text/TextManager.h"
#include <utility>

namespace MadoEngine {

bool UIGroup::AddSprite(const std::string& name, SpriteHandle handle) {
	Sprite* sprite = SpriteManager::GetInstance().TryGet(handle);

	// 型をまたぐ名前の重複と、一つの要素の多重所属を防止
	if (name.empty() || elements_.contains(name) || !sprite || sprite->HasParent()) {
		return false;
	}
	elements_.emplace(name, handle);
	if (!sprite->SetParent(*this)) {
		elements_.erase(name);
		return false;
	}
	return true;
}

bool UIGroup::AddText(const std::string& name, TextHandle handle) {
	Text* text = TextManager::GetInstance().TryGet(handle);

	// Spriteと同じ名前空間で登録し、型別の取得時に誤参照を防止
	if (name.empty() || elements_.contains(name) || !text || text->HasParent()) {
		return false;
	}
	elements_.emplace(name, handle);
	if (!text->SetParent(*this)) {
		elements_.erase(name);
		return false;
	}
	return true;
}

bool UIGroup::AddGroup(const std::string& name, UIGroup& group) {
	if (name.empty() || elements_.contains(name) || group.HasParent()) {
		return false;
	}

	// 親子接続の失敗時は名前登録も取り消し、半端な所属状態を回避
	elements_.emplace(name, GroupReference{ &group, group.lifetime_ });
	if (!group.SetParent(*this)) {
		elements_.erase(name);
		return false;
	}
	return true;
}

Sprite* UIGroup::TryGetSprite(const std::string& name) {
	return const_cast<Sprite*>(std::as_const(*this).TryGetSprite(name));
}

const Sprite* UIGroup::TryGetSprite(const std::string& name) const {
	const auto it = elements_.find(name);

	// 名前一致だけでは型を保証できないため登録型も確認
	if (it == elements_.end() || !std::holds_alternative<SpriteHandle>(it->second)) {
		return nullptr;
	}
	return static_cast<const Sprite*>(Resolve(it->second));
}

Text* UIGroup::TryGetText(const std::string& name) {
	return const_cast<Text*>(std::as_const(*this).TryGetText(name));
}

const Text* UIGroup::TryGetText(const std::string& name) const {
	const auto it = elements_.find(name);

	// 型違いの名前アクセスは失敗として扱い、呼び出し側の分岐を統一
	if (it == elements_.end() || !std::holds_alternative<TextHandle>(it->second)) {
		return nullptr;
	}
	return static_cast<const Text*>(Resolve(it->second));
}

UIGroup* UIGroup::TryGetGroup(const std::string& name) {
	return const_cast<UIGroup*>(std::as_const(*this).TryGetGroup(name));
}

const UIGroup* UIGroup::TryGetGroup(const std::string& name) const {
	const auto it = elements_.find(name);

	// 子グループの破棄後も名前登録が残るため寿命確認を含めて解決
	if (it == elements_.end() || !std::holds_alternative<GroupReference>(it->second)) {
		return nullptr;
	}
	return static_cast<const UIGroup*>(Resolve(it->second));
}

bool UIGroup::Remove(const std::string& name) {
	const auto it = elements_.find(name);
	if (it == elements_.end()) {
		return false;
	}

	// 対象が既に破棄されていても登録名を解放し、同名の再登録を許可
	if (TransformNode2D* node = Resolve(it->second)) {
		node->ClearParent();
	}
	elements_.erase(it);
	return true;
}

void UIGroup::Clear() {

	// 解除対象のローカル値とリソース寿命を維持し、直接の所属だけを解除
	for (const auto& [name, element] : elements_) {
		(void)name;
		if (TransformNode2D* node = Resolve(element)) {
			node->ClearParent();
		}
	}
	elements_.clear();
}

bool UIGroup::RenameElement(const std::string& name, const std::string& newName) {

	// 親子の接続を変更せず名前索引だけを更新
	if (newName.empty() || elements_.contains(newName) || !elements_.contains(name)) {
		return false;
	}
	auto element = elements_.extract(name);
	element.key() = newName;
	elements_.insert(std::move(element));
	return true;
}

TransformNode2D* UIGroup::Resolve(const Element& element) const {
	TransformNode2D* node = nullptr;

	// Manager所有の対象は世代付きHandleで毎回検証し、再利用Slotとの混同を防止
	if (const auto* handle = std::get_if<SpriteHandle>(&element)) {
		node = SpriteManager::GetInstance().TryGet(*handle);
	} else if (const auto* handle = std::get_if<TextHandle>(&element)) {
		node = TextManager::GetInstance().TryGet(*handle);
	} else if (const auto* reference = std::get_if<GroupReference>(&element)) {

		// 非所有の子グループはアドレス参照より先に寿命情報を確認
		if (!reference->lifetime.expired()) {
			node = reference->group;
		}
	}

	if (node && node->IsParent(*this)) {
		return node;
	}
	return nullptr;
}

} // namespace MadoEngine
