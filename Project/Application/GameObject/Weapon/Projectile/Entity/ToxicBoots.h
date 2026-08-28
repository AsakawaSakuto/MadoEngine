#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class ToxicBoots : public IProjectile {
	public:
		/// @brief ToxicBootsのデストラクタ
		~ToxicBoots() override;

		/// @brief ToxicBootsを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief ToxicBootsを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:
		float kBaseAttackRadius = 2.0f;    // 攻撃範囲の基本半径
		float kBaseModelScale = 2.0f;      // Eyeモデルの基本スケール
		float kRotationSpeed = 3.14f;      // Y軸回転速度
		float kReductionStartRatio = 0.9f; // 縮小開始時の寿命進行率

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		GameTimer reductionTimer_;
		bool isReductionStarted_ = false;
	};
}
