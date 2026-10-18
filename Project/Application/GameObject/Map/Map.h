#pragma once
#include "MapBlock.h"
#include "MapLimit.h"
#include "MapEditor.h"
#include "MapGenerationSettings.h"
#include "MapInteractionController.h"
#include "Utility/Random.h"
#include <memory>
#include <string>
#include <vector>

namespace Player {
	class Base;
}

/// @brief Mapの所有と生成と更新を取りまとめるクラス
class Map {
public:
	/// @brief 指定シードでMapを初期化
	/// @param seed Map生成に使用するシード値
	void Initialize(uint32_t seed);

	/// @brief Mapを更新
	/// @param player 相互作用の対象になるPlayer
	/// @param deltaTime 前フレームからの経過時間
	void Update(Player::Base& player, float deltaTime);

	/// @brief 操作案内Textの表示可否を設定
	/// @param isVisible 表示する場合はtrue
	void SetInteractionTextVisible(bool isVisible);

	/// @brief 指定したMapイベント要求の受付可否を設定
	/// @param action 設定対象のイベント要求
	/// @param isEnabled 受付可能にする場合はtrue
	void SetEventActionEnabled(MapEventAction action, bool isEnabled);

	/// @brief Map調整用のImGuiを描画
	/// @param player プレビューへ表示するPlayer
	void DrawImGui(const Player::Base* player);

	/// @brief 保留中のEditor再生成要求を適用
	/// @return Mapを再生成した場合はtrue
	bool ApplyPendingEditorGeneration();

	/// @brief Map生成Editorの現在状態を文字列Snapshotへ変換
	/// @return UndoとRedoに使用する文字列Snapshot
	std::string CaptureEditorState() const;

	/// @brief 文字列SnapshotからMap生成Editorの状態を復元
	/// @param snapshot 復元する文字列Snapshot
	void RestoreEditorState(const std::string& snapshot);

	/// @brief Map生成Editorの設定をJsonへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveEditorSettings() const;

	/// @brief Map生成Editorの設定をJsonから再読込
	/// @return 読み込みに成功した場合はtrue
	bool ReloadEditorSettings();

	/// @brief 現在生成済みMapのシード値を取得
	/// @return 現在生成済みMapのシード値
	uint32_t GetCurrentSeed() const { return appliedSettings_.seed; }

	/// @brief 現在生成済みMapに対応する移動制限を取得
	/// @return Map外周と高さを表す移動制限
	MapLimit CreateMapLimit() const;

	/// @brief Playerを配置する通常Block上面の中心座標を生成
	/// @param seed 配置Blockの選択に使用するシード値
	/// @return Playerを配置する地表座標
	Vector3 CreatePlayerSpawnGroundPosition(uint32_t seed) const;

	/// @brief 未処理のMapイベント要求を取得してキューをクリア
	/// @return 発生順に格納されたMapイベント要求
	std::vector<MapEventRequest> ConsumeEventRequests();

	/// @brief 成功したインタラクトの結果を取得してキューをクリア
	/// @return 発生順に格納されたインタラクト結果
	std::vector<MapInteractionEvent> ConsumeInteractionEvents();

private:
	/// @brief 指定設定で地形とイベントオブジェクトを生成
	/// @param settings 生成に使用する設定
	void Generate(const MapGenerationSettings& settings);

	/// @brief Player初期配置Colliderと配置禁止Objectの重複を判定
	/// @param collider Player初期配置時の移動Collider
	/// @return 配置禁止Objectと重なる場合はtrue
	bool IsPlayerSpawnBlocked(const Sphere& collider) const;

	/// @brief 指定座標のブロック高さを取得
	/// @param x Map上のX座標
	/// @param z Map上のZ座標
	/// @return ブロックの高さ
	uint32_t GetBlockHeight(int x, int z) const;

	std::vector<std::vector<MapBlock>> mapBlocks_;
	std::vector<std::unique_ptr<MapEventObjectBase>> eventObjects_;
	MapGenerationSettings appliedSettings_;
	MapEditor editor_;
	MapInteractionController interactionController_;
	Random terrainRandom_;
	bool isModelDraw_ = true;
};
