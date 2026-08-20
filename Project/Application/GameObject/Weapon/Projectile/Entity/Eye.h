#pragma once
#include "../IProjectile.h"
#include "Utility/GameTimer/GameTimer.h"
#include <string>

namespace Projectile {

	/// @brief 武器所有者へ追従する常時展開型Projectile
	class Eye : public IProjectile {
	public:
		/// @brief Eyeのデストラクタ
		~Eye() override;

		/// @brief Eyeを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Eyeを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 武器所有者の位置と現在の強化値を同期
		/// @param context 同期するProjectile情報
		void SynchronizePersistentState(const InitializeDesc& context) override;

	private:
		/// @brief Eye Effect Sequenceのループ再生を開始
		void StartEffectSequence();

		/// @brief Eye Effect Sequenceを現在位置と攻撃範囲へ同期
		void UpdateEffectSequenceTransform();

		/// @brief Eye Effect Sequenceを即時停止
		void StopEffectSequence();

		/// @brief ModelとEffectの色を経過時間に応じて更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateColor(float deltaTime);

		static constexpr float kBaseAttackRadius = 2.0f; // 攻撃範囲の基本半径
		static constexpr float kBaseModelScale =   2.0f; // モデルの基本スケール
		static constexpr float kMinSizeRate =      0.1f; // 攻撃範囲の最小倍率、0.0f以下は反転やゼロ半径になるため制限
		static constexpr float kRotationSpeed =    1.0f; // Eyeを回転させる速度
		static constexpr float kColorCycleDuration = 5.0f; // 色が開始色へ戻るまでの時間
		static constexpr Vector4 kColorStart = { 1.0f, 0.0f, 1.0f, 1.0f };
		static constexpr Vector4 kColorEnd = { 1.0f, 0.5f, 1.0f, 1.0f };

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		MadoEngine::EffectSequence::MyEffectSequence3d effectSequence_;
		GameTimer colorAnimationTimer_;
	};
}
