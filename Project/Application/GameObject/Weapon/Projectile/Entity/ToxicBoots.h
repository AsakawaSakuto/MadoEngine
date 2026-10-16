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
		/// @brief Modelの色を経過時間に応じて更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateColor(float deltaTime);

		float kBaseAttackRadius_ = 2.0f;    // 攻撃範囲の基本半径
		float kBaseModelScale_ = 2.0f;      // Eyeモデルの基本スケール
		float kRotationSpeed_ = 3.14f;      // Y軸回転速度
		float kGroundOffset_ = 0.05f;       // 地面とのZファイティングを防ぐ表示オフセット
		float kReductionStartRatio_ = 0.7f; // 縮小開始時の寿命進行率
		static constexpr float kColorCycleDuration = 5.0f; // 色が開始色へ戻るまでの時間
		static constexpr Vector4 kColorStart = { 0.5f, 1.0f, 0.0f, 0.95f };
		static constexpr Vector4 kColorEnd = { 0.0f, 1.0f, 0.0f, 0.95f };

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		GameTimer reductionTimer_;
		GameTimer colorAnimationTimer_;
		Vector3 groundNormal_ = { 0.0f, 1.0f, 0.0f };
		float rotationYaw_ = 0.0f;
		bool isReductionStarted_ = false;
	};
}
