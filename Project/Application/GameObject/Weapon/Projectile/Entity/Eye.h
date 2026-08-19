#pragma once
#include "../IProjectile.h"
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
		static constexpr float kBaseAttackRadius = 2.0f; // 攻撃範囲の基本半径
		static constexpr float kBaseModelScale =   2.0f; // モデルの基本スケール
		static constexpr float kMinSizeRate =      0.1f; // 攻撃範囲の最小倍率、0.0f以下は反転やゼロ半径になるため制限
		static constexpr float kRotationSpeed =    1.0f; // Eyeを回転させる速度

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
	};
}
