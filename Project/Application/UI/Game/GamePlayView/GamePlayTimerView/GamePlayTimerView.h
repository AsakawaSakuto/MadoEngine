#pragma once

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief ゲームプレイの残り時間をTextへ表示するビュー
	class GamePlayTimerView {
	public:
		/// @brief 残り時間表示を初期化
		void Initialize();

		/// @brief 残り時間表示を更新
		/// @param remainingTime ゲームプレイの残り時間（秒）
		void Update(float remainingTime);

		/// @brief 残り時間表示を終了
		void Finalize();

	private:
		MadoEngine::Text* timerText_ = nullptr;
		int displayedRemainingSeconds_ = -1;
	};
}
