#pragma once
#include "SceneTransitionController.h"
#include "System/GameSeed/GameSeedSystem.h"

/// @brief Sceneをまたいで保持するApplication共通データ
class CommonData final {
public:
	/// @brief Application共通データの初期化
	void Initialize() {
		gameSeedSystem_.Initialize();
		sceneTransitionController_.Reset();
	}

	/// @brief GameSeed管理システムを取得
	/// @return GameSeed管理システム
	System::GameSeedSystem& GetGameSeedSystem() {
		return gameSeedSystem_;
	}

	/// @brief GameSeed管理システムを読み取り専用で取得
	/// @return GameSeed管理システム
	const System::GameSeedSystem& GetGameSeedSystem() const {
		return gameSeedSystem_;
	}

	/// @brief シーン遷移管理を取得
	/// @return シーン遷移管理
	SceneTransitionController& GetSceneTransitionController() {
		return sceneTransitionController_;
	}

	/// @brief 読み取り専用のシーン遷移管理を取得
	/// @return 読み取り専用のシーン遷移管理
	const SceneTransitionController& GetSceneTransitionController() const {
		return sceneTransitionController_;
	}

private:
	System::GameSeedSystem gameSeedSystem_;
	SceneTransitionController sceneTransitionController_;
};
