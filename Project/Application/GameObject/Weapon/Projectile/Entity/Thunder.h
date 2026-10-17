#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	/// @brief ロックオン対象の頭上から落下するProjectile
	class Thunder : public IProjectile {
	public:
		/// @brief Thunderのデストラクタ
		~Thunder() override;

		/// @brief ロックオン対象の頭上でThunderを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Thunderの移動と寿命を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 初回のEnemy命中で落下を終了
		void OnEnemyHit() override;

	private:
		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		int initialBounceCount_ = 0;
		bool hasHitEnemy_ = false;
	};
}
