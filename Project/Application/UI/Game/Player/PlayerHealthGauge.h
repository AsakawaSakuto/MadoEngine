#pragma once
#include "RenderHeaders.h"

class Camera;

namespace UI::Game {

	/// @brief Playerの2D表示と頭上3D表示を管理するHPゲージ
	class PlayerHealthGauge final {
	public:
		/// @brief Playerの2Dと3DのHP表示を初期化
		void Initialize();

		/// @brief 画面固定と頭上のHPゲージの表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief Playerの座標とHPを2Dと3DのHP表示へ反映
		/// @param playerPosition Playerのワールド座標
		/// @param currentHealth 現在HP
		/// @param maxHealth 最大HP
		/// @param camera 描画に使用するCamera
		void Update(
			const Vector3& playerPosition,
			float currentHealth,
			float maxHealth,
			const Camera& camera
		);

		/// @brief Playerの2Dと3DのHP表示を解放
		void Finalize();

		/// @brief 2Dと3DのHPゲージのImGuiを描画
		void DrawImGui();

	private:
		std::unique_ptr<Gauge2d> healthGauge2d_;
		std::unique_ptr<Gauge3d> healthGauge3d_;
	};

} // namespace UI::Game
