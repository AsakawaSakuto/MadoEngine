#include "EnemyFactory.h"
#include "EnemyBoss.h"
#include "EnemyNormal.h"

namespace Enemy {

	std::unique_ptr<Base> Factory::Create(Data::Type type) {
		switch (type) {
		case Data::Type::Normal:
			return std::make_unique<Normal>();
		case Data::Type::Boss:
			return std::make_unique<Boss>();
		}

		return nullptr;
	}

	Data::Status Factory::CreateDefaultStatus(Data::Type type) {
		switch (type) {
		case Data::Type::Normal:
			return { 10.0f, 5.0f, 3.0f };
		case Data::Type::Boss:
			return { 1000.0f, 20.0f, 1.5f };
		}

		return {};
	}

} // namespace Enemy