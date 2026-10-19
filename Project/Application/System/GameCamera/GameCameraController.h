#pragma once
#include "Utility/Camera/CameraManager.h"

namespace System {

	/// @brief ゲーム用Cameraの追従入力とデバッグ切り替えの管理
	class GameCameraController final {
	public:
		/// @brief Scene所有のManagerへゲーム用Cameraを登録
		/// @param cameraManager Cameraの所有先
		void Initialize(CameraManager& cameraManager);

		/// @brief Scene終了時に操作対象のHandleを初期化
		void Finalize();

		/// @brief Player追従とズーム入力を適用してCameraを更新
		/// @param cameraManager 更新対象のCamera所有先
		/// @param playerPosition Playerのワールド座標
		/// @param gameDeltaTime 停止状態を反映したゲーム経過時間
		/// @param isInputEnabled TPS操作入力を受け付ける場合はtrue
		void UpdateTracking(CameraManager& cameraManager, const Vector3& playerPosition,
			float gameDeltaTime, bool isInputEnabled);

		/// @brief Editorの選択状態に追従してデバッグCamera切り替え入力を処理
		/// @param cameraManager 切り替え対象のCamera所有先
		void UpdateSwitchInput(CameraManager& cameraManager);

		/// @brief Player操作へ接続するTPS Cameraを取得
		/// @param cameraManager Cameraの所有先
		/// @return 登録済みTPS Camera、有効なHandleがない場合はnullptr
		TPS_Camera* TryGetPlayerCamera(CameraManager& cameraManager) const;

	private:
		CameraHandle debugCameraHandle_{};
		CameraHandle tpsCameraHandle_{};
		bool useDebugCamera_ = false;
	};

}
