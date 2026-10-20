#pragma once
#include "../IProjectile.h"
#include <string>
#include <unordered_map>

namespace Projectile {

	/// @brief 武器所有者の傾きに沿った面を指定回数だけ連続回転する近接Projectile
	class Sword : public IProjectile {
	public:
		/// @brief Swordのデストラクタ
		~Sword() override;

		/// @brief Swordの開始角度と連続回転のTimerを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief 連続回転の薙ぎ払いと終了状態を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 指定した速度と回転数から連続回転の所要時間を計算
		/// @param sweepSpeed 一秒あたりの回転角度、単位はラジアン
		/// @param sweepCount 連続する回転数
		/// @return 連続回転の所要時間、単位は秒
		static float CalculateSweepDuration(float sweepSpeed, int sweepCount);

		/// @brief 薙ぎ払いの中心を武器所有者へ追従
		/// @param ownerPosition 武器所有者の現在座標
		void SynchronizeOwnerPosition(const Vector3& ownerPosition) override;

		/// @brief 衝突解決後の所有者座標と傾きをModelとColliderへ同期
		/// @param ownerPosition 武器所有者の現在座標
		/// @param ownerUpDirection 武器所有者の正規化済み上方向
		void SynchronizeOwnerPose(const Vector3& ownerPosition, const Vector3& ownerUpDirection) override;

		/// @brief Frame内に通過した剣の軌道とEnemyの接触を判定
		/// @param enemyColliderName EnemyのCollider名
		/// @return 軌道上で接触している場合はtrue
		bool IsHitEnemy(const std::string& enemyColliderName) override;

		/// @brief 一回転につき同じEnemyへの命中を一回だけ受付
		/// @param enemyId 衝突したEnemyの識別番号
		/// @param bounceTargetPosition 共通インターフェースの跳弾先座標、Swordでは未使用
		/// @return ダメージを適用する新しい衝突の場合はtrue
		bool HandleEnemyCollision(std::uint32_t enemyId, const Vector3* bounceTargetPosition) override;

	private:
		/// @brief Player中心と回転進捗をModelとColliderへ反映
		void UpdatePose();

		/// @brief 所有者の上方向を回転軸とした薙ぎ払い姿勢を作成
		/// @param yaw 周回面内の連続回転角度
		/// @param upDirection 周回面の正規化済み上方向
		/// @return ModelとOBBに共通のEuler角
		Vector3 CreateSweepRotation(float yaw, const Vector3& upDirection) const;

		/// @brief 開始角度から数えた回転の周回番号を取得
		/// @param yaw 周回面内の連続回転角度
		/// @return ゼロ始まりの周回番号
		int GetSweepIndex(float yaw) const;

		/// @brief 指定した角度と支点へ剣のOBBを同期
		/// @param yaw 周回面内の連続回転角度
		/// @param pivotPosition 薙ぎ払いの支点座標
		/// @param upDirection 周回面の正規化済み上方向
		void UpdateHitbox(float yaw, const Vector3& pivotPosition, const Vector3& upDirection);

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		GameTimer sweepTimer_;
		std::unordered_map<std::uint32_t, int> lastHitSweepIndices_;
		Vector3 hitboxPosition_;
		Vector3 previousPivotPosition_;
		Vector3 ownerUpDirection_ = { 0.0f, 1.0f, 0.0f };
		Vector3 previousUpDirection_ = { 0.0f, 1.0f, 0.0f };
		float startYaw_ = 0.0f;
		float sweepYaw_ = 0.0f;
		float previousSweepYaw_ = 0.0f;
		int sweepCount_ = 1;
		int collisionSweepIndex_ = 0;
	};
}
