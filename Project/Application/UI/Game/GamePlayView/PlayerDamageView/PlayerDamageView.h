#pragma once
#include "Math/Vector2.h"
#include "Render/Object/ObjectHandle.h"

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief Editor配置のPlayer被ダメージTextの表示演出を管理するビュー
	class PlayerDamageView final {
	public:
		/// @brief PlayerDamageTextへの参照を取得して通常時の表示を抑制
		void Initialize();

		/// @brief 被ダメージ量の表示演出を開始
		/// @param damage 実際に減少したHP量
		void Spawn(float damage);

		/// @brief 被ダメージTextの上昇と拡縮を更新
		/// @param deltaTime ゲーム停止中も進行する実時間（秒）
		void Update(float deltaTime);

		/// @brief 被ダメージTextの表示可否を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief Editor配置を復元してTextへの参照を解放
		void Finalize();

	private:
		/// @brief 名前からTextを再取得してEditor再生成後の基準配置を保持
		/// @return Game所属のText、存在しない場合はnullptr
		MadoEngine::Text* ResolveText();

		/// @brief 表示演出を終了してTextの基準配置を復元
		/// @param text 復元する被ダメージText
		void Stop(MadoEngine::Text& text);

		MadoEngine::TextHandle text_{};
		Vector2 basePosition_{};
		Vector2 baseScale_{ 1.0f, 1.0f };
		float elapsedTime_ = 0.0f;
		float displayLifeTime_ = 0.3f;      // 上昇と初期拡大の収束に使用する時間（秒）
		float shrinkDuration_ = 0.1f;       // 上昇後に縮小して消える時間（秒）
		float initialScaleAddition_ = 0.5f; // 表示開始時のScale加算量
		float riseDistance_ = 48.0f;        // 基準座標からの上昇距離（ピクセル）
		bool isActive_ = false;
		bool isVisible_ = true;
	};

}
