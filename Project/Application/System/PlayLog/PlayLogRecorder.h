#pragma once
#include "PlayLogData.h"
#include "GameObject/Combat/DamageEvent.h"
#include "GameObject/Map/EventObject/MapInteractionEvent.h"
#include "GameObject/Weapon/WeaponInventoryEvent.h"

namespace System {

	/// @brief GameSceneの寿命に対応するプレイ集計
	class PlayLogRecorder final {
	public:
		/// @brief 新しいプレイの集計を開始
		/// @param seed ゲームシード
		void Initialize(std::uint32_t seed);

		/// @brief 停止時間を除いたプレイ時間と装備時間を加算
		/// @param deltaTime ゲーム進行へ適用した経過秒数
		void AdvancePlayTime(double deltaTime);

		/// @brief 成立した武器の装備変更を記録
		/// @param event 装備変更の結果通知
		void RecordWeaponChange(const Weapon::InventoryChangeEvent& event);

		/// @brief 装備中武器の最終レベルを同期
		/// @param weaponId 武器インスタンスの識別番号
		/// @param level 同期する武器レベル
		void UpdateWeaponLevel(std::uint64_t weaponId, int level);

		/// @brief 実ダメージと撃破を発生源ごとに集計
		/// @param event ダメージ適用結果
		void RecordDamage(const Combat::DamageEvent& event);

		/// @brief 実際に加算された獲得金額を記録
		/// @param amount 倍率適用後の獲得金額
		void RecordMoneyGain(double amount);

		/// @brief 支払いまで成功したリロールを記録
		/// @param paidMoney 実際の支払額
		void RecordReroll(int paidMoney);

		/// @brief 成功したMapインタラクトを記録
		/// @param event インタラクトの結果通知
		void RecordInteraction(const MapInteractionEvent& event);

		/// @brief 終了時の状態とスコアを固定して集計を終了
		/// @param reason プレイの終了理由
		/// @param endingMoney 終了時の所持金
		/// @param endingDifficulty 終了時のKarma由来難易度
		/// @param relics 終了時に所持していたレリック
		/// @return 初回の終了確定に成功した場合はtrue
		bool Finish(PlayEndReason reason, double endingMoney, int endingDifficulty, std::vector<PlayRelicRecord> relics);

		/// @brief 集計を終了しているか確認
		/// @return 終了確定済みの場合はtrue
		bool IsFinished() const { return state_ == State::Finished; }

		/// @brief 現在の集計データを読み取り専用で取得
		/// @return 集計データへのconst参照
		const PlayLogData& GetData() const { return data_; }

	private:
		enum class State {
			NotStarted,
			Recording,
			Finished,
		};

		/// @brief 削除済みを含む武器の履歴を検索
		/// @param weaponId 武器インスタンスの識別番号
		/// @return 対応する履歴、未登録の場合はnullptr
		PlayWeaponRecord* FindWeapon(std::uint64_t weaponId);

		/// @brief 撃破数と探索数と生存時間から最終スコアを計算
		/// @return 生存倍率適用後の小数部を切り捨てたスコア
		std::uint64_t CalculateScore() const;

		PlayLogData data_;
		State state_ = State::NotStarted;
	};

}
