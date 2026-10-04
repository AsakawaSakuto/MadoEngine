#pragma once
#include <RenderHeaders.h>
#include <array>
#include <cstdint>
#include <string>

namespace Relic {
	class System;
	struct Definition;
}

namespace UI::Game {

	/// @brief 宝箱から獲得する一個のレリックを表示するUI
	class RelicAcquisitionUI {
	public:
		/// @brief 描画オブジェクトを初期化
		void Initialize();

		/// @brief 描画オブジェクトの参照を解放
		void Finalize();

		/// @brief 獲得待ちレリックの表示と決定入力を更新
		/// @param relicSystem 更新対象のレリックシステム
		void Update(Relic::System& relicSystem);

	private:
		/// @brief 表示内容をレリック定義へ同期
		/// @param definition 表示対象のレリック定義
		/// @param ownedCount 獲得前の所持数
		void SetDefinition(const Relic::Definition& definition, int ownedCount);

		/// @brief 全構成要素の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		std::array<MadoEngine::SpriteHandle, 3> panelSprites_{};
		MadoEngine::SpriteHandle iconSprite_{};
		std::array<MadoEngine::TextHandle, 5> textHandles_{};
		std::string displayedRelicId_;
		std::uint64_t displayedRevision_ = 0;
		bool isVisible_ = false;
		bool isInitialized_ = false;
	};

}
