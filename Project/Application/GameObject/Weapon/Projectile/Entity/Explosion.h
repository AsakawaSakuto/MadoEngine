#pragma once
#include "../IProjectile.h"
#include <string>

namespace Projectile {

	class Explosion : public IProjectile {
	public:
		/// @brief 元Projectileの現在位置に攻撃情報を引き継いだ爆発を生成
		/// @param source 爆発の発生元Projectile
		/// @param radius 爆発の半径
		/// @param damageRate 元Projectileのダメージに対する倍率
		static void SpawnFromProjectile(const IProjectile& source, float radius, float damageRate);

		/// @brief Explosionのデストラクタ
		~Explosion() override;

		/// @brief Explosionを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Explosionを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		std::string objectName_;
	};
}
