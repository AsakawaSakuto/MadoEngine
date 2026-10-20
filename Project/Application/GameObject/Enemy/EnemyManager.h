#pragma once
#include "EnemyBase.h"
#include "GameObject/Combat/DamageEvent.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Player {
	class Base;
}

namespace Enemy {
	/// @brief Enemy頭上の状態異常表示に使用する読み取り専用データ
	struct StatusEffectDisplayData {
		std::uint32_t enemyId = 0;
		Vector3 anchorPosition = { 0.0f, 0.0f, 0.0f };
		std::array<bool, StatusEffect::kTypeCount> activeEffects{};
	};

	/// @brief 生成されたEnemyの所有と一括処理を管理するクラス
	class Manager {
	public:
		/// @brief Enemy::Managerを初期化
		/// @param player Enemyとの相互作用に使用するPlayer
		void Initialize(Player::Base* player);

		/// @brief Enemyを生成して管理対象へ登録
		/// @param desc Enemyの生成情報
		void Spawn(const SpawnDesc& desc);

		/// @brief 指定座標へBossを生成して管理対象へ登録
		/// @param position Bossの生成座標
		/// @param sceneType Bossを所属させるシーン種別
		void SpawnBoss(const Vector3& position, SceneType sceneType);

		/// @brief 管理中のEnemyを一括更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Collider更新後にEnemyの衝突とダメージを一括解決
		void ResolveAfterCollision();

		/// @brief 管理中のEnemyのColliderをDebugLineへ登録
		void DrawDebugLine() const;

		/// @brief 管理中のEnemyをすべて破棄
		void Clear();

		/// @brief 新規生成と管理中Enemyの移動制限を設定
		/// @param mapLimit Map外周と高さを表す移動制限
		void SetMapLimit(const MapLimit& mapLimit);

		/// @brief 現在管理しているEnemy数を取得
		/// @return 現在管理しているEnemy数
		std::size_t GetEnemyCount() const { return enemies_.size(); }

		/// @brief Playerに最も近いEnemyの座標を取得
		/// @param outPosition 最も近いEnemyの座標を受け取る変数
		/// @return 取得できた場合はtrue
		bool TryGetNearestEnemyPosition(Vector3& outPosition) const;

		/// @brief Playerに最も近いEnemyの座標を取得
		/// @return 最も近いEnemyの座標、Enemyが存在しない場合はゼロ座標
		Vector3 GetNearestEnemyPosition() const;

		/// @brief Player周辺の区画から攻撃範囲内のEnemy座標を距離順で上限件数まで収集
		/// @param maxCount 収集するEnemy数の上限、0以下の場合は空配列
		/// @param outPositions 距離順のEnemy座標の格納先
		void CollectNearestAttackTargetPositions(int maxCount, std::vector<Vector3>& outPositions) const;

		/// @brief 未処理のEnemyダメージイベントを取得してキューをクリア
		/// @return 発生順に格納されたEnemyダメージイベント
		std::vector<Combat::DamageEvent> ConsumeDamageEvents();

		/// @brief 生存中Enemyの状態異常表示データを収集
		/// @param outDisplayData 状態異常表示データの格納先
		void CollectStatusEffectDisplayData(
			std::vector<StatusEffectDisplayData>& outDisplayData) const;

	private:
		/// @brief Enemyの現在座標と有効状態を攻撃対象の区画索引へ同期
		/// @param enemy 同期対象のEnemy
		void SynchronizeAttackTargetCell(Base& enemy);

		/// @brief Enemyを攻撃対象の区画索引から除外
		/// @param enemy 除外対象のEnemy
		void RemoveAttackTargetCell(const Base& enemy);

		/// @brief ProjectileとEnemyの衝突結果を処理
		void ProcessProjectileHits();

		/// @brief PlayerとEnemyの接触結果を処理
		void ProcessPlayerCollisions();

		/// @brief Player周辺の削除範囲外にいるEnemyを無効化
		void DeactivateEnemiesOutsidePlayerRange();

		/// @brief 無効になったEnemyを管理対象から削除
		void RemoveInactiveEnemies();

		Player::Base* player_ = nullptr;
		std::vector<std::unique_ptr<Base>> enemies_;
		std::unordered_map<std::uint64_t, std::vector<Base*>> attackTargetCells_;
		std::unordered_map<std::uint32_t, std::uint64_t> attackTargetCellKeys_;
		float maxAttackTargetHorizontalExtent_ = 0.0f;
		std::vector<Combat::DamageEvent> damageEvents_;
		MapLimit mapLimit_;
		std::uint32_t nextEnemyId_ = 0;
	};
} // namespace Enemy
