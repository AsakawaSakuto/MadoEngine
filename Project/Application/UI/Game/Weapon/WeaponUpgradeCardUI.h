#pragma once
#include <RenderHeaders.h>
#include "Utility/GameTimer/GameTimer.h"
#include <array>
#include <cstddef>

namespace Weapon {

	struct UpgradeChoice;
}

namespace UI::Game {

	/// @brief 武器アップグレード候補を1枚のカードとして表示するクラス
	class WeaponUpgradeCardUI {
	public:
		/// @brief カードを初期化
		/// @param cardIndex 左から数えたカード番号
		void Initialize(std::size_t cardIndex);

		/// @brief カードが所有する描画オブジェクトを破棄
		void Finalize();

		/// @brief 表示するアップグレード候補を設定
		/// @param choice 表示対象のアップグレード候補
		void SetChoice(const Weapon::UpgradeChoice& choice);

		/// @brief カードの選択状態を設定
		/// @param isSelected 選択中の場合はtrue
		void SetSelected(bool isSelected);

		/// @brief カードの表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 選択中カードの表示演出を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 選択決定時の上昇演出を開始
		void PlayDecisionAnimation();

		/// @brief 選択決定時の上昇演出を初期状態へ復元
		void ResetDecisionAnimation();

		/// @brief 選択決定時の上昇演出が完了したか確認
		/// @return 上昇演出が完了した場合はtrue
		bool IsDecisionAnimationFinished() const;

	private:
		enum class CardSpriteType {
			Border,
			Background,
			IconBorder,
			IconBackground,
			Count,
		};

		/// @brief カードの配置を設定
		void ApplyLayout();

		/// @brief カード全体へ選択状態の拡大率を適用
		/// @param scale カード全体へ適用する拡大率
		void ApplySelectionScale(float scale);

		/// @brief 管理する描画オブジェクトの表示状態を反映
		void ApplyVisibility();

		static constexpr std::size_t kCardSpriteCount =
			static_cast<std::size_t>(CardSpriteType::Count);

		std::size_t cardIndex_ = 0;
		std::array<MadoEngine::SpriteHandle, kCardSpriteCount> cardSprites_{};
		MadoEngine::SpriteHandle cardIconSprite_{};
		MadoEngine::TextHandle weaponNameText_{}; // 武器名とカテゴリを表示するテキスト
		MadoEngine::TextHandle categoryText_{};   // カテゴリ名とレアリティを表示するテキスト
		MadoEngine::TextHandle detailText_{};     // ステータス変化量や説明を表示するテキスト
		MadoEngine::TextHandle selectionText_{};  // 選択中を表示するテキスト
		GameTimer scaleTransitionTimer_;
		GameTimer selectedPulseTimer_;
		GameTimer decisionAnimationTimer_;
		float scaleTransitionStart_ = 1.0f;
		float currentScale_ = 1.0f;
		float decisionOffsetY_ = 0.0f;
		Vector4 accentColor_ = { 0.35f, 0.38f, 0.45f, 1.0f }; // 未選択時のカード枠とアイコン枠の色
		Vector4 backgroundColor_ = { 0.055f, 0.07f, 0.11f, 0.96f }; // カード本体の背景色
		bool isSelected_ = false;
		bool isDecisionAnimationPlaying_ = false;
		bool isVisible_ = false;
		bool isInitialized_ = false;
	};
}
