#include "EnemyFactory.h"
#include "Type/EnemyBoss.h"
#include "Type/EnemyNormal.h"
#include "Type/EnemyOverTime.h"
#include "Type/EnemyRunner.h"
#include "EnemySettings.h"
#include "Type/EnemyTank.h"

namespace Enemy {

	std::unique_ptr<Base> Factory::Create(Data::Type type) {

		// 派生クラスの選択をFactoryへ集約して呼び出し元から具象型への依存を分離
		switch (type) {
		case Data::Type::Normal:
			return std::make_unique<Normal>();
		case Data::Type::Runner:
			return std::make_unique<Runner>();
		case Data::Type::Tank:
			return std::make_unique<Tank>();
		case Data::Type::Boss:
			return std::make_unique<Boss>();
		case Data::Type::OverTime:
			return std::make_unique<OverTime>();
		}

		return nullptr;
	}

	Data::Status Factory::CreateDefaultStatus(Data::Type type) {
		return Settings::GetInstance().GetTypeSettings(type).status;
	}

} // namespace Enemy
