#include "DropObjectManager.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <limits>

namespace DropObject {

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Initialize() {
		Clear();
		InitializePool(expPool_, Type::Exp, 0);
		InitializePool(moneyPool_, Type::Money, static_cast<int>(kExpPoolCapacity));
	}

	template<std::size_t Capacity>
	void Manager::InitializePool(Pool<Capacity>& pool, Type type, int firstIndex) {
		pool.type = type;

		// Game開始前に描画と衝突のリソースまで確保して撃破Frameの新規生成を回避
		for (std::size_t index = 0; index < pool.entries.size(); ++index) {
			PoolEntry& entry = pool.entries[index];
			if (!entry.object) {
				entry.object = std::make_unique<Base>();
				entry.object->Initialize(type, {}, firstIndex + static_cast<int>(index));
			}
			entry.object->Deactivate();
		}
	}

	void Manager::Update(float deltaTime, Player::Base& player) {
		const Vector3 targetPosition = player.GetPosition();

		// 種類ごとに回収を完結させ、ExpとMoneyの蓄積報酬の混在を防止
		UpdatePool(expPool_, deltaTime, player);
		UpdatePool(moneyPool_, deltaTime, player);

		for (auto& dropObject : dropObjects_) {
			dropObject->SetTargetPosition(targetPosition);
			dropObject->Update(deltaTime, player);
		}

		// Update中に回収済みとなったObjectを全更新完了後にまとめて破棄
		dropObjects_.erase(
			std::remove_if(
				dropObjects_.begin(),
				dropObjects_.end(),
				[](const std::unique_ptr<Base>& dropObject) {
					return !dropObject || !dropObject->IsAlive();
				}),
			dropObjects_.end());
	}

	template<std::size_t Capacity>
	void Manager::UpdatePool(Pool<Capacity>& pool, float deltaTime, Player::Base& player) {

		// 回収済みDropは破棄せず保持し、次の配置で同じModelとColliderを再利用
		for (PoolEntry& entry : pool.entries) {
			if (!entry.object || !entry.object->IsAlive()) {
				continue;
			}
			entry.object->Update(deltaTime, player);
			GrantCollectedReward(pool.type, player, entry.object->ConsumeCollectedRewardAmount(), pool.pendingAmount);
		}
	}

	void Manager::Spawn(Type type, const Vector3& position) {

		// Scene外からの単独利用でも他種Dropの生成前に確保し、後続の初期化による消失を防止
		if (!expPool_.entries[0].object || !moneyPool_.entries[0].object) {
			Initialize();
		}

		// ExpとMoneyは独立した固定容量へ振り分け、Healだけを都度生成
		if (type == Type::Exp) {
			SpawnPooled(expPool_, position);
			return;
		}
		if (type == Type::Money) {
			SpawnPooled(moneyPool_, position);
			return;
		}

		auto newDropObject = std::make_unique<Base>();
		newDropObject->Initialize(type, position, spawnCount_);
		dropObjects_.push_back(std::move(newDropObject));
		spawnCount_++;
	}

	template<std::size_t Capacity>
	void Manager::SpawnPooled(Pool<Capacity>& pool, const Vector3& position) {
		std::size_t selectedIndex = pool.nextIndex;

		// 循環位置から空き枠を優先し、全枠が有効な場合だけ最古の配置を選択
		for (std::size_t offset = 0; offset < pool.entries.size(); ++offset) {
			const std::size_t index = (pool.nextIndex + offset) % pool.entries.size();
			const PoolEntry& entry = pool.entries[index];
			if (!entry.object->IsAlive()) {

				// 回収済みの枠があれば未回収Dropを消さずに再利用
				selectedIndex = index;
				break;
			}
			if (entry.spawnOrder < pool.entries[selectedIndex].spawnOrder) {
				selectedIndex = index;
			}
		}

		PoolEntry& selectedEntry = pool.entries[selectedIndex];
		if (selectedEntry.object->IsAlive()) {

			// 上書き前の報酬だけを同種Poolへ退避し、新しいDropとの二重計上を防止
			pool.pendingAmount += static_cast<std::uint64_t>(selectedEntry.object->GetRewardAmount());
		}
		selectedEntry.object->Activate(position);
		selectedEntry.spawnOrder = ++pool.spawnOrder;
		pool.nextIndex = (selectedIndex + 1) % pool.entries.size();
	}

	void Manager::GrantCollectedReward(Type type, Player::Base& player, int collectedAmount, std::uint64_t& pendingAmount) {
		if (collectedAmount <= 0) {
			return;
		}

		// 次の同種Drop一個へ蓄積分を全量合算し、同Frameの後続回収では再加算を防止
		std::uint64_t amount = pendingAmount + static_cast<std::uint64_t>(collectedAmount);
		pendingAmount = 0;

		// Playerのint引数を超える蓄積でも切り捨てず分割して全量を加算
		constexpr std::uint64_t kMaxGrantAmount = static_cast<std::uint64_t>((std::numeric_limits<int>::max)());
		while (amount > 0) {
			const int grantAmount = static_cast<int>((std::min)(amount, kMaxGrantAmount));
			if (type == Type::Exp) {
				player.AddExp(grantAmount);
			} else if (type == Type::Money) {
				player.AddMoney(grantAmount);
			}
			amount -= static_cast<std::uint64_t>(grantAmount);
		}
	}

	void Manager::Clear() {
		dropObjects_.clear();
		ClearPool(expPool_);
		ClearPool(moneyPool_);
	}

	template<std::size_t Capacity>
	void Manager::ClearPool(Pool<Capacity>& pool) {

		// EditorのMap再生成でもPoolの再確保を避け、旧配置の報酬だけを破棄
		for (PoolEntry& entry : pool.entries) {
			if (entry.object) {
				entry.object->Deactivate();
			}
			entry.spawnOrder = 0;
		}
		pool.nextIndex = 0;
		pool.spawnOrder = 0;
		pool.pendingAmount = 0;
	}

	void Manager::Finalize() {
		Clear();

		// Scene一括削除より先に所有Objectを解放し、Collider参照の寿命を維持
		expPool_ = {};
		moneyPool_ = {};
		spawnCount_ = static_cast<int>(kExpPoolCapacity + kMoneyPoolCapacity);
	}
}
