#include "DropObjectManager.h"
#include "Audio/MyAudio.h"
#include "GameObject/Player/Player.h"
#include "Utility/Random.h"
#include <algorithm>
#include <array>
#include <limits>

namespace DropObject {

	namespace {
		constexpr std::array<const char*, 5> kExpSoundKeys = { "Exp1", "Exp2", "Exp3", "Exp4", "Exp5" };
		constexpr std::array<Type, 4> kSpecialTypes = {
			Type::Heal, Type::AttackUpper, Type::SpeedUpper, Type::DefuseUpper
		};
		constexpr int kJarSpecialDropChancePercent = 100;
		constexpr int kEliteSpecialDropChancePercent = 25;
	}

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Initialize() {
		Clear();
		InitializePool(expPool_, Type::Exp, 0);
		InitializePool(moneyPool_, Type::Money, static_cast<int>(kExpPoolCapacity));

		// 特殊4種類は共通の16枠を再利用し、ExpとMoneyの枠や上書き報酬から分離
		for (std::size_t index = 0; index < specialDropObjects_.size(); ++index) {
			auto& object = specialDropObjects_[index];
			if (!object) {
				object = std::make_unique<Base>();
				object->Initialize(Type::Heal, {}, static_cast<int>(kExpPoolCapacity + kMoneyPoolCapacity + index));
			}
			object->Deactivate();
		}
	}

	void Manager::EnsureInitialized() {

		// Scene外の単独利用でも全枠を先に確保し、後続の初期化による配置消失を防止
		if (!expPool_.entries[0].object || !moneyPool_.entries[0].object || !specialDropObjects_[0]) {
			Initialize();
		}
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
		expCollectionSoundCooldown_ = (std::max)(0.0f, expCollectionSoundCooldown_ - (std::max)(0.0f, deltaTime));

		// 種類ごとに回収を完結させ、ExpとMoneyの蓄積報酬の混在を防止
		const bool hasCollectedExp = UpdatePool(expPool_, deltaTime, player);
		UpdatePool(moneyPool_, deltaTime, player);

		// 経験値は全件加算したうえでSEだけをまとめ、同時回収時の音声生成を抑制
		if (hasCollectedExp) {
			PlayExpCollectionSound(player);
		}

		// 回収済みの特殊Dropも保持し、次回の出現時にModelとColliderを再利用
		for (auto& dropObject : specialDropObjects_) {
			if (!dropObject || !dropObject->IsAlive()) {
				continue;
			}
			dropObject->Update(deltaTime, player);

			// Playerへ効果を適用して回収済みへ遷移した特殊Dropだけを通知
			if (!dropObject->IsAlive()) {
				specialCollectionEvents_.push_back(dropObject->GetType());
			}
		}
	}

	std::vector<Type> Manager::ConsumeSpecialCollectionEvents() {
		std::vector<Type> events;

		// 同じ取得を複数Frameで表示しないよう未処理通知の所有権を呼び出し側へ移動
		events.swap(specialCollectionEvents_);
		return events;
	}

	template<std::size_t Capacity>
	bool Manager::UpdatePool(Pool<Capacity>& pool, float deltaTime, Player::Base& player) {
		bool hasCollectedReward = false;

		// 回収済みDropは破棄せず保持し、次の配置で同じModelとColliderを再利用
		for (PoolEntry& entry : pool.entries) {
			if (!entry.object || !entry.object->IsAlive()) {
				continue;
			}
			entry.object->Update(deltaTime, player);
			const int collectedAmount = entry.object->ConsumeCollectedRewardAmount();
			if (collectedAmount > 0) {
				hasCollectedReward = true;
			}
			GrantCollectedReward(pool.type, player, collectedAmount, pool.pendingAmount);
		}
		return hasCollectedReward;
	}

	void Manager::PlayExpCollectionSound(const Player::Base& player) {
		if (expCollectionSoundCooldown_ > 0.0f) {
			return;
		}

		// レベルアップ後の残り経験値を基準に20%刻みの音源を選択し、実行時の音程変換を回避
		const Player::Status status = player.GetStatus();
		float levelProgress = 0.0f;
		if (status.expToNextLevel > 0.0f) {
			levelProgress = std::clamp(status.currentExp / status.expToNextLevel, 0.0f, 1.0f);
		}
		std::size_t soundIndex = static_cast<std::size_t>(levelProgress * static_cast<float>(kExpSoundKeys.size()));
		soundIndex = (std::min)(soundIndex, kExpSoundKeys.size() - 1);
		MyAudio::Play(kExpSoundKeys[soundIndex]);
		expCollectionSoundCooldown_ = kExpCollectionSoundInterval;
	}

	void Manager::Spawn(Type type, const Vector3& position) {
		EnsureInitialized();

		// 通常報酬と特殊Dropで容量と満杯時の扱いを分離
		if (type == Type::Exp) {
			SpawnPooled(expPool_, position);
			return;
		}
		if (type == Type::Money) {
			SpawnPooled(moneyPool_, position);
			return;
		}

		switch (type) {
		case Type::Heal:
		case Type::AttackUpper:
		case Type::SpeedUpper:
		case Type::DefuseUpper:
			SpawnSpecial(type, position);
			break;
		default:
			break;
		}
	}

	bool Manager::TrySpawnSpecial(SpecialDropSource source, const Vector3& position) {
		int chancePercent = 0;

		// 発生元ごとに一度だけ抽選し、種類ごとの独立抽選による複数出現を防止
		switch (source) {
		case SpecialDropSource::Jar:
			chancePercent = kJarSpecialDropChancePercent;
			break;
		case SpecialDropSource::EliteEnemy:
			chancePercent = kEliteSpecialDropChancePercent;
			break;
		default:
			return false;
		}
		if (MyRand::GetInt(1, 100) > chancePercent) {
			return false;
		}

		EnsureInitialized();
		const std::size_t typeIndex = static_cast<std::size_t>(MyRand::GetInt(0, static_cast<int>(kSpecialTypes.size()) - 1));
		return SpawnSpecial(kSpecialTypes[typeIndex], position);
	}

	bool Manager::SpawnSpecial(Type type, const Vector3& position) {

		// 使用中の特殊Dropは上書きせず、回収で空いた枠だけを再利用
		for (auto& dropObject : specialDropObjects_) {
			if (!dropObject->IsAlive()) {
				dropObject->Activate(type, position);
				return true;
			}
		}
		return false;
	}

	void Manager::AttractAllExp(const Vector3& targetPosition) {

		// 使用中のExpだけを対象とし、未使用枠や別種Dropへの効果適用を防止
		for (PoolEntry& entry : expPool_.entries) {
			if (!entry.object || !entry.object->IsAlive()) {
				continue;
			}
			entry.object->AttractTo(targetPosition);
		}
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

		// 配置のリセット時も特殊Dropの確保枠を保持して再生成負荷を抑制
		for (auto& dropObject : specialDropObjects_) {
			if (dropObject) {
				dropObject->Deactivate();
			}
		}
		ClearPool(expPool_);
		ClearPool(moneyPool_);
		specialCollectionEvents_.clear();
		expCollectionSoundCooldown_ = 0.0f;
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
		for (auto& dropObject : specialDropObjects_) {
			dropObject.reset();
		}
	}
}
