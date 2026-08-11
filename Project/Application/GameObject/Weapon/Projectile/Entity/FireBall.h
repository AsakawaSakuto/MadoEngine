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
		/// @brief FireBall Effect Sequenceのループ再生を開始
		void StartEffectSequence();

		/// @brief FireBall Effect Sequenceを現在位置へ追従
		void UpdateEffectSequenceTransform();

		/// @brief FireBall Effect Sequenceを即時停止
		void StopEffectSequence();

		/// @brief 現在座標に爆発を生成
		void SpawnExplosion();

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		MadoEngine::EffectSequence::MyEffectSequence3d effectSequence_;
	};
}
