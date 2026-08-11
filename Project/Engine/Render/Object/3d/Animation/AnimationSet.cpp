#include "AnimationSet.h"
#include <utility>

bool AnimationSet::AddClip(std::string name, AnimationClip clip) {
	if (name.empty() || clip.nodeAnimations.empty()) {
		return false;
	}

	clip.name = name;
	clips_.insert_or_assign(name, std::move(clip));
	if (defaultClipName_.empty()) {
		defaultClipName_ = std::move(name);
	}

	return true;
}

const AnimationClip* AnimationSet::FindClip(std::string_view name) const {
	const auto iterator = clips_.find(std::string(name));
	return iterator == clips_.end() ? nullptr : &iterator->second;
}

bool AnimationSet::SetDefaultClip(std::string_view name) {
	if (!FindClip(name)) {
		return false;
	}

	defaultClipName_ = name;
	return true;
}

const AnimationClip* AnimationSet::GetDefaultClip() const {
	return FindClip(defaultClipName_);
}
