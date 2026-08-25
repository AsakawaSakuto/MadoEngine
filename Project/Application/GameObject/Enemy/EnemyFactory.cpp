#include "EnemyFactory.h"
#include "Type/EnemyBoss.h"
#include "Type/EnemyNormal.h"
#include "Type/EnemyRunner.h"
#include "EnemySettings.h"
#include "Type/EnemyTank.h"

namespace Enemy {

	std::unique_ptr<Base> Factory::Create(Data::Type type) {
		switch (type) {
		case Data::Type::Normal:
			return std::make_unique<Normal>();
		case Data::Type::Runner:
			return std::make_unique<Runner>();
		case Data::Type::Tank:
			return std::make_unique<Tank>();
		case Data::Type::Boss:
			return std::make_unique<Boss>();
		}

		return nullptr;
	}

	Data::Status Factory::CreateDefaultStatus(Data::Type type) {
		return Settings::GetInstance().GetTypeSettings(type).status;
	}

} // namespace Enemy
