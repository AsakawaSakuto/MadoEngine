#pragma once
#include "System/GameSeed/GameSeedSystem.h"

/// @brief Sceneをまたいで保持するApplication共通データ
class CommonData final {
public:
	/// @brief Application共通データの初期化
	void Initialize() {
		gameSeedSystem_.Initialize();
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

private:
	System::GameSeedSystem gameSeedSystem_;
};
