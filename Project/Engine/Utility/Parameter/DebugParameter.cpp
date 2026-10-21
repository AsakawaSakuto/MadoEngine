#include "DebugParameter.h"
#include <stdexcept>

namespace MadoEngine {

DebugParameter::DebugParameter(ParameterStore& store, std::string root)
	: store_(store), root_(std::move(root)) {
	if (!ParameterStore::IsValidGroup(root_) || ParameterStore::GetRoot(root_) != root_) {
		throw std::invalid_argument("パラメータのルートグループ名が不正です");
	}
}

void DebugParameter::RegisterEaseType(const std::string& name, EaseType& value, int priority, const std::string& subgroup) {
	RegisterEnum<EaseType>(name, value, "EaseType", {
		{ EaseType::Linear, "Linear", {} },
		{ EaseType::EaseInQuad, "EaseInQuad", {} },
		{ EaseType::EaseOutQuad, "EaseOutQuad", {} },
		{ EaseType::EaseInOutQuad, "EaseInOutQuad", {} },
		{ EaseType::EaseOutInQuad, "EaseOutInQuad", {} },
		{ EaseType::EaseInCubic, "EaseInCubic", {} },
		{ EaseType::EaseOutCubic, "EaseOutCubic", {} },
		{ EaseType::EaseInOutCubic, "EaseInOutCubic", {} },
		{ EaseType::EaseOutInCubic, "EaseOutInCubic", {} },
		{ EaseType::EaseInQuart, "EaseInQuart", {} },
		{ EaseType::EaseOutQuart, "EaseOutQuart", {} },
		{ EaseType::EaseInOutQuart, "EaseInOutQuart", {} },
		{ EaseType::EaseOutInQuart, "EaseOutInQuart", {} },
		{ EaseType::EaseInQuint, "EaseInQuint", {} },
		{ EaseType::EaseOutQuint, "EaseOutQuint", {} },
		{ EaseType::EaseInOutQuint, "EaseInOutQuint", {} },
		{ EaseType::EaseOutInQuint, "EaseOutInQuint", {} },
		{ EaseType::EaseInSine, "EaseInSine", {} },
		{ EaseType::EaseOutSine, "EaseOutSine", {} },
		{ EaseType::EaseInOutSine, "EaseInOutSine", {} },
		{ EaseType::EaseOutInSine, "EaseOutInSine", {} },
		{ EaseType::EaseInExpo, "EaseInExpo", {} },
		{ EaseType::EaseOutExpo, "EaseOutExpo", {} },
		{ EaseType::EaseInOutExpo, "EaseInOutExpo", {} },
		{ EaseType::EaseOutInExpo, "EaseOutInExpo", {} },
		{ EaseType::EaseInCirc, "EaseInCirc", {} },
		{ EaseType::EaseOutCirc, "EaseOutCirc", {} },
		{ EaseType::EaseInOutCirc, "EaseInOutCirc", {} },
		{ EaseType::EaseOutInCirc, "EaseOutInCirc", {} },
		{ EaseType::EaseInBack, "EaseInBack", {} },
		{ EaseType::EaseOutBack, "EaseOutBack", {} },
		{ EaseType::EaseInOutBack, "EaseInOutBack", {} },
		{ EaseType::EaseOutInBack, "EaseOutInBack", {} },
		{ EaseType::EaseInElastic, "EaseInElastic", {} },
		{ EaseType::EaseOutElastic, "EaseOutElastic", {} },
		{ EaseType::EaseInOutElastic, "EaseInOutElastic", {} },
		{ EaseType::EaseOutInElastic, "EaseOutInElastic", {} },
		{ EaseType::EaseInBounce, "EaseInBounce", {} },
		{ EaseType::EaseOutBounce, "EaseOutBounce", {} },
		{ EaseType::EaseInOutBounce, "EaseInOutBounce", {} },
		{ EaseType::EaseOutInBounce, "EaseOutInBounce", {} },
		{ EaseType::None, "None", {} },
	}, priority, subgroup);
}

void DebugParameter::RegisterTransform(Transform3D& transform, const std::string& subgroup) {
	Register("位置", transform.translate, 0, subgroup);
	RegisterAngle("回転", transform.rotate, 1, subgroup);
	Register("拡縮", transform.scale, 2, subgroup);
}

void DebugParameter::RegisterTransform(Transform2D& transform, const std::string& subgroup) {
	Register("位置", transform.translate, 0, subgroup);
	RegisterAngle("回転", transform.rotate, 1, subgroup);
	Register("拡縮", transform.scale, 2, subgroup);
}

void DebugParameter::Apply() {
	for (const auto& [key, binding] : bindings_) {
		binding->Apply(true);
	}
}

bool DebugParameter::ApplyIfDirty() {
	bool applied = false;
	for (const auto& [key, binding] : bindings_) {

		// 各Bindingの反映済み番号を使い、他オブジェクトの変更検出を維持
		applied = binding->Apply(false) || applied;
	}
	return applied;
}

void DebugParameter::Unregister(const std::string& name, const std::string& subgroup) {
	bindings_.erase(MakeKey(name, subgroup));
}

void DebugParameter::ClearBindings() {
	bindings_.clear();
}

ParameterKey DebugParameter::MakeKey(const std::string& name, const std::string& subgroup) const {
	std::string group = root_;
	if (!subgroup.empty()) {
		group += "/" + subgroup;
	}
	return { group, name };
}

}
