#pragma once
#include <RenderHeaders.h>
#include "Utility/GameTimer/GameTimer.h"
#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace Weapon {

	struct UpgradeChoice;
}

namespace UI::Game {

	/// @brief 武器アップグレード候補を1枚のカードとして表示するクラス
	class WeaponUpgradeCardUI {
	public:
		/// @brief Editor管理のカードGroupへの参照を初期化
		/// @param cardIndex 左から数えたカード番号
		void Initialize(std::size_t cardIndex);

		/// @brief Editor管理の描画要素を維持した参照と演出状態の解放
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
			LegendaryBackground,
			IconBorder,
			IconBackground,
			Count,
		};

		/// @brief 再読込後の要素にも反映するカードの表示内容
		struct CardContent {
			std::string textureName;
			std::string weaponName;
			std::string category;
			std::string detail;
			std::string newWeaponDescription;
			std::string currentValue;
			std::string upgradedValue;
		};

		/// @brief Upgrade内のカードGroupから現在の描画要素を再取得
		void RefreshBindings();

		/// @brief 候補内容とレアリティ色を現在の描画要素へ同期
		void ApplyChoiceContent();

		/// @brief カード全体へ選択状態の拡大率を適用
		/// @param scale カード全体へ適用する拡大率
		void ApplySelectionScale(float scale);

		/// @brief 管理する描画オブジェクトの表示状態を反映
		void ApplyVisibility();

		static constexpr std::size_t kCardSpriteCount =
			static_cast<std::size_t>(CardSpriteType::Count);

		std::size_t cardIndex_ = 0;
		MadoEngine::UIGroupHandle cardGroup_{};
		std::optional<CardContent> content_;
		std::array<MadoEngine::SpriteHandle, kCardSpriteCount> cardSprites_{};
		MadoEngine::SpriteHandle cardIconSprite_{};
		MadoEngine::TextHandle weaponNameText_{}; // 武器名とカテゴリを表示するテキスト
		MadoEngine::TextHandle categoryText_{};   // カテゴリ名とレアリティを表示するテキスト
		MadoEngine::TextHandle newWeaponDescriptionText_{};
		MadoEngine::TextHandle selectionText_{};  // 選択中を表示するテキスト
		GameTimer scaleTransitionTimer_;
		GameTimer selectedPulseTimer_;
		GameTimer decisionAnimationTimer_;
		float scaleTransitionStart_ = 1.0f;
		float currentScale_ = 1.0f;
		float decisionOffsetY_ = 0.0f;
		Vector4 accentColor_ = { 0.35f, 0.38f, 0.45f, 1.0f }; // 未選択時のカード枠とアイコン枠の色
		bool isSelected_ = false;
		bool isDecisionAnimationPlaying_ = false;
		bool isOwnedWeaponUpgrade_ = false;
		bool isLegendary_ = false;
		bool isVisible_ = false;
		bool isInitialized_ = false;
	};
}
