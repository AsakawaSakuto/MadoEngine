#include "RelicInventory.h"

namespace Relic {

	bool Inventory::Add(const Definition& definition) {
		if (!CanAdd(definition)) {
			return false;
		}

		++counts_[definition.id];
		return true;
	}

	bool Inventory::Remove(const Id& id) {
		auto iterator = counts_.find(id);
		if (iterator == counts_.end() || iterator->second <= 0) {
			return false;
		}

		--iterator->second;
		if (iterator->second == 0) {
			counts_.erase(iterator);
		}
		return true;
	}

	int Inventory::GetCount(const Id& id) const {
		const auto iterator = counts_.find(id);
		return iterator != counts_.end() ? iterator->second : 0;
	}

	bool Inventory::CanAdd(const Definition& definition) const {
		return !definition.id.empty() && definition.maxStack > 0 &&
			GetCount(definition.id) < definition.maxStack;
	}

}
