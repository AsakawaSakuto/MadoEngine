#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class FireBall : public IProjectile {
	public:
		/// @brief FireBallのデストラクタ
		~FireBall() override;

		/// @brief FireBallを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief FireBallを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Enemy命中時に爆発を生成
		void OnEnemyHit() override;

	private:
		/// @brief FireBallパーティクルのループ再生を開始する
		void StartParticle();

		/// @brief FireBallパーティクルを現在位置へ追従させる
		void UpdateParticleTransform();

		/// @brief FireBallパーティクルを即時停止する
		void StopParticle();

		/// @brief 現在座標に爆発を生成
		void SpawnExplosion();

		Model* model_ = nullptr;
		std::string objectName_;
		MadoEngine::Particle::EffectHandle particleHandle_;
	};
}
