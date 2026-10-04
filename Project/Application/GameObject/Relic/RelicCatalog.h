#pragma once
#include "RelicDefinition.h"
#include <string>
#include <vector>

namespace Relic {

	/// @brief JSONからレリック定義を管理するカタログ
	class Catalog {
	public:
		/// @brief レリック定義をJSONから読み込み
		/// @param filePath 読み込むJSONファイルパス
		/// @return 一件以上の定義を読み込めた場合はtrue
		bool Load(const std::string& filePath);

		/// @brief 現在のレリック定義をJSONへ保存
		/// @param filePath 保存先のJSONファイルパス
		/// @return 保存に成功した場合はtrue
		bool Save(const std::string& filePath) const;

		/// @brief 検証済みのレリック定義一覧へ置換
		/// @param definitions 置換対象のレリック定義一覧
		/// @return 全定義が有効な場合はtrue
		bool Replace(std::vector<Definition> definitions);

		/// @brief IDに対応するレリック定義を取得
		/// @param id 検索対象のレリックID
		/// @return 定義が存在する場合はconstポインター、存在しない場合はnullptr
		const Definition* Find(const Id& id) const;

		/// @brief 全レリック定義を取得
		/// @return 全レリック定義へのconst参照
		const std::vector<Definition>& GetAll() const { return definitions_; }

	private:
		std::vector<Definition> definitions_;
	};

}
