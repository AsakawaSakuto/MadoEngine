#pragma once
#include "GameObject/Rarity.h"
#include "UI/Game/Control/InputControlFeedback.h"
#include "Utility/GameTimer/GameTimer.h"
#include <RenderHeaders.h>
#include <array>
#include <cstddef>
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
		/// @brief Editor配置のカードと決定案内への参照を初期化
		void Initialize();

		/// @brief 描画オブジェクトの参照を解放
		void Finalize();

		/// @brief 獲得表示とEditorプレビューの表示許可を設定
		/// @param isDisplayEnabled レリックカードの表示を許可する場合はtrue
		void SetDisplayEnabled(bool isDisplayEnabled);

		/// @brief 背景を暗くするSpriteの表示許可を設定
		/// @param isBackgroundEnabled 背景の暗転表示を許可する場合はtrue
		void SetBackgroundEnabled(bool isBackgroundEnabled);

		/// @brief 獲得待ちレリックの表示と決定入力を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param relicSystem 更新対象のレリックシステム
		/// @param acceptDecisionInput 決定入力を受け付ける場合はtrue
		void Update(float deltaTime, Relic::System& relicSystem, bool acceptDecisionInput = true);

		/// @brief レリック獲得UIのプレビューを描画
		/// @param relicSystem プレビューに使用するレリックシステム
		void DrawImGui(Relic::System& relicSystem);

	private:
		/// @brief Relic内のCardViewから現在の描画要素を再取得
		void RefreshCardBindings();

		/// @brief 表示内容をレリック定義へ同期
		/// @param definition 表示対象のレリック定義
		void SetDefinition(const Relic::Definition& definition);

		/// @brief 全構成要素の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief カードの表示状態へRelic Group内の決定案内と押下演出を同期
		/// @param deltaTime 停止中も進行する実時間
		void UpdateDecisionControl(float deltaTime);

		/// @brief 決定後の待機時間と獲得対象の保持状態を初期化
		void ResetDecisionDelay();

		std::array<MadoEngine::SpriteHandle, 3> panelSprites_{};
		MadoEngine::SpriteHandle iconSprite_{};
		std::array<MadoEngine::TextHandle, 3> textHandles_{};
		InputControlFeedback decisionControlFeedback_;
		GameTimer decisionDelayTimer_;
		std::string displayedRelicId_;
		std::string decisionRelicId_;
		std::size_t previewRelicIndex_ = 0;
		std::uint64_t displayedRevision_ = 0;
		std::uint64_t decisionRevision_ = 0;
		bool isPreviewEnabled_ = false;
		bool isDisplayEnabled_ = true;
		bool isBackgroundEnabled_ = true;
		bool isVisible_ = false;
		bool isInitialized_ = false;
	};

}
