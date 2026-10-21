#include "ParameterStore.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace {

/// @brief ファイル名にも使用可能なパス要素の検証
/// @param name 検証する要素
/// @return パス操作文字や予約名を含まない場合はtrue
bool IsValidSegment(const std::string& name) {
	if (name.empty() || name.back() == '.' || name.back() == ' ' || name.find_first_of("/\\:*?\"<>|") != std::string::npos) {
		return false;
	}
	std::string upper;
	for (unsigned char character : name) {
		if (character < 32) {
			return false;
		}
		if (character >= 'a' && character <= 'z') {
			character = static_cast<unsigned char>(character - 'a' + 'A');
		}
		upper += static_cast<char>(character);
	}

	// Windowsのデバイス名は拡張子付きでも通常ファイルとして使用不可
	upper = upper.substr(0, upper.find('.'));
	if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") {
		return false;
	}
	if (upper.size() == 4 && (upper.starts_with("COM") || upper.starts_with("LPT")) && upper[3] >= '1' && upper[3] <= '9') {
		return false;
	}
	return true;
}

/// @brief 数値を型と編集範囲に収まる有限値へ正規化
/// @tparam T 数値型
/// @param value 正規化する値
/// @param attributes 編集属性
/// @return 型と範囲が有効な場合はtrue
template<class T>
bool NormalizeNumber(T& value, const MadoEngine::ParameterAttributes& attributes) {
	double minimum = static_cast<double>((std::numeric_limits<T>::lowest)());
	double maximum = static_cast<double>((std::numeric_limits<T>::max)());
	if (attributes.minimum) {
		minimum = (std::max)(minimum, *attributes.minimum);
	}
	if (attributes.maximum) {
		maximum = (std::min)(maximum, *attributes.maximum);
	}
	if constexpr (std::is_integral_v<T>) {

		// 小数の境界を整数へ丸め、指定範囲外の整数値生成を防止
		minimum = std::ceil(minimum);
		maximum = std::floor(maximum);
	}
	if (!std::isfinite(static_cast<double>(value)) || minimum > maximum) {
		return false;
	}
	value = static_cast<T>(std::clamp(static_cast<double>(value), minimum, maximum));
	return true;
}

/// @brief 基本型の値と表示用途を検証して範囲内へ正規化
/// @tparam T 基本型またはVector型
/// @param value 正規化する値
/// @param attributes 編集属性
/// @return 値と属性が有効な場合はtrue
template<class T>
bool NormalizeTypedValue(T& value, const MadoEngine::ParameterAttributes& attributes) {
	if constexpr (std::is_same_v<T, MadoEngine::ParameterEnumValue>) {
		if (value.type.empty() || value.name.empty() || attributes.isColor || attributes.isAngle || attributes.minimum || attributes.maximum) {
			return false;
		}
		if (attributes.enumType.empty()) {

			// 登録前のJSON読込では識別名を保持し、選択肢の照合をコード側の登録時まで保留
			return attributes.enumChoices.empty();
		}
		if (value.type != attributes.enumType) {
			return false;
		}
		return std::any_of(attributes.enumChoices.begin(), attributes.enumChoices.end(), [&value](const auto& choice) {
			return choice.name == value.name;
		});
	} else if constexpr (std::is_same_v<T, bool>) {
		return !attributes.isColor && !attributes.isAngle;
	} else if constexpr (std::is_arithmetic_v<T>) {
		if (attributes.isColor || (attributes.isAngle && !std::is_same_v<T, float>)) {
			return false;
		}
		return NormalizeNumber(value, attributes);
	} else {
		if (attributes.isColor && (attributes.isAngle || std::is_same_v<T, Vector2>)) {
			return false;
		}

		// ベクトルも全成分を検証し、不正な一成分の部分反映を防止
		bool valid = NormalizeNumber(value.x, attributes) && NormalizeNumber(value.y, attributes);
		if constexpr (!std::is_same_v<T, Vector2>) {
			valid = NormalizeNumber(value.z, attributes) && valid;
		}
		if constexpr (std::is_same_v<T, Vector4>) {
			valid = NormalizeNumber(value.w, attributes) && valid;
		}
		return valid;
	}
}

/// @brief 逆転した範囲を成分ごとに入れ替えて順序を保証
/// @tparam T 数値型またはVector型
/// @param minimum 最小値
/// @param maximum 最大値
template<class T>
void OrderRange(T& minimum, T& maximum) {
	if constexpr (std::is_arithmetic_v<T>) {
		if (minimum > maximum) {
			std::swap(minimum, maximum);
		}
	} else {

		// 空間範囲は軸ごとの大小関係で整合性を保証
		OrderRange(minimum.x, maximum.x);
		OrderRange(minimum.y, maximum.y);
		if constexpr (!std::is_same_v<T, Vector2>) {
			OrderRange(minimum.z, maximum.z);
		}
		if constexpr (std::is_same_v<T, Vector4>) {
			OrderRange(minimum.w, maximum.w);
		}
	}
}

/// @brief 値と編集属性を検証して範囲内へ正規化
/// @param value 正規化する値
/// @param attributes 編集属性
/// @return 値と属性が有効な場合はtrue
bool NormalizeValue(MadoEngine::ParameterValue& value, const MadoEngine::ParameterAttributes& attributes) {
	if (!std::isfinite(attributes.dragSpeed) || attributes.dragSpeed <= 0.0f ||
		(attributes.minimum && !std::isfinite(*attributes.minimum)) ||
		(attributes.maximum && !std::isfinite(*attributes.maximum)) ||
		(attributes.minimum && attributes.maximum && *attributes.minimum > *attributes.maximum)) {
		return false;
	}
	if (!std::holds_alternative<MadoEngine::ParameterEnumValue>(value) &&
		(!attributes.enumType.empty() || !attributes.enumChoices.empty())) {
		return false;
	}

	// 範囲は両端の検証を完了してから成分ごとの大小関係を確定
	return std::visit([&attributes](auto& typedValue) {
		using T = std::decay_t<decltype(typedValue)>;
		if constexpr (MadoEngine::kIsParameterRange<T>) {
			if (!NormalizeTypedValue(typedValue.minimum, attributes) || !NormalizeTypedValue(typedValue.maximum, attributes)) {
				return false;
			}
			OrderRange(typedValue.minimum, typedValue.maximum);
			return true;
		} else {
			return NormalizeTypedValue(typedValue, attributes);
		}
	}, value);
}

}

namespace MadoEngine {

void ParameterStore::Register(const ParameterKey& key, const ParameterValue& initialValue, int priority, const ParameterAttributes& attributes) {
	ParameterValue normalized = initialValue;
	if (!IsValidGroup(key.group) || key.name.empty() || !NormalizeValue(normalized, attributes)) {
		throw std::invalid_argument("パラメータの識別子、初期値、編集属性が不正です");
	}
	const auto [iterator, inserted] = entries_.try_emplace(key, ParameterEntry{ normalized, normalized, attributes, priority, nextRevision_, true });
	ParameterEntry& entry = iterator->second;
	if (inserted) {
		++nextRevision_;
		return;
	}

	// 再生成時は保存値を維持し、コード側の型変更時だけ既定値へ復元
	ParameterValue previous = entry.value;
	if (previous.index() != normalized.index() || !NormalizeValue(previous, attributes)) {
		previous = normalized;
	}
	entry.defaultValue = normalized;
	entry.attributes = attributes;
	entry.priority = priority;
	entry.registered = true;
	if (entry.value != previous) {
		entry.value = previous;
		entry.revision = nextRevision_++;
	}
}

const ParameterEntry* ParameterStore::Find(const ParameterKey& key) const {
	const auto iterator = entries_.find(key);
	if (iterator == entries_.end()) {
		return nullptr;
	}
	return &iterator->second;
}

bool ParameterStore::SetValue(const ParameterKey& key, const ParameterValue& value) {
	const auto iterator = entries_.find(key);
	if (iterator == entries_.end()) {
		return false;
	}
	ParameterEntry& entry = iterator->second;
	ParameterValue normalized = value;

	// UIと再読込を同じ境界で検証し、変更のない書き込みでは反映番号を維持
	if (entry.value.index() != value.index() || !NormalizeValue(normalized, entry.attributes) || entry.value == normalized) {
		return false;
	}
	entry.value = normalized;
	entry.revision = nextRevision_++;
	return true;
}

bool ParameterStore::ResetRoot(const std::string& root) {
	bool changed = false;
	for (const auto& [key, entry] : entries_) {
		if (GetRoot(key.group) == root && entry.registered) {
			changed = SetValue(key, entry.defaultValue) || changed;
		}
	}
	return changed;
}

bool ParameterStore::RestoreRoot(const std::string& root, const std::map<ParameterKey, ParameterValue>& values) {
	if (!IsValidGroup(root) || GetRoot(root) != root) {
		return false;
	}
	std::map<ParameterKey, ParameterValue> normalizedValues;
	for (const auto& [key, value] : values) {
		ParameterValue normalized = value;
		const ParameterEntry* entry = Find(key);
		ParameterAttributes attributes;
		if (entry) {
			attributes = entry->attributes;
		}
		if (!IsValidGroup(key.group) || GetRoot(key.group) != root || key.name.empty() ||
			(entry && entry->registered && entry->value.index() != value.index()) || !NormalizeValue(normalized, attributes)) {
			return false;
		}
		normalizedValues.emplace(key, normalized);
	}

	// 全件検証後に反映し、不正ファイルによるルート内の部分更新を防止
	for (const auto& [key, entry] : entries_) {
		if (GetRoot(key.group) == root && entry.registered && !normalizedValues.contains(key)) {
			SetValue(key, entry.defaultValue);
		}
	}
	for (const auto& [key, value] : normalizedValues) {
		auto iterator = entries_.find(key);
		if (iterator == entries_.end()) {
			entries_.emplace(key, ParameterEntry{ value, value, {}, 0, nextRevision_++, false });
		} else if (!iterator->second.registered && iterator->second.value.index() != value.index()) {
			iterator->second.value = value;
			iterator->second.defaultValue = value;
			iterator->second.revision = nextRevision_++;
		} else {
			SetValue(key, value);
		}
	}
	return true;
}

std::vector<std::string> ParameterStore::GetRoots() const {
	std::set<std::string> roots;
	for (const auto& [key, entry] : entries_) {
		roots.insert(GetRoot(key.group));
	}
	std::vector<std::string> result(roots.begin(), roots.end());
	std::sort(result.begin(), result.end(), [this](const std::string& left, const std::string& right) {
		if (GetGroupPriority(left) != GetGroupPriority(right)) {
			return GetGroupPriority(left) < GetGroupPriority(right);
		}
		return left < right;
	});
	return result;
}

void ParameterStore::SetGroupPriority(const std::string& group, int priority) {
	if (!IsValidGroup(group)) {
		throw std::invalid_argument("パラメータのグループパスが不正です");
	}
	groupPriorities_[group] = priority;
}

int ParameterStore::GetGroupPriority(const std::string& group) const {
	const auto iterator = groupPriorities_.find(group);
	if (iterator == groupPriorities_.end()) {
		return 0;
	}
	return iterator->second;
}

bool ParameterStore::IsValidGroup(const std::string& group) {
	std::size_t start = 0;

	// 空の階層と保存先を逸脱する要素を各区切りで検証
	while (true) {
		const std::size_t end = group.find('/', start);
		if (!IsValidSegment(group.substr(start, end - start))) {
			return false;
		}
		if (end == std::string::npos) {
			return true;
		}
		start = end + 1;
	}
}

std::string ParameterStore::GetRoot(const std::string& group) {
	return group.substr(0, group.find('/'));
}

}
