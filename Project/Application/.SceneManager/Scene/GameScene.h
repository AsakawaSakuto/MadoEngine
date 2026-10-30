#pragma once
#include ".SceneManager/CommonData.h"
#include ".SceneManager/IScene.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Map/Map.h"
#include "GameObject/Map/MapLimit.h"
#include "GameObject/Enemy/EnemyEditor.h"
#include "GameObject/Enemy/EnemyManager.h"
#include "GameObject/Enemy/EnemySpawner.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponStatusEditor.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include "GameObject/Weapon/Projectile/ProjectileStatus.h"
#include "GameObject/Relic/RelicSystem.h"
#include "GameObject/Relic/RelicEditor.h"
#include "Utility/Light/LightManager.h"
#include "Utility/Random.h"
#include "UI/Game/GameHud.h"
#include "System/GameCamera/GameCameraController.h"
#include "System/InGameSession/InGameSession.h"
#include "System/PlayLog/PlayLogRecorder.h"
#include "System/RewardSelection/RewardSelectionController.h"
#include <cstdint>

/// @brief ゲームシーン
/// @details ゲーム本編の更新順序、システム間の通知、Scene遷移の管理
class GameScene : public IScene {
public:
	/// @brief コンストラクタ
	/// @param commonData Sceneをまたいで保持するApplication共通データ
	explicit GameScene(CommonData& commonData);

	/// @brief デストラクタ
	~GameScene() override;

	/// @brief 初期化処理
	void Initialize() override;

	/// @brief 更新処理
	/// @param dt デルタタイム
	/// @return 次に遷移するシーンの種類
	SceneType Update(float dt) override;

	/// @brief PauseメニューによるGameSceneの再生成要求を取得
	/// @return GameSceneを最初から開始する場合はtrue
	bool IsSceneRestartRequested() const override { return isSceneRestartRequested_; }

	/// @brief 武器選択とレリック獲得演出に応じたBGM一時停止の要否を取得
	/// @return 武器選択またはレリック獲得演出中の場合はtrue
	bool ShouldPauseBgm() const override;

	/// @brief 武器選択とレリック獲得用BGMの再生区間を識別する値を取得
	/// @return 武器選択では未処理回数、開封後のレリック獲得待機では-1、それ以外は0
	int GetSelectionBgmPlaybackId() const override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief Scene終了要求に対応するプレイ記録を確定
	/// @param destination 遷移先Scene、アプリ終了の場合はSceneType::None
	void OnExitRequested(SceneType destination) override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief ImGui描画処理
	void DrawImGui() override;

	/// @brief Map生成Editorを提供しているか確認
	/// @return Map生成Editorを提供している場合はtrue
	bool HasMapGeneratorEditor() const override { return map_ != nullptr; }

	/// @brief Map生成EditorのImGuiを描画
	void DrawMapGeneratorImGui() override;

	/// @brief Map生成Editor状態を文字列Snapshotへ変換
	/// @return UndoとRedoに使用する文字列Snapshot
	std::string CaptureMapGeneratorEditorState() const override;

	/// @brief 文字列SnapshotからMap生成Editor状態を復元
	/// @param snapshot 復元する文字列Snapshot
	void RestoreMapGeneratorEditorState(const std::string& snapshot) override;

	/// @brief Game Scene固有のEditor Documentを保存
	/// @return 保存に成功した場合はtrue
	bool SaveEditorDocuments() const override;

	/// @brief Game Scene固有のEditor Documentを再読込
	/// @return 読み込みに成功した場合はtrue
	bool ReloadEditorDocuments() override;

	/// @brief Frame末尾に保留中のMap再生成を適用
	/// @return Mapを再生成した場合はtrue
	bool ApplyPendingEditorOperations() override;

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得
	/// @return Playerのワールド座標
	Vector3 GetShadowFocusPosition() const override;

	/// @brief シャドウマップ確認用のPlayer描画座標を取得
	/// @param outPosition PlayerのModelワールド座標を受け取る変数
	/// @return Player座標を取得できた場合はtrue
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const override;
private:
	/// @brief 移動と衝突解決からMap操作と攻撃までのゲーム進行を更新
	/// @param deltaTime 停止状態を反映したゲーム経過時間
	void UpdateGameplay(float deltaTime);

	/// @brief 指定時刻への到達に応じたEnemy大量出現演出を更新
	/// @param previousElapsedTime 前Frameまでのゲーム経過時間（秒）
	/// @param elapsedTime 現在Frameまでのゲーム経過時間（秒）
	void UpdateMassiveEnemySpawnPresentation(float previousElapsedTime, float elapsedTime);

	/// @brief Playerの被ダメージ通知を取得して効果音とCameraとHUDへ反映
	void HandlePlayerDamage();

	/// @brief Enemyダメージ通知を効果音とHUDとプレイ記録へ配送
	void DispatchDamageEvents();

	/// @brief Mapのイベント要求を対応するゲームSystemへ配送
	void DispatchMapEventRequests();

	/// @brief 確定済みのゲーム状態と武器攻撃通知をHUDへ同期
	/// @param dt 停止中も進行する実時間
	/// @param deltaTime 停止状態を反映したゲーム経過時間
	void UpdateHud(float dt, float deltaTime);

	/// @brief 成功したゲーム操作の通知をプレイ集計へ配送
	void ConsumePlayLogEvents();

	/// @brief 終了時の装備と所持品をSnapshotへ固定
	/// @param reason プレイの終了理由
	void FinishPlayLog(System::PlayEndReason reason);

	/// @brief 現在Mapに依存する移動制限を各Game Systemへ同期
	void SynchronizeMapDependentState();

	/// @brief Playerの位置と向きと残存イベントObjectをミニマップへ同期
	void UpdateMiniMap();

	/// @brief 所持レリックの変更をPlayerとWeaponへ反映
	void ApplyRelicModifiersIfChanged();

	/// @brief PlayerPointLightをPlayerの頭上座標へ同期
	void SynchronizePlayerPointLight();

	/// @brief Chest演出に応じたPlayerOutlineの有効状態の切り替え
	/// @param isSuppressed PlayerOutlineを無効化する場合はtrue
	void SetPlayerOutlineSuppressed(bool isSuppressed);

	/// @brief PlayerのHP割合に応じて低体力Vignetteの強度を更新
	void UpdateLowHealthVignette();

	/// @brief 名前で特定したフルスクリーンVignetteの強度を反映
	/// @param intensity 低体力Vignetteの強度（0から1）
	void SetLowHealthVignetteIntensity(float intensity);

	CommonData& commonData_;
	std::uint32_t gameSeed_ = 0;
	
	LightHandle playerPointLightHandle_{};
	System::GameCameraController gameCameraController_;

	ColliderShape mapLimitBox_;
	Vector3 mapLimitBoxPos_;

	std::unique_ptr<Player::Base> player_;

	std::unique_ptr<Map> map_;

	std::unique_ptr<Enemy::Manager> enemyManager_;
	std::unique_ptr<Enemy::Spawner> enemySpawner_;
	std::unique_ptr<Enemy::Editor> enemyEditor_;

	std::unique_ptr<Weapon::Inventory> weaponInventory_;
	std::unique_ptr<Weapon::StatusEditor> weaponStatusEditor_;
	std::unique_ptr<Weapon::UpgradeSystem> weaponUpgradeSystem_;
	std::unique_ptr<Relic::System> relicSystem_;
	Relic::Editor relicEditor_;
	
	UI::Game::GameHud hud_;
	System::RewardSelectionController rewardSelectionController_;
	std::uint64_t appliedRelicRevision_ = 0;

	const int weaponSlotCount_ = 6;

	// System
	std::unique_ptr<System::InGameSession> inGameSession_;
	System::PlayLogRecorder playLogRecorder_;
	bool playLogSaveAttempted_ = false;
	bool isSceneRestartRequested_ = false;
};
