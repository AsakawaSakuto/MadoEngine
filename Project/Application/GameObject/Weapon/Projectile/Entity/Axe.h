#pragma once
#include "../IProjectile.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
#include <array>
#include <cstdint>
#include <string>

namespace Projectile {

	class Axe : public IProjectile {
	public:
		/// @brief Axeのデストラクタ
		~Axe() override;

		/// @brief Axeを初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief Axeを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

	private:

		static constexpr std::array<std::uint32_t, 2> kTrailVertexIndices = { 1897, 903 };

		MadoEngine::ModelHandle model_{};
		std::array<MadoEngine::Ribbon::RibbonEffectHandle, kTrailVertexIndices.size()> trails_{};
		std::string objectName_;

		GameTimer startTimer_;
		GameTimer reductionTimer_;
		bool isReductionStarted_ = false;
	};
}
