#include "EnemySpawner.h"
#include "GameObject/Player/Player.h"
#include "Utility/Collider/MyCollider.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kSpawnBuriedDepth = 2.0f;
	constexpr float kMinSpawnInterval = 0.1f;
	constexpr float kSecondsPerMinute = 60.0f;
	constexpr std::size_t kMaxSpawnPositionAttempts = 8;
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

		// 長いFrameでも経過した生成周期を取りこぼさないようTimer残量を順次消費
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
		ImGui::DragScalar("Spawn Limit", ImGuiDataType_U64, &spawnLimit_, 1.0f);
		ImGui::DragFloat("生成間隔（秒）", &spawnInterval_, 0.1f, kMinSpawnInterval, 600.0f, "%.1f");
		ImGui::DragFloat("体力・攻撃力強化率（毎分）", &healthPowerGrowthRatePerMinute_, 0.01f, 0.0f, 10.0f, "%.2f");
		ImGui::DragFloat("移動速度強化率（毎分）", &moveSpeedGrowthRatePerMinute_, 0.01f, 0.0f, 10.0f, "%.2f");

		// 直接入力されたDebug値も実行可能な範囲へ制限
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
		float groundSurfaceY = 0.0f;
		if (!TryCreateSpawnPosition(desc.position, groundSurfaceY)) {
			return;
		}

		// 地形Colliderを無効化した出現状態で地表面直下から上昇
		desc.emergeFromGround = true;
		desc.groundSurfaceY = groundSurfaceY;
		desc.status = CalculateSpawnStatus();
		desc.type = Data::Type::Normal;
		desc.bonusType = Data::BonusType::None;
		desc.sceneType = sceneType_;
		enemyManager_->Spawn(desc);
	}

	bool Spawner::TryCreateSpawnPosition(Vector3& outPosition, float& outGroundSurfaceY) const {
		const Vector3 playerPosition = player_->GetPosition();
		const float groundSearchDistance = mapLimit_.max.y - mapLimit_.min.y;

		// 穴や地形未配置地点を避けるためPlayer周辺の候補を複数回探索
		for (std::size_t attempt = 0; attempt < kMaxSpawnPositionAttempts; ++attempt) {
			const float angle = MyRand::GetFloat(0.0f, kPi * 2.0f);
			const float radius = MyRand::GetFloat(minSpawnRadius_, maxSpawnRadius_);
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

	Data::Status Spawner::CalculateSpawnStatus() const {

		// 経過分数へ線形成長率を適用して長時間Play時の難易度を上昇
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
