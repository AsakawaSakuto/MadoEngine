#pragma once
#include "RelicDefinition.h"
#include <array>
#include <cstddef>
#include <vector>

namespace Relic {

	class System;

	/// @brief レリック定義編集用のImGuiを管理するクラス
	class Editor {
	public:
		/// @brief 現在のレリック定義から編集状態を初期化
		/// @param relicSystem 初期値を取得するレリックシステム
		void Initialize(const System& relicSystem);

		/// @brief レリック定義編集用のImGuiを描画
		/// @param relicSystem 保存と再読み込みの対象システム
		void DrawImGui(System& relicSystem);

		/// @brief 編集中のレリック定義を保存して適用
		/// @param relicSystem 保存と適用の対象システム
		/// @return 保存と適用に成功した場合はtrue
		bool Save(System& relicSystem) const;

		/// @brief JSONからレリック定義を再読み込み
		/// @param relicSystem 再読み込みの対象システム
		/// @return 再読み込みに成功した場合はtrue
		bool Reload(System& relicSystem);

	private:
		/// @brief 選択中定義の文字列を入力バッファへ同期
		void SynchronizeTextBuffers();

		/// @brief 入力バッファの文字列を選択中定義へ反映
		void ApplyTextBuffers();

		/// @brief 重複しない新規レリックIDを生成
		/// @return 新規レリック用ID
		Id CreateUniqueId() const;

		std::vector<Definition> editingDefinitions_;
		std::size_t selectedIndex_ = 0;
		std::array<char, 64> idBuffer_{};
		std::array<char, 128> displayNameBuffer_{};
		std::array<char, 256> descriptionBuffer_{};
		std::array<char, 128> textureNameBuffer_{};
	};

}
