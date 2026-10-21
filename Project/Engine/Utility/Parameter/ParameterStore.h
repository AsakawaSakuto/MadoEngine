#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <compare>
#include <cstdint>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace MadoEngine {

/// @brief 最小値と最大値を一項目として保持する範囲値
/// @tparam T 整数、小数、Vector型
template<class T>
struct ParameterRange {
	static_assert(std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::uint32_t> ||
		std::is_same_v<T, float> || std::is_same_v<T, Vector2> || std::is_same_v<T, Vector3> ||
		std::is_same_v<T, Vector4>, "範囲登録に未対応のパラメータ型です");
	T minimum{};
	T maximum{};

	/// @brief 範囲の両端を比較
	/// @param other 比較対象の範囲
	/// @return 両端が一致する場合はtrue
	bool operator==(const ParameterRange& other) const = default;
};

template<class T>
inline constexpr bool kIsParameterRange = false;
template<class T>
inline constexpr bool kIsParameterRange<ParameterRange<T>> = true;

/// @brief 列挙型の種類と選択肢を数値に依存せず保持する設定値
struct ParameterEnumValue {
	std::string type;
	std::string name;

	/// @brief 列挙型の種類と選択肢を比較
	/// @param other 比較対象の設定値
	/// @return 種類と選択肢が一致する場合はtrue
	bool operator==(const ParameterEnumValue& other) const = default;
};

/// @brief 列挙型入力欄の選択肢と表示名
struct ParameterEnumChoice {
	std::string name;
	std::string label;
};

/// @brief ゲーム側の列挙値と保存用識別名、表示名の対応
/// @tparam T 登録する列挙型
template<class T>
struct ParameterEnumOption {
	static_assert(std::is_enum_v<T>, "列挙型の選択肢にはenumが必要です");
	T value;
	std::string name;
	std::string label;
};

using ParameterValue = std::variant<std::int32_t, std::uint32_t, float, bool, Vector2, Vector3, Vector4,
	ParameterRange<std::int32_t>, ParameterRange<std::uint32_t>, ParameterRange<float>,
	ParameterRange<Vector2>, ParameterRange<Vector3>, ParameterRange<Vector4>, ParameterEnumValue>;

/// @brief パラメータ入力欄の編集属性
struct ParameterAttributes {
	float dragSpeed = 0.1f;
	std::optional<double> minimum;
	std::optional<double> maximum;
	bool isColor = false;
	bool isAngle = false;
	std::string enumType;
	std::vector<ParameterEnumChoice> enumChoices;

	/// @brief 保存単位から入力欄の表示単位へ変換
	/// @param value 保存単位の数値、角度の場合はラジアン
	/// @return 角度の場合は度数、それ以外は元の数値
	double ToDisplayNumber(double value) const {
		if (isAngle) {
			return value * (180.0 / std::numbers::pi_v<double>);
		}
		return value;
	}

	/// @brief 入力欄の表示単位から保存単位へ変換
	/// @param value 表示単位の数値、角度の場合は度数
	/// @return 角度の場合はラジアン、それ以外は元の数値
	double ToStoredNumber(double value) const {
		if (isAngle) {
			return value * (std::numbers::pi_v<double> / 180.0);
		}
		return value;
	}
};

/// @brief グループパスと項目名による永続識別子
struct ParameterKey {
	std::string group;
	std::string name;
	/// @brief グループパスと項目名による順序比較
	/// @param other 比較対象の識別子
	/// @return 識別子の辞書順による比較結果
	auto operator<=>(const ParameterKey& other) const = default;
};

/// @brief 設定値とコード側の既定値、編集情報
struct ParameterEntry {
	ParameterValue value;
	ParameterValue defaultValue;
	ParameterAttributes attributes;
	int priority = 0;
	std::uint64_t revision = 0;
	bool registered = false;
};

/// @brief ImGuiと描画APIに依存しない調整用設定値の管理
class ParameterStore final {
public:
	/// @brief 空の設定値管理を構築
	ParameterStore() = default;
	ParameterStore(const ParameterStore&) = delete;
	ParameterStore& operator=(const ParameterStore&) = delete;
	ParameterStore(ParameterStore&&) = delete;
	ParameterStore& operator=(ParameterStore&&) = delete;

	/// @brief 項目の登録と既存設定値の維持
	/// @param key 登録先の識別子
	/// @param initialValue コード側の既定値
	/// @param priority 小さい値ほど先に表示する優先順位
	/// @param attributes 入力欄の編集属性
	void Register(const ParameterKey& key, const ParameterValue& initialValue, int priority = 0, const ParameterAttributes& attributes = {});

	/// @brief 項目を処理中だけ参照するための検索
	/// @param key 検索する識別子
	/// @return 存在する場合は項目、存在しない場合はnullptr
	const ParameterEntry* Find(const ParameterKey& key) const;

	/// @brief 有効な同型値の設定と変更番号の更新
	/// @param key 更新する識別子
	/// @param value 編集後の値
	/// @return 値が変更された場合はtrue
	bool SetValue(const ParameterKey& key, const ParameterValue& value);

	/// @brief ルート内の登録済み項目をコード側の既定値へ復元
	/// @param root 復元対象のルートグループ
	/// @return 値が変更された場合はtrue
	bool ResetRoot(const std::string& root);

	/// @brief 検証済み読み込み値をルート単位で反映
	/// @param root 読み込み対象のルートグループ
	/// @param values 読み込んだ項目一覧
	/// @return 全項目の型と値が有効な場合はtrue
	bool RestoreRoot(const std::string& root, const std::map<ParameterKey, ParameterValue>& values);

	/// @brief 全項目の読み取り専用一覧を取得
	/// @return 識別子順の項目一覧
	const std::map<ParameterKey, ParameterEntry>& GetEntries() const { return entries_; }

	/// @brief ルートグループ一覧を表示順で取得
	/// @return 優先順位と名前順のルート一覧
	std::vector<std::string> GetRoots() const;

	/// @brief グループの表示優先順位を設定
	/// @param group グループパス
	/// @param priority 小さい値ほど先に表示する優先順位
	void SetGroupPriority(const std::string& group, int priority);

	/// @brief グループの表示優先順位を取得
	/// @param group グループパス
	/// @return 未指定の場合は0
	int GetGroupPriority(const std::string& group) const;

	/// @brief 保存可能なグループパスか検証
	/// @param group 検証するグループパス
	/// @return 空要素やパス操作文字を含まない場合はtrue
	static bool IsValidGroup(const std::string& group);

	/// @brief グループパスからルート名を取得
	/// @param group グループパス
	/// @return 最上位のグループ名
	static std::string GetRoot(const std::string& group);

private:
	std::map<ParameterKey, ParameterEntry> entries_;
	std::map<std::string, int> groupPriorities_;
	std::uint64_t nextRevision_ = 1;
};

}
