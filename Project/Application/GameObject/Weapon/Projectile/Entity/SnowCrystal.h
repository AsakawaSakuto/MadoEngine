#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	/// @brief 前方へ飛んだ後に武器所有者へ戻るProjectile
	class SnowCrystal : public IProjectile {
	public:
		/// @brief SnowCrystalのデストラクタ
		~SnowCrystal() override;

		/// @brief SnowCrystalの発射方向と初速度、斜面に沿った姿勢を初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief 減速による折り返し、斜面に沿った回転と寿命を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 復路の目標となる武器所有者の現在座標を同期
		/// @param ownerPosition 武器所有者の現在座標
		void SynchronizeOwnerPosition(const Vector3& ownerPosition) override;

	private:
		/// @brief 速度の減算による往復移動と所有者への到着を更新
		/// @param deltaTime 移動へ適用する経過時間
		void UpdateMovement(float deltaTime);

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		Vector3 groundNormal_ = { 0.0f, 1.0f, 0.0f }; // 発射時の斜面に沿った回転軸
		float rotationYaw_ = 0.0f; // 斜面上の回転角度
		float currentSpeed_ = 0.0f;
	};
}
