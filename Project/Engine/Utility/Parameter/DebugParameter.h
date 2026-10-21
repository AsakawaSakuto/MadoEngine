#pragma once
#include "ParameterStore.h"
#include "Math/Transform.h"
#include "Utility/Easing/EaseType.h"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace MadoEngine {

/// @brief 異なる型の変数への反映を統一するインターフェース
class IParamBinding {
public:
	/// @brief 派生Bindingの破棄
	virtual ~IParamBinding() = default;
	/// @brief 必要に応じた設定値の反映
	/// @param force 変更番号に関係なく反映する場合はtrue
	/// @return 反映した場合はtrue
	virtual bool Apply(bool force) = 0;
};

/// @brief 型付き設定値と変数またはSetterとの結び付け
/// @tparam T 対応する設定値型
template<class T>
class ParamBinding final : public IParamBinding {
public:
	/// @brief 反映先と永続識別子の保持
	/// @param store Bindingより長く保持する設定値管理
	/// @param key 対象項目の識別子
	/// @param setter 設定値の反映先
	ParamBinding(ParameterStore& store, ParameterKey key, std::function<void(const T&)> setter)
		: store_(store), key_(std::move(key)), setter_(std::move(setter)) {}

	/// @brief 変更番号をBinding単位で追跡した設定値の反映
	/// @param force 変更番号に関係なく反映する場合はtrue
	/// @return 型が一致して反映した場合はtrue
	bool Apply(bool force) override {
		const ParameterEntry* entry = store_.Find(key_);
		if (!entry || (!force && entry->revision == appliedRevision_)) {
			return false;
		}
		const T* value = std::get_if<T>(&entry->value);
		if (!value) {
			return false;
		}

		// Setter内で他項目が更新されても今回取得した値と番号を対として反映
		const T snapshot = *value;
		const std::uint64_t revision = entry->revision;
		setter_(snapshot);
		appliedRevision_ = revision;
		return true;
	}

private:
	ParameterStore& store_;
	ParameterKey key_;
	std::function<void(const T&)> setter_;
	std::uint64_t appliedRevision_ = 0;
};

/// @brief オブジェクト寿命内の変数登録と明示的な設定反映
class DebugParameter final {
public:
	/// @brief 登録先の設定値管理とルートグループの保持
	/// @param store DebugParameterより長く保持する設定値管理
	/// @param root ルートグループ名
	DebugParameter(ParameterStore& store, std::string root);
	DebugParameter(const DebugParameter&) = delete;
	DebugParameter& operator=(const DebugParameter&) = delete;
	DebugParameter(DebugParameter&&) = delete;
	DebugParameter& operator=(DebugParameter&&) = delete;

	/// @brief 変数を参照で登録
	/// @tparam T 対応する設定値型
	/// @param name 項目名
	/// @param value 登録先の変数
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	/// @param attributes 入力欄の編集属性
	template<class T>
	void Register(const std::string& name, T& value, int priority = 0, const std::string& subgroup = {}, const ParameterAttributes& attributes = {}) {
		RegisterSetter<T>(name, value, [&value](const T& nextValue) { value = nextValue; }, priority, subgroup, attributes);
	}

	/// @brief カプセル化された更新処理を反映先として登録
	/// @tparam T 対応する設定値型
	/// @param name 項目名
	/// @param initialValue コード側の既定値
	/// @param setter 値の反映と関連状態の更新処理
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	/// @param attributes 入力欄の編集属性
	template<class T>
	void RegisterSetter(const std::string& name, const T& initialValue, std::function<void(const T&)> setter, int priority = 0, const std::string& subgroup = {}, const ParameterAttributes& attributes = {}) {
		static_assert(std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::uint32_t> ||
			std::is_same_v<T, float> || std::is_same_v<T, bool> || std::is_same_v<T, Vector2> ||
			std::is_same_v<T, Vector3> || std::is_same_v<T, Vector4> || kIsParameterRange<T> ||
			std::is_same_v<T, ParameterEnumValue>, "未対応のパラメータ型です");
		if (!setter) {
			throw std::invalid_argument("パラメータの反映先が未設定です");
		}
		const ParameterKey key = MakeKey(name, subgroup);
		store_.Register(key, initialValue, priority, attributes);
		bindings_[key] = std::make_unique<ParamBinding<T>>(store_, key, std::move(setter));
	}

	/// @brief RGBまたはRGBAの色編集属性を付与して登録
	/// @tparam T Vector3またはVector4
	/// @param name 項目名
	/// @param value 登録先の色変数
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	/// @param attributes 入力欄の編集属性
	template<class T>
	void RegisterColor(const std::string& name, T& value, int priority = 0, const std::string& subgroup = {}, const ParameterAttributes& attributes = {}) {
		static_assert(std::is_same_v<T, Vector3> || std::is_same_v<T, Vector4>, "色登録にはVector3またはVector4が必要です");
		ParameterAttributes colorAttributes = attributes;
		colorAttributes.isColor = true;
		colorAttributes.isAngle = false;
		Register(name, value, priority, subgroup, colorAttributes);
	}

	/// @brief 保存値をラジアンに維持して度数表示の角度として登録
	/// @tparam T float、Vector2、Vector3、Vector4
	/// @param name 項目名
	/// @param value ラジアン単位の登録先変数
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	/// @param attributes 上下限はラジアン、ドラッグ速度は度数の編集属性
	template<class T>
	void RegisterAngle(const std::string& name, T& value, int priority = 0, const std::string& subgroup = {}, const ParameterAttributes& attributes = {}) {
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, Vector2> || std::is_same_v<T, Vector3> ||
			std::is_same_v<T, Vector4>, "角度登録にはfloatまたはVector型が必要です");
		ParameterAttributes angleAttributes = attributes;
		angleAttributes.isAngle = true;
		angleAttributes.isColor = false;
		Register(name, value, priority, subgroup, angleAttributes);
	}

	/// @brief 最小値と最大値を一項目として登録して大小関係を保証
	/// @tparam T int32_t、uint32_t、float、Vector2、Vector3、Vector4
	/// @param name 項目名
	/// @param minimum 登録先の最小値変数
	/// @param maximum 登録先の最大値変数
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	/// @param attributes 両端に適用する編集属性
	template<class T>
	void RegisterRange(const std::string& name, T& minimum, T& maximum, int priority = 0, const std::string& subgroup = {}, const ParameterAttributes& attributes = {}) {
		const ParameterRange<T> initialValue{ minimum, maximum };
		RegisterSetter<ParameterRange<T>>(name, initialValue, [&minimum, &maximum](const ParameterRange<T>& range) {
			minimum = range.minimum;
			maximum = range.maximum;
		}, priority, subgroup, attributes);
	}

	/// @brief 列挙型の変数を識別名付きの選択欄として登録
	/// @tparam T 登録する列挙型
	/// @param name 項目名
	/// @param value 登録先の列挙型変数
	/// @param enumType 保存用の列挙型識別名
	/// @param options 列挙値と不変の識別名、表示名の一覧
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	template<class T>
	void RegisterEnum(const std::string& name, T& value, const std::string& enumType,
		std::initializer_list<ParameterEnumOption<T>> options, int priority = 0, const std::string& subgroup = {}) {
		static_assert(std::is_enum_v<T>, "列挙型登録にはenumが必要です");
		if (enumType.empty() || options.size() == 0) {
			throw std::invalid_argument("列挙型の識別名と選択肢が必要です");
		}

		// 呼び出し側の一時的な選択肢一覧から独立した寿命をBindingへ確保
		std::vector<ParameterEnumOption<T>> registeredOptions;
		ParameterAttributes attributes;
		attributes.enumType = enumType;
		ParameterEnumValue initialValue{ enumType, {} };
		for (const auto& option : options) {
			if (option.name.empty() || std::any_of(registeredOptions.begin(), registeredOptions.end(), [&option](const auto& previous) {
				return previous.name == option.name || previous.value == option.value;
			})) {
				throw std::invalid_argument("列挙型の選択肢に空の識別名または重複があります");
			}
			std::string label = option.label;
			if (label.empty()) {
				label = option.name;
			}
			attributes.enumChoices.push_back({ option.name, label });
			registeredOptions.push_back(option);
			if (option.value == value) {
				initialValue.name = option.name;
			}
		}
		if (initialValue.name.empty()) {
			throw std::invalid_argument("列挙型の初期値が選択肢に存在しません");
		}
		RegisterSetter<ParameterEnumValue>(name, initialValue, [&value, registeredOptions](const ParameterEnumValue& selected) {
			for (const auto& option : registeredOptions) {
				if (option.name == selected.name) {
					value = option.value;
					return;
				}
			}
		}, priority, subgroup, attributes);
	}

	/// @brief 全EaseTypeを選択可能なイージング入力欄として登録
	/// @param name 項目名
	/// @param value 登録先のEaseType変数
	/// @param priority 表示優先順位
	/// @param subgroup ルート配下のグループパス
	void RegisterEaseType(const std::string& name, EaseType& value, int priority = 0, const std::string& subgroup = {});

	/// @brief 位置、回転、拡縮の3D用一括登録
	/// @param transform 登録する3D Transform
	/// @param subgroup 登録先のサブグループ
	void RegisterTransform(Transform3D& transform, const std::string& subgroup = "Transform3D");

	/// @brief 位置、回転、拡縮の2D用一括登録
	/// @param transform 登録する2D Transform
	/// @param subgroup 登録先のサブグループ
	void RegisterTransform(Transform2D& transform, const std::string& subgroup = "Transform2D");

	/// @brief 登録した全項目の初期反映または強制再反映
	void Apply();

	/// @brief 未反映の変更を持つ項目だけの反映
	/// @return 反映した項目がある場合はtrue
	bool ApplyIfDirty();

	/// @brief 設定値を保持したまま一項目のBindingを解除
	/// @param name 項目名
	/// @param subgroup 登録先のサブグループ
	void Unregister(const std::string& name, const std::string& subgroup = {});

	/// @brief 設定値を保持したまま全Bindingを解除
	void ClearBindings();

private:
	/// @brief ルートとサブグループから識別子を構築
	/// @param name 項目名
	/// @param subgroup 登録先のサブグループ
	/// @return 完全な識別子
	ParameterKey MakeKey(const std::string& name, const std::string& subgroup) const;

	ParameterStore& store_;
	std::string root_;
	std::map<ParameterKey, std::unique_ptr<IParamBinding>> bindings_;
};

}
