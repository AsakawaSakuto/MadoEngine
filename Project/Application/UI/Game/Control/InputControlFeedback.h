#pragma once
#include "Math/Vector2.h"
#include "Render/Object/ObjectHandle.h"
#include <array>

namespace UI::Game {

	/// @brief 入力操作と押下演出を適用するObjectの対応情報
	struct InputControlFeedbackDefinition {
		const char* actionName;
		std::array<const char*, 3> spriteNames;
		const char* textName;
	};

	/// @brief Editor配置の操作案内に対する共通の押下演出
	class InputControlFeedback final {
	public:
		/// @brief 押下中の縮小と入力解除後の復元を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param isVisible 操作案内を表示する場合はtrue
		/// @param definition 入力名と演出対象のObject名
		void Update(float deltaTime, bool isVisible, const InputControlFeedbackDefinition& definition);

		/// @brief 変更したScaleを基準値へ戻して演出状態を初期化
		void Reset();

	private:
		std::array<MadoEngine::SpriteHandle, 3> sprites_{};
		MadoEngine::TextHandle text_{};
		std::array<Vector2, 3> spriteBaseScales_{};
		Vector2 textBaseScale_{ 1.0f, 1.0f };
		float pressAmount_ = 0.0f;
	};
}
