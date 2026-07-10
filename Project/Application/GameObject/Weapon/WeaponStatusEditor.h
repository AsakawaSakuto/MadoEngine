#pragma once
#include "WeaponStatus.h"
#include <string>

namespace Weapon {

	/// @brief 武器初期ステータス編集用のImGuiを管理するクラス
	class StatusEditor {
	public:
		/// @brief 武器初期ステータス編集用のImGuiを描画します。
		void DrawImGui();

	private:
		/// @brief 編集中の武器初期ステータスをJsonへ保存します。
		/// @return 保存に成功した場合はtrueを返します。
		bool SaveToJson() const;

		/// @brief 入力名のJsonがあれば読み込み、なければ現在値で作成します。
		/// @return 読込または作成に成功した場合はtrueを返します。
		bool LoadOrCreateJson();

		/// @brief 入力名から武器初期ステータスJsonの保存先を取得します。
		/// @return Jsonファイルの保存先パスです。
		std::string GetJsonFilePath() const;

		UpgradeStatus editingStatus_;
		char statusName_[64] = "Pistol";
	};
}
