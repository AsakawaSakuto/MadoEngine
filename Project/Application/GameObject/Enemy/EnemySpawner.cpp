#include "EnemySpawner.h"
#include "EnemyFactory.h"
#include "GameObject/Player/Player.h"
#include "Utility/Collider/MyCollider.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kSpawnBuriedDepth = 2.0f;
	constexpr float kSecondsPerMinute = 60.0f;
	constexpr std::size_t kMaxSpawnPositionAttempts = 8;
	constexpr std::size_t kInvalidWaveIndex = static_cast<std::size_t>(-1);
	constexpr std::size_t kBonusWaveIndex = kInvalidWaveIndex - 1;
} // namespace

namespace Enemy {

	void Spawner::Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType, float timeLimit) {
		player_ = player;
		enemyManager_ = enemyManager;
		sceneType_ = sceneType;
		timeLimit_ = std::isfinite(timeLimit) ? std::max(0.0f, timeLimit) : 0.0f;
		Clear();
		isActive_ = true;
		Logger::Output("Enemy::Spawnerを初期化しました", Logger::Level::Application);
	}

	void Spawner::Update(float deltaTime) {
		if (!isActive_ || !player_ || !enemyManager_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}

		const float previousElapsedTime = elapsedTime_;
		elapsedTime_ += deltaTime;
		std::size_t waveIndex = kInvalidWaveIndex;
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(waveIndex);
		if (!wave) {

			// Wave空白時間の周期残量を次のWaveへ持ち越さないよう生成状態を解除
			ChangeActiveWave(kInvalidWaveIndex);
			return;
		}

		ChangeActiveWave(waveIndex);

		// 制限時間へ到達したFrameは超過分だけをBonus Waveの生成Timerへ反映
		const float effectiveDeltaTime =
			waveIndex == kBonusWaveIndex && previousElapsedTime < timeLimit_ ?
			elapsedTime_ - timeLimit_ : deltaTime;
		spawnTimer_ += std::max(0.0f, effectiveDeltaTime);
		const float spawnInterval = std::max(0.01f, wave->spawnInterval);

		// 長いFrameでも経過した生成周期を取りこぼさないようTimer残量を順次消費
		while (spawnTimer_ >= spawnInterval) {
			spawnTimer_ -= spawnInterval;
			SpawnBatch(*wave, wave->spawnCount);
		}
	}

	void Spawner::Clear() {
		activeWaveIndex_ = kInvalidWaveIndex;
		activeWaveEnemySpawnCount_ = 0;
		spawnTimer_ = 0.0f;
		elapsedTime_ = 0.0f;
	}

	std::uint32_t Spawner::GetRemainingSpawnCountUntilElite() const {
		std::size_t waveIndex = kInvalidWaveIndex;
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(waveIndex);
		if (!wave) {
			return 0;
		}

		const std::uint64_t eliteSpawnInterval = std::max<std::uint64_t>(1, wave->eliteSpawnInterval);
		const std::uint64_t waveSpawnCount = waveIndex == activeWaveIndex_ ? activeWaveEnemySpawnCount_ : 0;
		const std::uint64_t completedInCurrentCycle = waveSpawnCount % eliteSpawnInterval;
		return static_cast<std::uint32_t>(eliteSpawnInterval - completedInCurrentCycle);
	}

	bool Spawner::TryGetActiveWaveIndex(std::size_t& outIndex) const {
		if (activeWaveIndex_ == kInvalidWaveIndex || activeWaveIndex_ == kBonusWaveIndex) {
			return false;
		}

		outIndex = activeWaveIndex_;
		return true;
	}

	bool Spawner::IsBonusWaveActive() const {
		return activeWaveIndex_ == kBonusWaveIndex;
	}

	std::uint32_t Spawner::SpawnImmediately(std::uint32_t spawnCount) {
		std::size_t waveIndex = kInvalidWaveIndex;
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(waveIndex);
		if (!wave) {
			return 0;
		}

		ChangeActiveWave(waveIndex);
		return SpawnBatch(*wave, spawnCount);
	}

	const WaveSpawnSettings* Spawner::FindActiveSpawnSettings(std::size_t& outIndex) const {
		if (elapsedTime_ >= timeLimit_) {

			// 制限時間到達後は通常Waveの時間帯に関係なくBonus Waveを最優先
			outIndex = kBonusWaveIndex;
			return &Settings::GetInstance().GetBonusWaveSettings();
		}

		const std::vector<WaveSettings>& waves = Settings::GetInstance().GetWaves();
		const WaveSettings* selectedWave = nullptr;
		float selectedStartTime = -1.0f;
		outIndex = kInvalidWaveIndex;

		// 時間帯が重複する場合は開始時間が遅いWaveを優先して結果を一意化
		for (std::size_t index = 0; index < waves.size(); ++index) {
			const WaveSettings& wave = waves[index];
			if (elapsedTime_ < wave.startTime || elapsedTime_ >= wave.endTime) {
				continue;
			}
			if (!selectedWave || wave.startTime >= selectedStartTime) {
				selectedWave = &wave;
				selectedStartTime = wave.startTime;
				outIndex = index;
			}
		}

		return selectedWave;
	}

	void Spawner::ChangeActiveWave(std::size_t waveIndex) {
		if (activeWaveIndex_ == waveIndex) {
			return;
		}

		// Waveをまたいで生成TimerとElite生成数が引き継がれないよう同時に初期化
		activeWaveIndex_ = waveIndex;
		activeWaveEnemySpawnCount_ = 0;
		spawnTimer_ = 0.0f;
	}

	std::uint32_t Spawner::SpawnBatch(const WaveSpawnSettings& wave, std::uint32_t spawnCount) {
		const std::size_t maxAliveEnemies = Settings::GetInstance().GetMaxAliveEnemies();
		if (!enemyManager_ || spawnCount == 0 || enemyManager_->GetEnemyCount() >= maxAliveEnemies) {
			return 0;
		}

		const std::size_t availableCount = maxAliveEnemies - enemyManager_->GetEnemyCount();
		const std::uint32_t requestCount = static_cast<std::uint32_t>(std::min<std::size_t>(
			availableCount,
			static_cast<std::size_t>(spawnCount)));
		std::uint32_t spawnedCount = 0;
		for (std::uint32_t index = 0; index < requestCount; ++index) {
			if (SpawnEnemy(wave)) {
				++spawnedCount;
			}
		}

		return spawnedCount;
	}

	Data::Type Spawner::SelectSpawnType(const WaveSpawnSettings& wave) const {
		const float normalRate = std::max(0.0f, wave.normalSpawnRate);
		const float runnerRate = std::max(0.0f, wave.runnerSpawnRate);
		const float tankRate = std::max(0.0f, wave.tankSpawnRate);
		const float totalRate = normalRate + runnerRate + tankRate;
		if (totalRate <= 0.0f) {
			return Data::Type::Normal;
		}

		const float randomValue = MyRand::GetFloat(0.0f, totalRate);
		if (randomValue < normalRate) {
			return Data::Type::Normal;
		}
		if (randomValue < normalRate + runnerRate) {
			return Data::Type::Runner;
		}

		return Data::Type::Tank;
	}

	bool Spawner::SpawnEnemy(const WaveSpawnSettings& wave) {
		if (!player_ || !enemyManager_) {
			return false;
		}

		SpawnDesc desc;
		float groundSurfaceY = 0.0f;
		if (!TryCreateSpawnPosition(wave, desc.position, groundSurfaceY)) {
			return false;
		}

		// 地形Colliderを無効化した出現状態で地表面直下から上昇
		const Data::Type spawnType = SelectSpawnType(wave);
		desc.emergeFromGround = true;
		desc.groundSurfaceY = groundSurfaceY;
		desc.status = CalculateSpawnStatus(spawnType, wave);
		desc.type = spawnType;
		++activeWaveEnemySpawnCount_;
		const std::uint64_t eliteSpawnInterval = std::max<std::uint64_t>(1, wave.eliteSpawnInterval);

		// Wave内で指定周期に到達したEnemyだけを種類に依存せずElite化
		desc.bonusType = activeWaveEnemySpawnCount_ % eliteSpawnInterval == 0 ?
			Data::BonusType::Elite : Data::BonusType::None;
		desc.sceneType = sceneType_;
		enemyManager_->Spawn(desc);
		return true;
	}

	bool Spawner::TryCreateSpawnPosition(
		const WaveSpawnSettings& wave,
		Vector3& outPosition,
		float& outGroundSurfaceY) const {
		const Vector3 playerPosition = player_->GetPosition();
		const float groundSearchDistance = mapLimit_.max.y - mapLimit_.min.y;

		// 穴や地形未配置地点を避けるためPlayer周辺の候補を複数回探索
		for (std::size_t attempt = 0; attempt < kMaxSpawnPositionAttempts; ++attempt) {
			const float angle = MyRand::GetFloat(0.0f, kPi * 2.0f);
			const float radius = MyRand::GetFloat(wave.minSpawnRadius, wave.maxSpawnRadius);
			Vector3 groundQueryOrigin = {
				playerPosition.x + std::sin(angle) * radius,
				mapLimit_.max.y,
				playerPosition.z + std::cos(angle) * radius,
			};

			// Player周囲の生成候補がMap外へ出ないようXZ座標を範囲内へ制限
			groundQueryOrigin.x = std::clamp(groundQueryOrigin.x, mapLimit_.min.x, mapLimit_.max.x);
			groundQueryOrigin.z = std::clamp(groundQueryOrigin.z, mapLimit_.min.z, mapLimit_.max.z);

			float blockSurfaceY = 0.0f;
			float slopeSurfaceY = 0.0f;
			const bool foundBlock = MyCollider::TryGetGroundSurfaceY(
				groundQueryOrigin, CollisionTag::MapBlock, blockSurfaceY, groundSearchDistance);
			const bool foundSlope = MyCollider::TryGetGroundSurfaceY(
				groundQueryOrigin, CollisionTag::MapSlope, slopeSurfaceY, groundSearchDistance);
			if (!foundBlock && !foundSlope) {
				continue;
			}

			// 境界上で複数の地形が見つかった場合は埋まりを避けるため高い表面を採用
			outGroundSurfaceY = foundBlock && foundSlope ? std::max(blockSurfaceY, slopeSurfaceY) :
				foundBlock ? blockSurfaceY : slopeSurfaceY;
			outPosition = groundQueryOrigin;
			outPosition.y = outGroundSurfaceY - kSpawnBuriedDepth;
			return true;
		}

		return false;
	}

	Data::Status Spawner::CalculateSpawnStatus(Data::Type type, const WaveSpawnSettings& wave) const {

		// Game全体の経過分数へWave固有の強化幅を適用して難易度を算出
		const float elapsedMinutes = elapsedTime_ / kSecondsPerMinute;
		const float healthPowerMultiplier = 1.0f + elapsedMinutes * wave.healthPowerGrowthRatePerMinute;
		const float moveSpeedMultiplier = 1.0f + elapsedMinutes * wave.moveSpeedGrowthRatePerMinute;

		Data::Status status = Factory::CreateDefaultStatus(type);
		status.currentHealth *= healthPowerMultiplier;
		status.power *= healthPowerMultiplier;
		status.moveSpeed *= moveSpeedMultiplier;
		return status;
	}

} // namespace Enemy
