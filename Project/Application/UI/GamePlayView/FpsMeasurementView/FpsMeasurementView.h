#pragma once

namespace MadoEngine {
	class Text;
}

namespace GamePlayView {

	/// @brief FPSを計測してTextへ表示するビュー
	class FpsMeasurementView {
	public:
		/// @brief FPS表示を初期化
		void Initialize();

		/// @brief FPSを計測して表示を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief FPS表示を終了
		void Finalize();

	private:
		MadoEngine::Text* fpsText_ = nullptr;
		float sampleTime_ = 0.0f;
		int sampleFrameCount_ = 0;
	};
}
