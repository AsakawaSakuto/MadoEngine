#pragma once
#include "Render/Object/ObjectHandle.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <array>
#include <cstddef>
#include <cstdint>

class Camera;

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief Projectileによるダメージ数値をワールド座標へ追従表示するビュー
	class ProjectileDamageView {
	public:
		/// @brief ダメージ表示用Textプールを初期化する
		void Initialize();

		/// @brief ダメージ数値の表示を開始する
		/// @param damage 実際に適用されたダメージ量
		/// @param worldPosition ダメージを受けた対象のワールド座標
		void Spawn(float damage, const Vector3& worldPosition);

		/// @brief 表示中のダメージ数値を更新する
		/// @param deltaTime 前フレームからの経過時間
		/// @param camera ワールド座標の投影に使用するカメラ
		void Update(float deltaTime, const Camera& camera);

		/// @brief ダメージ表示用Textプールを終了する
		void Finalize();

		/// @brief ダメージ表示設定をImGuiへ描画する
		void DrawImGui();

	private:
		struct DamageTextSlot {
			MadoEngine::TextHandle text{};
			Vector3 worldPosition = { 0.0f, 0.0f, 0.0f };
			float horizontalOffset = 0.0f;
			float verticalOffset = 0.0f;
			float elapsedTime = 0.0f;
			bool isActive = false;
		};

		static constexpr std::size_t kPoolSize = 96;

		/// @brief 表示に使用できるTextスロットを取得する
		/// @return 使用可能なTextスロット。生成済みTextがない場合はnullptr
		DamageTextSlot* AcquireSlot();

		/// @brief ワールド座標を基準画面上の座標へ変換する
		/// @param worldPosition 変換するワールド座標
		/// @param camera 投影に使用するカメラ
		/// @param outScreenPosition 変換後の画面座標
		/// @return 画面内へ投影できた場合はtrue
		bool WorldToScreen(
			const Vector3& worldPosition,
			const Camera& camera,
			Vector2& outScreenPosition) const;

		std::array<DamageTextSlot, kPoolSize> slots_;
		std::size_t nextSlotIndex_ = 0;
		std::uint64_t spawnSequence_ = 0;
		float displayLifeTime_ = 0.5f;         // ダメージ表示の寿命（秒）
		float fadeStartProgress_ = 0.5f;       // ダメージ表示のフェード開始タイミング
		float scaleSettleProgress_ = 1.0f;     // ダメージ表示のスケールが落ち着くタイミング
		float initialScaleAddition_ = 1.0f;    // ダメージ表示の初期スケール増加量
		float riseDistance_ = 64.0f;           // ダメージ表示の上昇距離（ピクセル）
		float enemyHeadOffsetMin_ = 1.0f;      // Enemy座標から頭上への最小オフセット
		float enemyHeadOffsetMax_ = 2.0f;      // Enemy座標から頭上への最大オフセット
		float horizontalOffsetMin_ = -24.0f;   // ダメージ表示の最小左右オフセット
		float horizontalOffsetMax_ = 24.0f;    // ダメージ表示の最大左右オフセット
		float fontSize_ = 32.0f;               // ダメージ表示のフォントサイズ
		Vector4 damageTextColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // ダメージ表示の文字色
	};

} // namespace UI::Game
