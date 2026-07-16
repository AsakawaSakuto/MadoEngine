#include "EnemySpawner.h"
#include "GameObject/Player/Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kSpawnHeightOffset = 1.0f;
	constexpr float kMinSpawnInterval = 0.1f;
	constexpr float kSecondsPerMinute = 60.0f;
} // namespace

namespace Enemy {

	void Spawner::Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType) {
		player_ = player;
		enemyManager_ = enemyManager;
		sceneType_ = sceneType;
		Clear();
		isActive_ = true;
		Logger::Output("[Engine] Enemy::Spawnerを初期化しました。", Logger::Level::Application);
	}

	void Spawner::Update(float deltaTime) {
		if (!isActive_ || !player_ || !enemyManager_ || deltaTime <= 0.0f) {
			return;
		}

		elapsedTime_ += deltaTime;
		spawnTimer_ += deltaTime;
		spawnInterval_ = std::max(spawnInterval_, kMinSpawnInterval);
		while (spawnTimer_ >= spawnInterval_) {
			spawnTimer_ -= spawnInterval_;
			if (enemyManager_->GetEnemyCount() < spawnLimit_) {
				SpawnEnemy();
			}
		}
	}

	void Spawner::DrawImGui() {
#ifdef USE_IMGUI
		ImGui::Begin("EnemySpawner");
		ImGui::Text("Enemy Count : %zu", enemyManager_ ? enemyManager_->GetEnemyCount() : 0);
		ImGui::Text("Elapsed Time : %.1f", elapsedTime_);
		ImGui::Checkbox("Active", &isActive_);
		ImGui::DragFloat("生成間隔（秒）", &spawnInterval_, 0.1f, kMinSpawnInterval, 600.0f, "%.1f");
		ImGui::DragFloat("体力・攻撃力強化率（毎分）", &healthPowerGrowthRatePerMinute_, 0.01f, 0.0f, 10.0f, "%.2f");
		ImGui::DragFloat("移動速度強化率（毎分）", &moveSpeedGrowthRatePerMinute_, 0.01f, 0.0f, 10.0f, "%.2f");
		spawnInterval_ = std::max(spawnInterval_, kMinSpawnInterval);
		healthPowerGrowthRatePerMinute_ = std::max(healthPowerGrowthRatePerMinute_, 0.0f);
		moveSpeedGrowthRatePerMinute_ = std::max(moveSpeedGrowthRatePerMinute_, 0.0f);

		if (ImGui::Button("敵を1体生成") && player_ && enemyManager_ && enemyManager_->GetEnemyCount() < spawnLimit_) {
			SpawnEnemy();
		}

		ImGui::End();
#endif // USE_IMGUI
	}

	void Spawner::Clear() {
		spawnTimer_ = 0.0f;
		elapsedTime_ = 0.0f;
	}

	void Spawner::SpawnEnemy() {
		if (!player_ || !enemyManager_) {
			return;
		}

		SpawnDesc desc;
		desc.position = CreateSpawnPosition();
		desc.status = CalculateSpawnStatus();
		desc.type = Data::Type::Normal;
		desc.sceneType = sceneType_;
		enemyManager_->Spawn(desc);
	}

	Vector3 Spawner::CreateSpawnPosition() const {
		const Vector3 playerPosition = player_->GetPosition();
		const float angle = MyRand::GetFloat(0.0f, kPi * 2.0f);
		const float radius = MyRand::GetFloat(minSpawnRadius_, maxSpawnRadius_);

		Vector3 spawnPosition = { playerPosition.x + std::sin(angle) * radius, playerPosition.y + kSpawnHeightOffset,
								  playerPosition.z + std::cos(angle) * radius };

		spawnPosition.x = std::clamp(spawnPosition.x, mapLimit_.min.x, mapLimit_.max.x);
		spawnPosition.y = std::clamp(spawnPosition.y, mapLimit_.min.y, mapLimit_.max.y);
		spawnPosition.z = std::clamp(spawnPosition.z, mapLimit_.min.z, mapLimit_.max.z);
		return spawnPosition;
	}

	Data::Status Spawner::CalculateSpawnStatus() const {
		const float elapsedMinutes = elapsedTime_ / kSecondsPerMinute;
		const float healthPowerMultiplier = 1.0f + elapsedMinutes * healthPowerGrowthRatePerMinute_;
		const float moveSpeedMultiplier = 1.0f + elapsedMinutes * moveSpeedGrowthRatePerMinute_;

		Data::Status status = baseStatus_;
		status.currentHealth *= healthPowerMultiplier;
		status.power *= healthPowerMultiplier;
		status.moveSpeed *= moveSpeedMultiplier;
		return status;
	}

} // namespace Enemy
