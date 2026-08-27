#pragma once
#include "EnemyManager.h"
#include "EnemySettings.h"
#include "GameObject/Map/MapLimit.h"
#include <cstddef>
#include <cstdint>

namespace Player {
	class Base;
}

namespace Enemy {

	/// @brief 時間経過に応じてEnemyの生成要求を発行するクラス
	class Spawner {
	public:
		/// @brief Enemy::Spawnerを初期化
		/// @param player 生成位置の基準になるPlayer
		/// @param enemyManager Enemyの生成要求を登録するManager
		/// @param sceneType Enemyを所属させるシーン種別
		/// @param timeLimit Bonus Waveへ切り替えるゲーム制限時間
		void Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType, float timeLimit);

		/// @brief 経過時間を更新し、生成条件を満たしたEnemyをManagerへ登録
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 生成時間と強化時間を初期化
		void Clear();

		/// @brief Enemy生成位置のMap制限を設定
		/// @param mapLimit Map外周と高さを表す移動制限
		void SetMapLimit(const MapLimit& mapLimit) { mapLimit_ = mapLimit; }

		/// @brief Enemy自動生成の有効状態を設定
		/// @param isActive 自動生成を有効にする場合はtrue
		void SetActive(bool isActive) { isActive_ = isActive; }

		/// @brief Enemy自動生成の有効状態を取得
		/// @return 自動生成が有効な場合はtrue
		bool IsActive() const { return isActive_; }

		/// @brief Spawnerの経過時間を取得
		/// @return Spawnerの経過時間
		float GetElapsedTime() const { return elapsedTime_; }

		/// @brief Bonus Waveへ切り替えるゲーム制限時間を取得
		/// @return Bonus Waveへ切り替えるゲーム制限時間
		float GetTimeLimit() const { return timeLimit_; }

		/// @brief 現在Wave内で次のElite生成までに必要なEnemy生成数を取得
		/// @return 次のElite生成までに必要な生成数、処理中Waveがない場合は0
		std::uint32_t GetRemainingSpawnCountUntilElite() const;

		/// @brief 現在処理中のWave位置を取得
		/// @param outIndex 現在処理中のWave位置
		/// @return 処理中のWaveが存在する場合はtrue
		bool TryGetActiveWaveIndex(std::size_t& outIndex) const;

		/// @brief Bonus Waveを処理中か確認
		/// @return Bonus Waveを処理中の場合はtrue
		bool IsBonusWaveActive() const;

		/// @brief 現在のWave設定でEnemyを即時生成
		/// @param spawnCount 同時生成するEnemy数
		/// @return 実際に生成したEnemy数
		std::uint32_t SpawnImmediately(std::uint32_t spawnCount);

	private:
		/// @brief 現在時刻に適用する通常WaveまたはBonus Waveを検索
		/// @param outIndex 見つかったWave位置
		/// @return 適用する生成設定、存在しない場合はnullptr
		const WaveSpawnSettings* FindActiveSpawnSettings(std::size_t& outIndex) const;

		/// @brief 処理対象Waveを切り替えて生成周期とElite周期を初期化
		/// @param waveIndex 新しく処理するWave位置
		void ChangeActiveWave(std::size_t waveIndex);

		/// @brief Wave設定に従ってEnemyをまとめて生成
		/// @param wave 適用するWave設定
		/// @param spawnCount 同時生成するEnemy数
		/// @return 実際に生成したEnemy数
		std::uint32_t SpawnBatch(const WaveSpawnSettings& wave, std::uint32_t spawnCount);

		/// @brief Waveの抽選率から生成するEnemy種類を選択
		/// @param wave 適用するWave設定
		/// @return 生成するEnemyの種類
		Data::Type SelectSpawnType(const WaveSpawnSettings& wave) const;

		/// @brief Wave設定によるEnemy生成要求を1件発行
		/// @param wave 適用するWave設定
		/// @return Enemyを生成した場合はtrue
		bool SpawnEnemy(const WaveSpawnSettings& wave);

		/// @brief Player周辺の地表面からEnemyの生成位置を作成
		/// @param wave 適用するWave設定
		/// @param outPosition Enemyの生成位置
		/// @param outGroundSurfaceY 生成地点の地表面Y座標
		/// @return 生成可能な地表面が見つかった場合はtrue
		bool TryCreateSpawnPosition(
			const WaveSpawnSettings& wave,
			Vector3& outPosition,
			float& outGroundSurfaceY) const;

		/// @brief 現在処理中Waveの開始後経過時間を取得
		/// @return 通常Wave開始またはBonus Wave開始からの経過秒数
		float GetActiveWaveElapsedTime() const;

		/// @brief Wave内経過時間に応じたEnemyステータスを計算
		/// @param type ステータスを計算するEnemyの種類
		/// @param wave 適用するWave設定
		/// @param waveElapsedTime Wave開始後の経過秒数
		/// @return 新しく生成するEnemyのステータス
		Data::Status CalculateSpawnStatus(
			Data::Type type,
			const WaveSpawnSettings& wave,
			float waveElapsedTime) const;

		Player::Base* player_ = nullptr;
		Manager* enemyManager_ = nullptr;
		SceneType sceneType_ = SceneType::None;
		MapLimit mapLimit_;
		std::size_t activeWaveIndex_ = static_cast<std::size_t>(-1);
		std::uint64_t activeWaveEnemySpawnCount_ = 0;
		float spawnTimer_ = 0.0f;
		float elapsedTime_ = 0.0f;
		float timeLimit_ = 0.0f;
		bool isActive_ = true;
	};
} // namespace Enemy
