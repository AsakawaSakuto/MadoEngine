#pragma once
#include "RelicDefinition.h"
#include <unordered_map>

namespace Relic {

	/// @brief レリックの種類別所持数を管理するクラス
	class Inventory {
	public:
		/// @brief レリックを一個追加
		/// @param definition 追加対象のレリック定義
		/// @return 所持上限未満で追加できた場合はtrue
		bool Add(const Definition& definition);

		/// @brief 指定レリックを一個破棄
		/// @param id 破棄対象のレリックID
		/// @return 一個以上所持していて破棄できた場合はtrue
		bool Remove(const Id& id);

		/// @brief 指定レリックの所持数を取得
		/// @param id 取得対象のレリックID
		/// @return 現在の所持数
		int GetCount(const Id& id) const;

		/// @brief 指定レリックを追加できるか確認
		/// @param definition 確認対象のレリック定義
		/// @return 所持上限未満の場合はtrue
		bool CanAdd(const Definition& definition) const;

		/// @brief 全所持数を取得
		/// @return レリックIDと所持数の対応表
		const std::unordered_map<Id, int>& GetAllCounts() const { return counts_; }

	private:
		std::unordered_map<Id, int> counts_;
	};

}
