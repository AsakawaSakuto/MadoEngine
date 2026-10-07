#include "RelicSystem.h"
#include "Utility/Logger/Logger.h"
#include <array>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kRelicRandomStreamId = 400;

}

namespace Relic {

	bool System::Initialize(std::uint32_t gameSeed, const std::string& catalogPath) {
		pendingRelicId_.reset();
		inventory_ = {};
		modifiers_.Reset();
		revision_ = 0;
		random_.SetSeed(MyRand::MakeDerivedSeed(gameSeed, kRelicRandomStreamId));
		catalogPath_ = catalogPath;
		return catalog_.Load(catalogPath_);
	}

	bool System::SaveCatalog(const std::vector<Definition>& definitions) {
		Catalog updatedCatalog;
		if (!updatedCatalog.Replace(definitions) || !IsCompatibleWithInventory(updatedCatalog)) {
			return false;
		}
		if (!updatedCatalog.Save(catalogPath_)) {
			return false;
		}

		catalog_ = std::move(updatedCatalog);
		if (pendingRelicId_ && !catalog_.Find(*pendingRelicId_)) {
			pendingRelicId_.reset();
		}
		RebuildModifiers();
		++revision_;
		Logger::Output("[Relic] レリック定義を保存して適用", Logger::Level::Assets);
		return true;
	}

	bool System::ReloadCatalog() {
		Catalog loadedCatalog;
		if (!loadedCatalog.Load(catalogPath_) || !IsCompatibleWithInventory(loadedCatalog)) {
			return false;
		}

		catalog_ = std::move(loadedCatalog);
		if (pendingRelicId_ && !catalog_.Find(*pendingRelicId_)) {
			pendingRelicId_.reset();
		}
		RebuildModifiers();
		++revision_;
		Logger::Output("[Relic] レリック定義を再読み込みして適用", Logger::Level::Assets);
		return true;
	}

	bool System::RequestReward() {
		if (pendingRelicId_) {
			return false;
		}

		std::array<std::vector<const Definition*>, kRarityDefinitions.size()> candidatesByRarity;
		for (const Definition& definition : catalog_.GetAll()) {
			if (!inventory_.CanAdd(definition)) {
				continue;
			}

			for (std::size_t index = 0; index < kRarityDefinitions.size(); ++index) {
				const RarityDefinition& rarityDefinition = kRarityDefinitions[index];
				if (rarityDefinition.selectionWeight > 0 &&
					definition.rarity == rarityDefinition.rarity) {
					candidatesByRarity[index].push_back(&definition);
					break;
				}
			}
		}

		// 候補が存在しないレアリティを除外して固定レアリティの抽選を維持
		float totalRarityWeight = 0.0f;
		for (std::size_t index = 0; index < candidatesByRarity.size(); ++index) {
			if (!candidatesByRarity[index].empty()) {
				totalRarityWeight += static_cast<float>(kRarityDefinitions[index].selectionWeight);
			}
		}
		if (totalRarityWeight <= 0.0f) {
			Logger::Output("[Relic] 全レリックが所持上限のため報酬を抽選できない", Logger::Level::Warning);
			return false;
		}

		float rarityRoll = random_.Float(0.0f, totalRarityWeight);
		for (std::size_t index = 0; index < candidatesByRarity.size(); ++index) {
			if (candidatesByRarity[index].empty()) {
				continue;
			}

			rarityRoll -= static_cast<float>(kRarityDefinitions[index].selectionWeight);
			if (rarityRoll <= 0.0f) {

				// 同じレアリティ内の全候補を均等な確率で選択
				const int selectedIndex = random_.Int(
					0,
					static_cast<int>(candidatesByRarity[index].size()) - 1);
				pendingRelicId_ = candidatesByRarity[index][static_cast<std::size_t>(selectedIndex)]->id;
				return true;
			}
		}
		return false;
	}

	bool System::CanRequestReward() const {
		if (pendingRelicId_) {
			return false;
		}

		for (const Definition& definition : catalog_.GetAll()) {
			if (inventory_.CanAdd(definition) && IsStandardRarity(definition.rarity)) {
				return true;
			}
		}
		return false;
	}

	bool System::ConfirmPendingReward() {
		const Definition* definition = GetPendingReward();
		if (!definition || !inventory_.Add(*definition)) {
			return false;
		}

		Logger::Output("[Relic] レリックを獲得: " + definition->displayName, Logger::Level::Application);
		pendingRelicId_.reset();
		RebuildModifiers();
		++revision_;
		return true;
	}

	bool System::AcquireForEditor(const Id& id) {
		const Definition* definition = catalog_.Find(id);
		if (!definition || !inventory_.Add(*definition)) {
			Logger::Output("[Relic] Editorからのレリック獲得を拒否: " + id, Logger::Level::Warning);
			return false;
		}

		// 獲得待ちとEditor付与が同じ種類の上限を競合した場合は待機状態を解消
		if (pendingRelicId_ == id && !inventory_.CanAdd(*definition)) {
			pendingRelicId_.reset();
		}
		RebuildModifiers();
		++revision_;
		Logger::Output("[Relic] Editorからレリックを獲得: " + definition->displayName, Logger::Level::Application);
		return true;
	}

	bool System::DiscardForEditor(const Id& id) {
		const Definition* definition = catalog_.Find(id);
		if (!definition || !inventory_.Remove(id)) {
			Logger::Output("[Relic] Editorからのレリック破棄を拒否: " + id, Logger::Level::Warning);
			return false;
		}

		RebuildModifiers();
		++revision_;
		Logger::Output("[Relic] Editorからレリックを破棄: " + definition->displayName, Logger::Level::Application);
		return true;
	}

	const Definition* System::GetPendingReward() const {
		return pendingRelicId_ ? catalog_.Find(*pendingRelicId_) : nullptr;
	}

	void System::RebuildModifiers() {
		modifiers_.Reset();

		// レリック獲得時だけ全所持品を走査して毎フレームの効果計算を回避
		for (const Definition& definition : catalog_.GetAll()) {
			const int stackCount = inventory_.GetCount(definition.id);
			for (const StatModifierEffect& effect : definition.effects) {
				modifiers_.Accumulate(effect, stackCount);
			}
		}
	}

	bool System::IsCompatibleWithInventory(const Catalog& catalog) const {
		for (const auto& [id, count] : inventory_.GetAllCounts()) {
			const Definition* definition = catalog.Find(id);
			if (!definition || count > definition->maxStack) {
				Logger::Output("[Relic] 所持状態と両立しない定義変更を拒否: " + id, Logger::Level::Error);
				return false;
			}
		}
		return true;
	}

}
