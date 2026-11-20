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

		/// @brief Playerの2Dゲージ参照と頭上3D表示を解放
		void Finalize();

	private:
		/// @brief 永続IDから最新の2Dゲージを一時取得
		/// @return 接続中の2Dゲージ、削除済みの場合はnullptr
		Gauge2d* TryGetGauge() const;

		std::string gaugeId_;
		std::unique_ptr<Gauge3d> healthGauge3d_;
		bool isVisible_ = true;
	};

} // namespace UI::Game
