#pragma once

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief FPSを計測してTextへ表示するビュー
	class FpsMeasurementView {
	public:
		/// @brief FPS表示を初期化
		void Initialize();

		/// @brief FPSを計測して表示を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param fpsText 表示先のText、未接続の場合はnullptr
		void Update(float deltaTime, MadoEngine::Text* fpsText);

		/// @brief FPS表示を終了
		void Finalize();

	private:
		float sampleTime_ = 0.0f;
		int sampleFrameCount_ = 0;
	};
}
