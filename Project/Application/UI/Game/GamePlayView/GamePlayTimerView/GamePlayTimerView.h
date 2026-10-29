#pragma once
#include "Math/Vector4.h"
#include "Render/Object/ObjectHandle.h"
#include "Utility/GameTimer/GameTimer.h"

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief 残り時間と制限時間超過後の経過時間をTextへ表示するビュー
	class GamePlayTimerView {
	public:
		/// @brief 時間表示の参照と更新状態を初期化
		void Initialize();

		/// @brief 残り時間と超過時間の表示および超過開始演出を更新
		/// @param remainingTime 残り時間（秒）、制限時間超過後は超過秒数の負値
		/// @param deltaTime フェード演出用の実経過時間（秒）
		void Update(float remainingTime, float deltaTime);

		/// @brief 時間表示の文字色と更新状態を復元
		void Finalize();

	private:
		/// @brief 超過開始の効果音とTextのフェード表示を更新
		/// @param remainingTime 残り時間（秒）、制限時間超過後は超過秒数の負値
		/// @param deltaTime フェード演出用の実経過時間（秒）
		void UpdateOvertimePresentation(float remainingTime, float deltaTime);

		/// @brief 超過開始演出を初期化しTextを非表示へ復元
		void ResetOvertimePresentation();

		MadoEngine::TextHandle timerText_;
		Vector4 normalColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		GameTimer overtimeFadeTimer_;
		int displayedSeconds_ = -1;
		bool isOvertime_ = false;
		bool hasOvertimeStarted_ = false;
	};
}
