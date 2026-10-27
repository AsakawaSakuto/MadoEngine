#pragma once
#include "GameObject/DropObject/DropObjectStatus.h"
#include "Math/Vector2.h"
#include "Render/Object/ObjectHandle.h"
#include "Utility/Easing/EaseType.h"

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief Editor配置の特殊Drop取得Textの表示演出を管理するビュー
	class DropObjectGetView final {
	public:
		/// @brief DropObjectGetTextへの参照を取得して通常時の表示を抑制
		void Initialize();

		/// @brief 取得した特殊Dropに対応する文字と色で表示演出を開始
		/// @param type 取得した特殊Dropの種類
		void Spawn(DropObject::Type type);

		/// @brief 取得Textの上昇、停止、拡大、縮小を順に更新
		/// @param deltaTime 停止状態を反映したゲーム経過時間（秒）
		void Update(float deltaTime);

		/// @brief 取得Textの表示可否を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief Editor配置を復元してTextへの参照を解放
		void Finalize();

	private:
		/// @brief 名前からTextを再取得してEditor再生成後の基準配置を保持
		/// @return Game所属のText、存在しない場合はnullptr
		MadoEngine::Text* ResolveText();

		/// @brief 表示演出を終了してTextの基準配置を復元
		/// @param text 復元する取得Text
		void Stop(MadoEngine::Text& text);

		MadoEngine::TextHandle text_{};
		Vector2 basePosition_{};
		Vector2 baseScale_{ 1.0f, 1.0f };
		float elapsedTime_ = 0.0f;
		float riseDuration_ = 0.2f;   // 一定距離まで上昇する時間（秒）
		float holdDuration_ = 0.1f;   // 上昇後に座標とScaleを維持する時間（秒）
		float expandDuration_ = 0.15f; // 停止後に最大Scaleまで拡大する時間（秒）
		float shrinkDuration_ = 0.1f; // 最大Scaleから縮小して消える時間（秒）
		float expandedScale_ = 2.0f;  // 基準Scaleに対する拡大時の最大倍率
		float riseDistance_ = 32.0f;  // 基準座標からの上昇距離（ピクセル）
		EaseType riseEaseType_ = EaseType::EaseInQuint; // 上昇終盤に向けて加速する5次のイージング
		bool isActive_ = false;
		bool isVisible_ = true;
	};

}
