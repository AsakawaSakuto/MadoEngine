#pragma once
#include "EnemyBase.h"
#include <memory>

namespace Enemy {

	/// @brief Enemy種類に対応する具象クラスと基礎能力値を生成するFactory
	class Factory final {
	public:
		/// @brief FactoryのInstance化を禁止
		Factory() = delete;

		/// @brief Enemy種類に対応する具象クラスを生成
		/// @param type 生成するEnemyの種類
		/// @return 生成したEnemy、未対応種類の場合はnullptr
		static std::unique_ptr<Base> Create(Data::Type type);

		/// @brief Enemy種類に対応する基礎能力値を作成
		/// @param type 能力値を取得するEnemyの種類
		/// @return Enemy種類に対応する基礎能力値
		static Data::Status CreateDefaultStatus(Data::Type type);
	};

} // namespace Enemy
