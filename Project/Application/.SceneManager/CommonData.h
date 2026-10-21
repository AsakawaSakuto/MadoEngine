#pragma once
#include "SceneTransitionController.h"
#include "System/GameSeed/GameSeedSystem.h"
#include "Utility/Parameter/ParameterStore.h"

/// @brief Sceneをまたいで保持するApplication共通データ
class CommonData final {
public:
	/// @brief Engine所有の設定値管理を共通データへ接続
	/// @param parameterStore Sceneより長く保持する設定値管理
	explicit CommonData(MadoEngine::ParameterStore& parameterStore) : parameterStore_(parameterStore) {}

	/// @brief 調整用設定値管理を取得
	/// @return Engine所有の設定値管理
	MadoEngine::ParameterStore& GetParameterStore() { return parameterStore_; }

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
	MadoEngine::ParameterStore& parameterStore_;
	System::GameSeedSystem gameSeedSystem_;
	SceneTransitionController sceneTransitionController_;
};
