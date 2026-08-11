#pragma once
#include "PlayerStatus.h"
#include <optional>

class Model;

namespace Player {

	/// @brief Playerの移動状態から描画Animationを選択するクラス
	class AnimationController {
	public:

		/// @brief Player Modelの標準Animationを設定
		/// @param model Animationを設定するModel
		void Initialize(Model& model);

		/// @brief Playerの移動状態に対応するAnimationへ遷移
		/// @param motion 現在のPlayer移動状態
		/// @param isMoving Playerが水平方向へ移動中の場合はtrue
		/// @param isGrounded Playerが接地中の場合はtrue
		/// @param isJumpStarted このフレームにJumpが成立した場合はtrue
		/// @param model Animationを設定するModel
		void Update(Motion motion, bool isMoving, bool isGrounded, bool isJumpStarted, Model& model);

	private:

		/// @brief Player移動状態に対応するAnimationClip名を取得
		/// @param motion 対応を取得するPlayer移動状態
		/// @return 対応するAnimationClip名
		static const char* GetClipName(Motion motion);

		/// @brief Player移動状態に対応する遷移時間を取得
		/// @param motion 対応を取得するPlayer移動状態
		/// @return 対応する遷移時間
		static float GetBlendDuration(Motion motion);

		std::optional<Motion> currentMotion_;
		bool isJumpAnimationActive_ = false;
		bool isLandingAnimationActive_ = false;
	};
}
