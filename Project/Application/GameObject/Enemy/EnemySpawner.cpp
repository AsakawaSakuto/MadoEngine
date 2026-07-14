#include "EnemySpawner.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
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
}

void EnemySpawner::Initialize(Player::Base* player, SceneType sceneType) {
	player_ = player;
	sceneType_ = sceneType;
	spawnTimer_ = 0.0f;
	nextEnemyId_ = 0;
	isActive_ = true;
	enemies_.clear();
	Logger::Output("[Engine] EnemySpawnerを初期化しました。", Logger::Level::Application);
}

void EnemySpawner::Update(float deltaTime) {
	if (!player_) {
		return;
	}

	if (isActive_) {
		spawnTimer_ += deltaTime;
		while (spawnTimer_ >= spawnInterval_) {
			spawnTimer_ -= spawnInterval_;
			if (enemies_.size() <= spawnLimit_) {
				SpawnEnemy();
			}
		}
	}

	for (std::unique_ptr<Enemy>& enemy : enemies_) {
		enemy->Update(deltaTime);
	}

	MyCollider::Update();

	Projectile::Manager& projectileManager = Projectile::Manager::GetInstance();
	std::vector<Projectile::HitInfo> projectileHitInfos;
	for (std::unique_ptr<Enemy>& enemy : enemies_) {
		enemy->ResolveAfterCollision();

		projectileManager.CollectHitsAgainst(enemy->GetHitColliderName(), projectileHitInfos);
		for (const Projectile::HitInfo& hitInfo : projectileHitInfos) {
			if (enemy->TakeProjectileDamage(hitInfo.projectileId, hitInfo.damage)) {
				break;
			}
		}

		if (!enemy->IsActive()) {
			continue;
		}

		if (enemy->IsHitPlayer()) {
			player_->TakeDamage(1);
			enemy->Kill();
		}

		if (MyInput::GetKeybord()->IsTrigger(DIK_7)) {
			enemy->Kill();
		}
	}

	RemoveDeadEnemies();
}

void EnemySpawner::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("EnemySpawner");
	ImGui::Text("Enemy Count : %zu", GetEnemyCount());
	ImGui::Checkbox("Active", &isActive_);
	ImGui::DragFloat("生成間隔（秒）", &spawnInterval_, 0.1f, kMinSpawnInterval, 600.0f, "%.1f");
	spawnInterval_ = std::max(spawnInterval_, kMinSpawnInterval);

	if (ImGui::Button("敵を1体生成") && player_ && enemies_.size() <= spawnLimit_) {
		SpawnEnemy();
	}

	ImGui::End();
#endif // USE_IMGUI
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
