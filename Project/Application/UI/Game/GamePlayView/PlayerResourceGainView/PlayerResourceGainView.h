#pragma once
#include "GameObject/Player/PlayerStatus.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/ObjectHandle.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace MadoEngine {
	class Text;
}

namespace UI::Game {

	/// @brief Playerのリソース獲得量をHUDへ表示するビュー
	class PlayerResourceGainView final {
	public:
		/// @brief 獲得通知用Textプールを初期化
		void Initialize();

		/// @brief リソース獲得量の表示を開始
		/// @param type 獲得したリソースの種類
		/// @param amount 実際に獲得した量
		void Spawn(Player::ResourceGainType type, float amount);

		/// @brief 表示中の獲得通知を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 獲得通知の表示可否を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 獲得通知用Textプールを終了
		void Finalize();

		/// @brief 獲得通知の調整項目をImGuiへ描画
		void DrawImGui();

	private:
		struct GainTextSlot {
			MadoEngine::TextHandle text{};
			Player::ResourceGainType type = Player::ResourceGainType::Health;
			float amount = 0.0f;
			float horizontalOffset = 0.0f;
			float elapsedTime = 0.0f;
			bool isActive = false;
		};

		static constexpr std::size_t kPoolSize = 24;

		/// @brief 短時間内の同種獲得を合算できるTextスロットを取得
		/// @param type 合算対象のリソース種類
		/// @return 合算可能なTextスロット、存在しない場合はnullptr
		GainTextSlot* FindMergeTarget(Player::ResourceGainType type);

		/// @brief 表示に使用できるTextスロットを取得
		/// @return 使用可能なTextスロット、生成済みTextがない場合はnullptr
		GainTextSlot* AcquireSlot();

		/// @brief リソース種類に対応するHUD上の基準座標を取得
		/// @param type リソースの種類
		/// @return 獲得通知の基準座標
		const Vector2& GetBasePosition(Player::ResourceGainType type) const;

		/// @brief リソース種類に対応する文字色を取得
		/// @param type リソースの種類
		/// @return 獲得通知の文字色
		const Vector4& GetTextColor(Player::ResourceGainType type) const;

		/// @brief 生成済みTextへ現在のフォントサイズを反映
		void ApplyFontSize();

		std::array<GainTextSlot, kPoolSize> slots_;
		std::size_t nextSlotIndex_ = 0;
		std::uint64_t spawnSequence_ = 0;
		bool isVisible_ = true;
		float displayLifeTime_ = 0.5f;                           // 獲得通知の表示時間
		float mergeDuration_ = 0.15f;                            // 同種獲得を合算する受付時間
		float fadeStartProgress_ = 0.99f;                        // フェードを開始する寿命の進捗率
		float initialScaleAddition_ = 0.5f;                      // 表示開始時のスケール加算量
		float riseDistance_ = 48.0f;                             // 表示中に上昇するピクセル距離
		float horizontalOffsetMin_ = -2.0f;                      // 通知を分散する最小Xオフセット
		float horizontalOffsetMax_ = 2.0f;                       // 通知を分散する最大Xオフセット
		float fontSize_ = 24.0f;                                 // 獲得通知のフォントサイズ
		Vector2 healthBasePosition_ = { 600.0f, 440.0f };        // HP獲得通知の基準座標
		Vector2 expBasePosition_ = { 600.0f, 440.0f };           // Exp獲得通知の基準座標
		Vector2 moneyBasePosition_ = { 600.0f, 440.0f };         // Money獲得通知の基準座標
		Vector4 healthTextColor_ = { 0.25f, 1.0f, 0.35f, 1.0f }; // HP獲得通知の文字色
		Vector4 expTextColor_ = { 0.2f, 0.9f, 1.0f, 1.0f };      // Exp獲得通知の文字色
		Vector4 moneyTextColor_ = { 1.0f, 0.85f, 0.15f, 1.0f };  // Money獲得通知の文字色
	};

} // namespace UI::Game