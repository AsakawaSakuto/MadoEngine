#pragma once
#include "GameObject/Player/PlayerStatus.h"
#include "Control/InputControlFeedback.h"
#include "GamePlayView/DropObjectGetView/DropObjectGetView.h"
#include "GamePlayView/EnemyDamageView/EnemyDamageView.h"
#include "GamePlayView/EnemyStatusEffectIconView/EnemyStatusEffectIconView.h"
#include "GamePlayView/FpsMeasurementView/FpsMeasurementView.h"
#include "GamePlayView/GamePlayTimerView/GamePlayTimerView.h"
#include "GamePlayView/PlayerResourceGainView/PlayerResourceGainView.h"
#include "GamePlayView/PlayerDamageView/PlayerDamageView.h"
#include "MiniMap/MiniMapUI.h"
#include "Player/PlayerExpGauge.h"
#include "Player/PlayerHealthGauge.h"
#include "Player/PlayerIconUI.h"
#include "Player/PlayerStaminaGauge.h"
#include "Relic/RelicInventoryUI.h"
#include "Utility/Color/GamingColor.h"
#include "Utility/GameTimer/GameTimer.h"
#include "Weapon/WeaponIconUI.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Map;

namespace UI::Game {

	/// @brief ゲーム本体からHUDへ渡すフレーム単位の表示情報
	struct GameHudState {
		Player::Status playerStatus;
		Vector3 playerPosition{};
		float wallClimbRemainingTime = 0.0f;
		float wallClimbMaxDuration = 0.0f;
		float remainingTime = 0.0f;
		float gameStartProgress = 0.0f;
		float gamePlayElapsedTime = 0.0f;
		std::uint64_t killCount = 0;
		std::size_t enemyCount = 0;
		int upgradeRerollCost = 0;
		bool isWallClimbGaugeVisible = false;
		bool isWaitingUpgradeSelection = false;
		bool isPaused = false;
		std::size_t pauseMenuSelectedIndex = 0;
	};

	/// @brief ゲーム中の常設HUDとイベント通知表示の所有と更新
	class GameHud final {
	public:
		/// @brief 報酬選択UIより先に常設HUDを初期化
		/// @param weaponSlotCount 表示する武器スロット数
		/// @param relicSystem 所持レリックの初期表示元
		/// @param miniMapData 生成済みMapの表示情報
		/// @param gameSeed 表示するゲームSeed
		void Initialize(int weaponSlotCount, const Relic::System& relicSystem,
			const MapMiniMapData& miniMapData, std::uint32_t gameSeed);

		/// @brief HUDの参照と所有する表示リソースを解放
		void Finalize();

		/// @brief 開始演出を除くHUD全体の表示状態を設定
		/// @param isVisible HUDを表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief HUD全体の表示状態を取得
		/// @return HUDを表示する場合はtrue
		bool IsVisible() const { return isVisible_; }

		/// @brief 確定済みのゲーム状態をゲージと文字表示へ同期
		/// @param deltaTime 停止中も進行する実時間
		/// @param gameDeltaTime 停止状態を反映したゲーム経過時間
		/// @param state 現在フレームの表示情報
		/// @param inventory 武器表示の読み取り元
		/// @param camera HP表示の投影に使用するCamera
		void Update(float deltaTime, float gameDeltaTime, const GameHudState& state,
			const Weapon::Inventory& inventory, const Camera& camera);

		/// @brief Camera切り替え後の姿勢でワールド追従表示を更新
		/// @param gameDeltaTime 停止状態を反映したゲーム経過時間
		/// @param isPlaying 通常プレイ中の場合はtrue
		/// @param enemyManager 状態異常表示の読み取り元
		/// @param camera ワールド座標の投影に使用するCamera
		void UpdateWorldViews(float gameDeltaTime, bool isPlaying,
			const Enemy::Manager& enemyManager, const Camera& camera);

		/// @brief 所持レリック表示を確定済みの所持品へ同期
		/// @param relicSystem 所持レリックの読み取り元
		void UpdateRelicInventory(const Relic::System& relicSystem);

		/// @brief GPU待機済みの時点でミニマップの地形表示を再構築
		/// @param data 生成済みMapの表示情報
		void RebuildMiniMap(const MapMiniMapData& data);

		/// @brief 残存イベントObjectとPlayerの現在地をミニマップへ同期
		/// @param map イベントObject座標の読み取り元
		/// @param playerPosition Playerのワールド座標
		/// @param playerFacingYaw Playerの水平Yaw角度
		void UpdateMiniMap(const Map& map, const Vector3& playerPosition, float playerFacingYaw);

		/// @brief 表示するMapのSeedを設定
		/// @param gameSeed 表示するゲームSeed
		void SetSeed(std::uint32_t gameSeed);

		/// @brief Enemyダメージの通知表示を開始
		/// @param event 表示するダメージ通知
		void ShowDamage(const Combat::DamageEvent& event);

		/// @brief Playerの被ダメージ量をマイナス記号付きで表示
		/// @param damage 実際に減少したHP量
		void ShowPlayerDamage(float damage);

		/// @brief Playerのリソース獲得通知を表示
		/// @param event 表示するリソース獲得通知
		void ShowResourceGain(const Player::ResourceGainEvent& event);

		/// @brief 特殊Dropの取得内容を表示
		/// @param type 取得した特殊Dropの種類
		void ShowDropObjectGet(DropObject::Type type);

		/// @brief Enemy大量出現Textのフェードインと待機および退場演出を開始
		void ShowMassiveEnemySpawn();

		/// @brief 攻撃した武器スロットの表示演出を開始
		/// @param slotIndex 攻撃した武器のスロット番号
		void PlayWeaponAttackAnimation(std::size_t slotIndex);

		/// @brief HUDの調整用ImGuiを描画
		void DrawImGui();

	private:
		/// @brief PauseGroupの表示と子Group単位の選択拡大を同期
		/// @param isPaused ゲームが一時停止中の場合はtrue
		/// @param selectedIndex 選択中の項目番号、Continueから順に0から2
		void UpdatePausePresentation(bool isPaused, std::size_t selectedIndex = 0);

		/// @brief 常設HUDのGroupとTextへ表示状態を反映
		/// @param isVisible HUDを表示する場合はtrue
		void ApplyStaticVisibility(bool isVisible);

		/// @brief Upgrade Groupの表示状態と操作案内および見出しの演出を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param isVisible 表示する場合はtrue
		void UpdateUpgradePresentation(float deltaTime, bool isVisible);

		/// @brief Upgrade内の決定とリロールの押下状態へ操作案内の拡縮演出を同期
		/// @param deltaTime 停止中も進行する実時間
		/// @param upgrade Upgrade Groupのハンドル
		void UpdateUpgradeControlFeedback(float deltaTime, MadoEngine::UIGroupHandle upgrade);

		/// @brief 開始待機とPlayingの経過時間に合わせて帯と開始Textの演出を更新
		/// @param progress 開始待機の進行率、0.0fから1.0f
		/// @param gamePlayElapsedTime Playing開始後のゲーム経過時間、秒単位
		void UpdateGameStartPresentation(float progress, float gamePlayElapsedTime);

		/// @brief Enemy大量出現Textの透明度と退場位置を更新
		/// @param deltaTime 停止中も進行する実時間（秒）
		void UpdateMassiveEnemySpawnPresentation(float deltaTime);

		/// @brief HP・所持金・撃破数・Enemy数・FPS・Seed・リロール費用の文字表示を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param state 現在フレームの表示情報
		void UpdateTexts(float deltaTime, const GameHudState& state);

		// プレイヤーの状態と装備の表示
		PlayerExpGauge expGauge_;           // 経験値ゲージと強化待機中の表示
		PlayerHealthGauge healthGauge_;     // 画面固定とプレイヤー頭上のHPゲージ
		PlayerStaminaGauge staminaGauge_;   // 壁登り可能時間のゲージ
		PlayerIconUI playerIconUI_;         // プレイヤーのアイコンと背景および枠
		WeaponIconUI weaponIconUI_;         // 武器スロットのアイコン、レベル、発射ゲージ
		RelicInventoryUI relicInventoryUI_; // 所持レリックのアイコンと個数

		// 計測表示とミニマップ
		FpsMeasurementView fpsMeasurementView_;            // FPSの計測とデバッグ欄の表示
		GamePlayTimerView gamePlayTimerView_;              // 残り時間と超過時の通知
		MiniMapUI miniMapUI_;                              // マップとプレイヤーおよびオブジェクトの位置表示
		MapMiniMapObjectPositions miniMapObjectPositions_; // ミニマップ更新で再利用するオブジェクトの座標バッファ

		// 戦闘と取得の通知
		PlayerResourceGainView playerResourceGainView_;       // プレイヤーが得たリソースの増加量
		PlayerDamageView playerDamageView_;                   // プレイヤーが受けたダメージの表示演出
		DropObjectGetView dropObjectGetView_;                 // 特殊ドロップ取得時の通知演出
		EnemyDamageView enemyDamageView_;                     // 敵の位置に追従するダメージ表示
		EnemyStatusEffectIconView enemyStatusEffectIconView_; // 敵の頭上に追従する状態異常アイコン

		// 強化選択画面の演出
		MadoEngine::TextHandle upgradeText_{};         // 拡大演出を適用中の強化見出し
		GamingColor upgradeTextColor_;                 // 強化見出しの色相の循環状態
		GameTimer upgradeTextPulseTimer_;              // 強化見出しの拡大演出の進行
		Vector2 upgradeTextBaseScale_{ 1.0f, 1.0f };   // 強化見出しの拡大倍率を掛ける前の編集用Scale
		InputControlFeedback upgradeDecisionFeedback_; // 決定ボタンの押下と復元の演出
		InputControlFeedback upgradeRerollFeedback_;   // リロールボタンの押下と復元の演出

		// 敵の大量出現通知
		GameTimer massiveEnemySpawnTimer_;            // フェードインから退場までの進行
		bool isMassiveEnemySpawnJustStarted_ = false; // 通知開始フレームのタイマー加算を抑える場合はtrue

		// HUDの共通状態
		std::uint32_t gameSeed_ = 0;       // デバッグ欄に表示する現在のゲームシード
		bool isVisible_ = true;            // HUDの表示を許可する場合はtrue
		bool isWorldViewsVisible_ = false; // プレイ進行中の通知表示を許可する場合はtrue
	};

}
