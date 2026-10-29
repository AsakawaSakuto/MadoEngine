#include "EnemySpawner.h"
#include "EnemyFactory.h"
#include "GameObject/Player/Player.h"
#include "Utility/Collider/MyCollider.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kSpawnBuriedDepth = 2.0f;
	constexpr std::size_t kMaxSpawnPositionAttempts = 8;
	constexpr std::size_t kInvalidWaveIndex = static_cast<std::size_t>(-1);
	constexpr std::size_t kBonusWaveIndex = kInvalidWaveIndex - 1;

	/// @brief 指定区間で完了したWave内強化周期の加算量を計算
	/// @param previousWaveElapsedTime 区間開始時のWave開始後経過秒数
	/// @param waveElapsedTime Wave開始後の経過秒数
	/// @param increaseSettings 加算間隔と一回の加算量
	/// @return 区間内で新たに完了した強化周期の加算量
	double CalculateStatIncrease(
		float previousWaveElapsedTime,
		float waveElapsedTime,
		const Enemy::TimedStatIncreaseSettings& increaseSettings) {
		if (waveElapsedTime <= previousWaveElapsedTime || increaseSettings.amount <= 0.0f) {
			return 0.0;
		}

		// 完了周期数の差分だけを加算し、Frame分割による二重加算と端数蓄積を回避
		const double interval = (std::max)(0.01, static_cast<double>(increaseSettings.interval));
		const double previousCompletedIntervals = std::floor(static_cast<double>(previousWaveElapsedTime) / interval);
		const double completedIntervals = std::floor(static_cast<double>(waveElapsedTime) / interval);
		return (completedIntervals - previousCompletedIntervals) * static_cast<double>(increaseSettings.amount);
	}

	/// @brief 基礎能力値へ現在の生成期間の累計強化値を適用
	/// @param baseValue 加算前の基礎能力値
	/// @param accumulatedIncrease 通常Wave群またはBonus Waveで累積した加算量
	/// @return 表現可能な範囲へ収めた加算後能力値
	float ApplyStatIncrease(float baseValue, double accumulatedIncrease) {
		const double increasedValue = static_cast<double>(baseValue) + accumulatedIncrease;
		return static_cast<float>((std::min)(
			increasedValue,
			static_cast<double>((std::numeric_limits<float>::max)())));
	}
} // namespace

namespace Enemy {

	void Spawner::Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType, float timeLimit) {
		player_ = player;
		enemyManager_ = enemyManager;
		sceneType_ = sceneType;
		if (std::isfinite(timeLimit)) {
			timeLimit_ = std::max(0.0f, timeLimit);
		} else {
			timeLimit_ = 0.0f;
		}
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
		UpdateStatIncreases(previousElapsedTime);
		std::size_t waveIndex = kInvalidWaveIndex;
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(elapsedTime_, waveIndex);
		if (!wave) {

			// Wave空白時間の周期残量を次のWaveへ持ち越さないよう生成状態を解除
			ChangeActiveWave(kInvalidWaveIndex);
			return;
		}

		ChangeActiveWave(waveIndex);

		// 制限時間へ到達したFrameは超過分だけをBonus Waveの生成Timerへ反映
		const float effectiveDeltaTime =
			[&]() {
				if (waveIndex == kBonusWaveIndex && previousElapsedTime < timeLimit_) {
					return (elapsedTime_ - timeLimit_);
				} else {
					return (deltaTime);
				}
			}();
		spawnTimer_ += std::max(0.0f, effectiveDeltaTime);
		const float spawnInterval = std::max(0.01f, wave->spawnInterval);
		const std::uint64_t karmaBonusSpawnCount = static_cast<std::uint64_t>(
			std::max(0, player_->GetKarmaCount()));

		// 長いFrameでも生成周期ごとの時刻で増加数を計算し、過去の周期への過剰加算を回避
		while (spawnTimer_ >= spawnInterval) {
			spawnTimer_ -= spawnInterval;
			const float spawnTime = elapsedTime_ - spawnTimer_;
			const std::uint32_t adjustedSpawnCount = CalculateSpawnCount(*wave, spawnTime, karmaBonusSpawnCount);
			SpawnBatch(*wave, adjustedSpawnCount);
		}
	}

	void Spawner::Clear() {
		activeWaveIndex_ = kInvalidWaveIndex;
		activeWaveEnemySpawnCount_ = 0;
		spawnTimer_ = 0.0f;
		elapsedTime_ = 0.0f;
		accumulatedHealthIncrease_ = 0.0;
		accumulatedPowerIncrease_ = 0.0;
		accumulatedMoveSpeedIncrease_ = 0.0;
	}

	std::uint32_t Spawner::GetRemainingSpawnCountUntilElite() const {
		std::size_t waveIndex = kInvalidWaveIndex;
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(elapsedTime_, waveIndex);
		if (!wave) {
			return 0;
		}

		const std::uint64_t eliteSpawnInterval = std::max<std::uint64_t>(1, wave->eliteSpawnInterval);
		const std::uint64_t waveSpawnCount = [&]() -> std::uint64_t {
			if (waveIndex == activeWaveIndex_) {
				return (activeWaveEnemySpawnCount_);
			} else {
				return (0);
			}
		}();
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
		const WaveSpawnSettings* wave = FindActiveSpawnSettings(elapsedTime_, waveIndex);
		if (!wave) {
			return 0;
		}

		ChangeActiveWave(waveIndex);
		return SpawnBatch(*wave, spawnCount);
	}

	const WaveSpawnSettings* Spawner::FindActiveSpawnSettings(float elapsedTime, std::size_t& outIndex) const {
		if (elapsedTime >= timeLimit_) {

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
			if (elapsedTime < wave.startTime || elapsedTime >= wave.endTime) {
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

	void Spawner::UpdateStatIncreases(float previousElapsedTime) {
		const std::vector<WaveSettings>& waves = Settings::GetInstance().GetWaves();
		float segmentStartTime = previousElapsedTime;
		if (previousElapsedTime < timeLimit_ && elapsedTime_ >= timeLimit_) {

			// 時間切れ以降の強化を独立させ、境界をまたぐFrameではBonus開始後の時間だけを集計
			accumulatedHealthIncrease_ = 0.0;
			accumulatedPowerIncrease_ = 0.0;
			accumulatedMoveSpeedIncrease_ = 0.0;
			segmentStartTime = timeLimit_;
		}
		while (segmentStartTime < elapsedTime_) {
			std::size_t waveIndex = kInvalidWaveIndex;
			const WaveSpawnSettings* wave = FindActiveSpawnSettings(segmentStartTime, waveIndex);
			float segmentEndTime = elapsedTime_;
			float waveStartTime = timeLimit_;
			if (segmentStartTime < timeLimit_) {

				// 長いFrameでもWave境界と空白時間を順にたどり、各設定が有効な区間だけを強化対象化
				segmentEndTime = (std::min)(segmentEndTime, timeLimit_);
				for (const WaveSettings& scheduledWave : waves) {
					if (scheduledWave.startTime > segmentStartTime) {
						segmentEndTime = (std::min)(segmentEndTime, scheduledWave.startTime);
					}
					if (scheduledWave.endTime > segmentStartTime) {
						segmentEndTime = (std::min)(segmentEndTime, scheduledWave.endTime);
					}
				}
				if (wave) {
					waveStartTime = waves[waveIndex].startTime;
				}
			}

			// 次のWaveへは確定した加算量だけを継承し、未完了の強化周期はWaveごとに区切り
			if (wave) {
				const float previousWaveElapsedTime = (std::max)(0.0f, segmentStartTime - waveStartTime);
				const float waveElapsedTime = (std::max)(0.0f, segmentEndTime - waveStartTime);
				accumulatedHealthIncrease_ += CalculateStatIncrease(
					previousWaveElapsedTime, waveElapsedTime, wave->healthIncrease);
				accumulatedPowerIncrease_ += CalculateStatIncrease(
					previousWaveElapsedTime, waveElapsedTime, wave->powerIncrease);
				accumulatedMoveSpeedIncrease_ += CalculateStatIncrease(
					previousWaveElapsedTime, waveElapsedTime, wave->moveSpeedIncrease);
			}
			segmentStartTime = segmentEndTime;
		}
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

	std::uint32_t Spawner::CalculateSpawnCount(const WaveSpawnSettings& wave, float spawnTime, std::uint64_t karmaBonusSpawnCount) const {
		double spawnCount = static_cast<double>(wave.spawnCount) + static_cast<double>(karmaBonusSpawnCount);
		if (activeWaveIndex_ == kBonusWaveIndex) {

			// 通常Waveの時間を含めず、Bonus Wave開始後に完了した周期だけを生成数へ加算
			const TimedSpawnCountIncreaseSettings& increase = Settings::GetInstance().GetBonusWaveSettings().spawnCountIncrease;
			const double interval = (std::max)(0.01, static_cast<double>(increase.interval));
			const double bonusElapsedTime = (std::max)(0.0, static_cast<double>(spawnTime) - timeLimit_);
			const double completedIntervals = std::floor(bonusElapsedTime / interval);
			spawnCount += completedIntervals * static_cast<double>(increase.amount);
		}

		// 長時間の継続や大きな増加量でも整数への変換前に上限を保証
		return static_cast<std::uint32_t>((std::min)(
			spawnCount, static_cast<double>((std::numeric_limits<std::uint32_t>::max)())));
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

		// 生成率を相対的な重みとして扱い、負の値を抽選対象から除外
		const float normalRate = std::max(0.0f, wave.normalSpawnRate);
		const float runnerRate = std::max(0.0f, wave.runnerSpawnRate);
		const float tankRate = std::max(0.0f, wave.tankSpawnRate);
		const float overTimeRate = std::max(0.0f, wave.overTimeSpawnRate);
		const float totalRate = normalRate + runnerRate + tankRate + overTimeRate;
		if (totalRate <= 0.0f) {
			return Data::Type::Normal;
		}

		// 各種類へ重ならない抽選区間を割り当て、最後の区間をOverTimeへ割り当て
		const float randomValue = MyRand::GetFloat(0.0f, totalRate);
		if (randomValue < normalRate) {
			return Data::Type::Normal;
		}
		if (randomValue < normalRate + runnerRate) {
			return Data::Type::Runner;
		}

		if (randomValue < normalRate + runnerRate + tankRate) {
			return Data::Type::Tank;
		}

		return Data::Type::OverTime;
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
		desc.status = CalculateSpawnStatus(spawnType);
		desc.type = spawnType;
		++activeWaveEnemySpawnCount_;
		const std::uint64_t eliteSpawnInterval = std::max<std::uint64_t>(1, wave.eliteSpawnInterval);

		// Wave内で指定周期に到達したEnemyだけを種類に依存せずElite化
		if (activeWaveEnemySpawnCount_ % eliteSpawnInterval == 0) {
			desc.bonusType = Data::BonusType::Elite;
		} else {
			desc.bonusType = Data::BonusType::None;
		}
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
			if (foundBlock && foundSlope) {
				outGroundSurfaceY = std::max(blockSurfaceY, slopeSurfaceY);
			} else {
				outGroundSurfaceY = [&]() {
					if (foundBlock) {
						return (blockSurfaceY);
					} else {
						return (slopeSurfaceY);
					}
				}();
			}
			outPosition = groundQueryOrigin;
			outPosition.y = outGroundSurfaceY - kSpawnBuriedDepth;
			return true;
		}

		return false;
	}

	Data::Status Spawner::CalculateSpawnStatus(Data::Type type) const {
		Data::Status status = Factory::CreateDefaultStatus(type);

		// 通常Wave群またはBonus Waveで蓄積した加算分をEnemy種類ごとの基礎能力値へ新規生成時だけ適用
		status.currentHealth = ApplyStatIncrease(status.currentHealth, accumulatedHealthIncrease_);
		status.power = ApplyStatIncrease(status.power, accumulatedPowerIncrease_);
		status.moveSpeed = ApplyStatIncrease(status.moveSpeed, accumulatedMoveSpeedIncrease_);
		return status;
	}

} // namespace Enemy
