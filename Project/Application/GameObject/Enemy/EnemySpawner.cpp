#include "EnemySpawner.h"
#include "GameObject/Player/Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kSpawnHeightOffset = 1.0f;
}

void EnemySpawner::Initialize(Player::Base* player, SceneType sceneType) {
	player_ = player;
	sceneType_ = sceneType;
	spawnTimer_ = 0.0f;
	nextEnemyId_ = 0;
	enemies_.clear();
	Logger::Output("[Engine] EnemySpawnerを初期化しました。", Logger::Level::Application);
}

void EnemySpawner::Update(float deltaTime) {
	if (!player_) {
		return;
	}

	spawnTimer_ += deltaTime;
	while (spawnTimer_ >= spawnInterval_) {
		spawnTimer_ -= spawnInterval_;
		if (enemies_.size() <= 99) {
			SpawnEnemy();
		}
	}

	for (std::unique_ptr<Enemy>& enemy : enemies_) {
		enemy->Update(deltaTime);
	}

	MyCollider::Update();

	for (std::unique_ptr<Enemy>& enemy : enemies_) {
		enemy->ResolveAfterCollision();
		if (enemy->IsHitPlayerProjectile()) {
			enemy->Kill();
			Logger::Output("[Debug] EnemyがPlayerのProjectileに接触したため削除します。", Logger::Level::Debug);
			continue;
		}

		if (enemy->IsHitPlayer()) {
			player_->TakeDamage(1);
			enemy->Kill();
			Logger::Output("[Engine] EnemyがPlayerに接触したため削除します。", Logger::Level::Debug);
		}

		if (MyInput::GetKeybord()->IsTrigger(DIK_7)) {
			enemy->Kill();
		}
	}

	RemoveDeadEnemies();
}

void EnemySpawner::Clear() {
	enemies_.clear();
	spawnTimer_ = 0.0f;
}

bool EnemySpawner::TryGetNearestEnemyPosition(Vector3& outPosition) const {
	if (!player_) {
		return false;
	}

	const Vector3 playerPosition = player_->GetPosition();
	float nearestDistanceSq = 0.0f;
	bool foundEnemy = false;

	for (const std::unique_ptr<Enemy>& enemy : enemies_) {
		if (!enemy || !enemy->IsActive()) {
			continue;
		}

		const Vector3 enemyPosition = enemy->GetPosition();
		const Vector3 toEnemy = enemyPosition - playerPosition;
		const float distanceSq = toEnemy.LengthSq();
		if (!foundEnemy || distanceSq < nearestDistanceSq) {
			nearestDistanceSq = distanceSq;
			outPosition = enemyPosition;
			foundEnemy = true;
		}
	}

	return foundEnemy;
}

Vector3 EnemySpawner::GetNearestEnemyPosition() const {
	Vector3 nearestEnemyPosition = { 0.0f, 0.0f, 0.0f };
	TryGetNearestEnemyPosition(nearestEnemyPosition);
	return nearestEnemyPosition;
}

void EnemySpawner::SpawnEnemy() {
	std::unique_ptr<Enemy> enemy = std::make_unique<Enemy>();
	enemy->Initialize(nextEnemyId_++, CreateSpawnPosition(), sceneType_);
	enemy->SetTargetPlayer(player_);
	enemies_.push_back(std::move(enemy));

	Logger::Output("[Engine] Enemyを生成しました。現在のEnemy数 : " + std::to_string(enemies_.size()), Logger::Level::Application);
}

Vector3 EnemySpawner::CreateSpawnPosition() const {
	const Vector3 playerPosition = player_->GetPosition();
	const float angle = MyRand::GetFloat(0.0f, kPi * 2.0f);
	const float radius = MyRand::GetFloat(minSpawnRadius_, maxSpawnRadius_);

	Vector3 spawnPosition = {
		playerPosition.x + std::sin(angle) * radius,
		playerPosition.y + kSpawnHeightOffset,
		playerPosition.z + std::cos(angle) * radius
	};

	spawnPosition.x = std::clamp(spawnPosition.x, -7.5f, 292.5f);
	spawnPosition.y = std::clamp(spawnPosition.y, 0.0f, 100.0f);
	spawnPosition.z = std::clamp(spawnPosition.z, -7.5f, 292.5f);
	return spawnPosition;
}

void EnemySpawner::RemoveDeadEnemies() {
	enemies_.erase(
		std::remove_if(
			enemies_.begin(),
			enemies_.end(),
			[](const std::unique_ptr<Enemy>& enemy) {
				return !enemy->IsActive();
			}),
		enemies_.end());
}
