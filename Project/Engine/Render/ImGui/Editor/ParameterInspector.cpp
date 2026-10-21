#include "ParameterInspector.h"
#ifdef USE_IMGUI
#include "EditorToolbar.h"
#include "ImGuiHeaders.h"
#include "Utility/Parameter/ParameterSerializer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <type_traits>

namespace {

/// @brief 入力欄に使用する型ごとの上下限を算出
/// @tparam T 入力する数値型
/// @param attributes 編集属性
/// @param minimum 下限の出力先
/// @param maximum 上限の出力先
template<class T>
void GetBounds(const MadoEngine::ParameterAttributes& attributes, T& minimum, T& maximum) {
	double lower = static_cast<double>((std::numeric_limits<T>::lowest)());
	double upper = static_cast<double>((std::numeric_limits<T>::max)());
	if (attributes.minimum) {
		lower = (std::max)(lower, *attributes.minimum);
	}
	if (attributes.maximum) {
		upper = (std::min)(upper, *attributes.maximum);
	}
	if constexpr (std::is_integral_v<T>) {
		lower = std::ceil(lower);
		upper = std::floor(upper);
	}
	minimum = static_cast<T>(lower);
	maximum = static_cast<T>(upper);
}

/// @brief ラジアン値を一時的に度数へ変換した入力欄を描画
/// @param components ラジアン単位の成分配列
/// @param count 編集する成分数
/// @param attributes 上下限はラジアン、ドラッグ速度は度数の編集属性
/// @return UI上で変更された場合はtrue
bool DrawAngle(float* components, int count, const MadoEngine::ParameterAttributes& attributes) {
	float minimum = 0.0f;
	float maximum = 0.0f;
	GetBounds(attributes, minimum, maximum);
	const double displayMinimum = attributes.ToDisplayNumber(minimum);
	const double displayMaximum = attributes.ToDisplayNumber(maximum);
	double degrees[4] = {};
	for (int index = 0; index < count; ++index) {
		degrees[index] = attributes.ToDisplayNumber(components[index]);
	}

	// floatの最大ラジアン値も表示変換で桁あふれしないよう度数の入力にはdoubleを使用
	if (!ImGui::DragScalarN("##Value", ImGuiDataType_Double, degrees, count, attributes.dragSpeed,
		&displayMinimum, &displayMaximum, "%.3f°", ImGuiSliderFlags_AlwaysClamp)) {
		return false;
	}
	for (int index = 0; index < count; ++index) {
		const double radians = attributes.ToStoredNumber(degrees[index]);
		components[index] = static_cast<float>(std::clamp(radians, static_cast<double>(minimum), static_cast<double>(maximum)));
	}
	return true;
}

/// @brief 操作中の範囲端点を反対側の端点で制限
/// @tparam T 数値型またはVector型
/// @param value 操作中の端点
/// @param opposite 反対側の端点
/// @param isMinimum 最小値の操作の場合はtrue
template<class T>
void ClampRangeEndpoint(T& value, const T& opposite, bool isMinimum) {
	if constexpr (std::is_arithmetic_v<T>) {
		if (isMinimum) {
			value = (std::min)(value, opposite);
		} else {
			value = (std::max)(value, opposite);
		}
	} else {

		// ドラッグ中の端点入れ替わりを避け、操作している軸だけを境界へ制限
		ClampRangeEndpoint(value.x, opposite.x, isMinimum);
		ClampRangeEndpoint(value.y, opposite.y, isMinimum);
		if constexpr (!std::is_same_v<T, Vector2>) {
			ClampRangeEndpoint(value.z, opposite.z, isMinimum);
		}
		if constexpr (std::is_same_v<T, Vector4>) {
			ClampRangeEndpoint(value.w, opposite.w, isMinimum);
		}
	}
}

/// @brief 設定値の型と用途に合わせた入力欄を描画
/// @param value 編集中の値
/// @param attributes 編集属性
/// @return UI上で変更された場合はtrue
bool DrawValue(MadoEngine::ParameterValue& value, const MadoEngine::ParameterAttributes& attributes) {
	return std::visit([&attributes](auto& typedValue) {
		using T = std::decay_t<decltype(typedValue)>;
		if constexpr (MadoEngine::kIsParameterRange<T>) {
			using Component = std::decay_t<decltype(typedValue.minimum)>;
			bool changed = false;
			ImGui::PushID("Minimum");
			ImGui::TextUnformatted("最小");
			ImGui::SetNextItemWidth(-1.0f);
			MadoEngine::ParameterValue minimum = typedValue.minimum;
			if (DrawValue(minimum, attributes)) {
				typedValue.minimum = std::get<Component>(minimum);
				ClampRangeEndpoint(typedValue.minimum, typedValue.maximum, true);
				changed = true;
			}
			ImGui::PopID();
			ImGui::PushID("Maximum");
			ImGui::TextUnformatted("最大");
			ImGui::SetNextItemWidth(-1.0f);
			MadoEngine::ParameterValue maximum = typedValue.maximum;
			if (DrawValue(maximum, attributes)) {
				typedValue.maximum = std::get<Component>(maximum);
				ClampRangeEndpoint(typedValue.maximum, typedValue.minimum, false);
				changed = true;
			}
			ImGui::PopID();
			return changed;
		} else if constexpr (std::is_same_v<T, MadoEngine::ParameterEnumValue>) {
			std::string preview = typedValue.name;
			for (const auto& choice : attributes.enumChoices) {
				if (choice.name == typedValue.name) {
					preview = choice.label;
					break;
				}
			}
			if (attributes.enumChoices.empty()) {

				// コード側の選択肢が未登録の保存値は表示だけに限定
				ImGui::TextDisabled("%s", preview.c_str());
				return false;
			}
			bool changed = false;
			if (ImGui::BeginCombo("##Value", preview.c_str())) {
				for (const auto& choice : attributes.enumChoices) {
					const bool selected = choice.name == typedValue.name;
					ImGui::PushID(choice.name.c_str());
					if (ImGui::Selectable(choice.label.c_str(), selected) && !selected) {
						typedValue.name = choice.name;
						changed = true;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			return changed;
		} else if constexpr (std::is_same_v<T, bool>) {
			return ImGui::Checkbox("##Value", &typedValue);
		} else if constexpr (std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::uint32_t>) {
			T minimum = 0;
			T maximum = 0;
			GetBounds(attributes, minimum, maximum);
			ImGuiDataType type = ImGuiDataType_S32;
			if constexpr (std::is_same_v<T, std::uint32_t>) {
				type = ImGuiDataType_U32;
			}
			return ImGui::DragScalar("##Value", type, &typedValue, attributes.dragSpeed, &minimum, &maximum, nullptr, ImGuiSliderFlags_AlwaysClamp);
		} else {
			float minimum = 0.0f;
			float maximum = 0.0f;
			GetBounds(attributes, minimum, maximum);
			if constexpr (std::is_same_v<T, float>) {
				if (attributes.isAngle) {
					return DrawAngle(&typedValue, 1, attributes);
				}
				return ImGui::DragFloat("##Value", &typedValue, attributes.dragSpeed, minimum, maximum, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			} else {

				// エンジンのVector内部レイアウトに依存しない一時配列を使用
				float components[4] = { typedValue.x, typedValue.y, 0.0f, 0.0f };
				bool changed = false;
				if constexpr (std::is_same_v<T, Vector2>) {
					if (attributes.isAngle) {
						changed = DrawAngle(components, 2, attributes);
					} else {
						changed = ImGui::DragFloat2("##Value", components, attributes.dragSpeed, minimum, maximum, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					}
				} else if constexpr (std::is_same_v<T, Vector3>) {
					components[2] = typedValue.z;
					if (attributes.isAngle) {
						changed = DrawAngle(components, 3, attributes);
					} else if (attributes.isColor) {
						changed = ImGui::ColorEdit3("##Value", components);
					} else {
						changed = ImGui::DragFloat3("##Value", components, attributes.dragSpeed, minimum, maximum, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					}
				} else {
					components[2] = typedValue.z;
					components[3] = typedValue.w;
					if (attributes.isAngle) {
						changed = DrawAngle(components, 4, attributes);
					} else if (attributes.isColor) {
						changed = ImGui::ColorEdit4("##Value", components);
					} else {
						changed = ImGui::DragFloat4("##Value", components, attributes.dragSpeed, minimum, maximum, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					}
				}
				if (changed) {
					typedValue.x = components[0];
					typedValue.y = components[1];
					if constexpr (!std::is_same_v<T, Vector2>) {
						typedValue.z = components[2];
					}
					if constexpr (std::is_same_v<T, Vector4>) {
						typedValue.w = components[3];
					}
				}
				return changed;
			}
		}
	}, value);
}

/// @brief 一グループの項目と子グループを表示順に描画
/// @param store 編集対象の設定値管理
/// @param group 表示するグループパス
/// @return 設定値が変更された場合はtrue
bool DrawGroup(MadoEngine::ParameterStore& store, const std::string& group) {
	ImGui::PushID(group.c_str());
	const std::size_t separator = group.rfind('/');
	std::string label = group;
	if (separator != std::string::npos) {
		label = group.substr(separator + 1);
	}
	bool changed = false;
	if (ImGui::TreeNodeEx("##Group", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label.c_str())) {
		if (separator == std::string::npos) {
			if (ImGui::SmallButton("このグループを初期値へ")) {
				changed = store.ResetRoot(group);
			}
		}
		std::vector<MadoEngine::ParameterKey> items;
		std::set<std::string> children;
		const std::string prefix = group + "/";
		for (const auto& [key, entry] : store.GetEntries()) {
			if (key.group == group) {
				items.push_back(key);
			} else if (key.group.starts_with(prefix)) {
				const std::size_t end = key.group.find('/', prefix.size());
				children.insert(key.group.substr(0, end));
			}
		}
		std::sort(items.begin(), items.end(), [&store](const auto& left, const auto& right) {
			if (store.Find(left)->priority != store.Find(right)->priority) {
				return store.Find(left)->priority < store.Find(right)->priority;
			}
			return left.name < right.name;
		});
		if (!items.empty() && ImGui::BeginTable("Items", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			ImGui::TableSetupColumn("設定値", ImGuiTableColumnFlags_WidthStretch, 0.6f);
			for (const auto& key : items) {
				const auto& entry = *store.Find(key);
				auto value = entry.value;
				ImGui::PushID(key.name.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(key.name.c_str());
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1.0f);
				if (DrawValue(value, entry.attributes)) {
					changed = store.SetValue(key, value) || changed;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		// 途中のグループに項目がなくても子孫のパスから階層を構築
		std::vector<std::string> sortedChildren(children.begin(), children.end());
		std::sort(sortedChildren.begin(), sortedChildren.end(), [&store](const auto& left, const auto& right) {
			if (store.GetGroupPriority(left) != store.GetGroupPriority(right)) {
				return store.GetGroupPriority(left) < store.GetGroupPriority(right);
			}
			return left < right;
		});
		for (const std::string& child : sortedChildren) {
			changed = DrawGroup(store, child) || changed;
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
	return changed;
}

}

namespace MadoEngine::Editor {

void ParameterInspector::Draw(ParameterStore& store, const std::filesystem::path& directory) {
	ImGui::SetNextWindowSize(ImVec2(600.0f, 650.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Debug Parameters")) {
		ImGui::End();
		return;
	}
	EditorToolbar& toolbar = EditorToolbar::GetInstance();
	if (ImGui::Button("保存")) {
		const bool succeeded = ParameterSerializer::SaveAll(store, directory);
		if (succeeded) {
			toolbar.MarkDocumentSaved(EditorDocument::DebugParameter);
		}
		toolbar.NotifyOperationResult("調整パラメータの保存", succeeded);
	}
	ImGui::SameLine();
	if (ImGui::Button("再読込")) {
		const bool succeeded = ParameterSerializer::LoadAll(store, directory);
		if (succeeded) {

			// ファイル復元を編集履歴として扱わず、保存済みの基準へ同期
			toolbar.SuppressCurrentDocumentHistory();
			toolbar.MarkDocumentSaved(EditorDocument::DebugParameter);
		}
		toolbar.NotifyOperationResult("調整パラメータの再読込", succeeded);
	}
	ImGui::TextWrapped("編集結果は次回のゲーム更新時に反映されます");
	ImGui::Separator();
	bool changed = false;
	const auto roots = store.GetRoots();
	if (roots.empty()) {
		ImGui::TextDisabled("登録されたパラメータはありません");
	}
	for (const std::string& root : roots) {
		changed = DrawGroup(store, root) || changed;
	}
	if (changed) {
		toolbar.MarkDocumentDirty(EditorDocument::DebugParameter);
	}
	ImGui::End();
}

}
#endif // USE_IMGUI
