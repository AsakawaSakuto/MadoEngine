#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlock.h"
#include "MapLimit.h"
#include "EventObject/MapEventObjectBase.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace Player {
	class Base;
}

/// @brief Map全体を管理するクラス
class Map {
public:
	/// @brief 指定シードでMapを初期化
	/// @param seed Map生成に使用するシード値
	void Initialize(uint32_t seed);

	/// @brief Mapを更新
	/// @param player 相互作用の対象になるPlayer
	/// @param deltaTime 前フレームからの経過時間
	void Update(Player::Base& player, float deltaTime);

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
	uint32_t GetCurrentSeed() const { return currentSeed_; }

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

private:
	/// @brief Map生成Editorで編集する生成条件
	struct GenerationSettings {
		uint32_t seed = 0;
		int mapWidth = 20;
		int mapHeight = 20;
		int jarSpawnCount = 100;
		int moneyJarSpawnCount = 50;
		int expJarSpawnCount = 50;
		int chestSpawnCount = 50;
		int normalChestSpawnCount = 25;
		int freeChestSpawnCount = 25;
		int karmaSpawnCount = 50;
		Vector3 blockSize = { 15.0f, 7.5f, 15.0f };
		int minHeight = 1;
		int maxHeight = 10;
		int minStartHeight = 1;
		int maxStartHeight = 2;
		int minRangeHeight = -1;
		int maxRangeHeight = 1;
		float slopeSpawnRate = 1.0f;
	};

	/// @brief 指定設定でMapを生成
	/// @param settings 生成に使用する設定
	void Generate(const GenerationSettings& settings);

	/// @brief 現在生成済みMapから生成設定を作成
	/// @return 現在生成済みMapの生成設定
	GenerationSettings CreateAppliedSettings() const;

	/// @brief 生成設定を有効範囲に補正
	/// @param settings 補正対象の生成設定
	static void ClampGenerationSettings(GenerationSettings& settings);

	/// @brief 二つの生成設定が一致するか判定
	/// @param lhs 比較する左辺設定
	/// @param rhs 比較する右辺設定
	/// @return 全設定が一致する場合はtrue
	static bool AreGenerationSettingsEqual(const GenerationSettings& lhs, const GenerationSettings& rhs);

	/// @brief 生成設定をJsonへ変換
	/// @param settings 変換する生成設定
	/// @return 生成設定を格納したJson
	static nlohmann::json GenerationSettingsToJson(const GenerationSettings& settings);

	/// @brief Jsonから生成設定を復元
	/// @param json 読み込み元Json
	/// @param outSettings 復元結果を受け取る生成設定
	/// @return 必須項目を読み込めた場合はtrue
	static bool GenerationSettingsFromJson(const nlohmann::json& json, GenerationSettings& outSettings);

	/// @brief 保存済みEditor設定を読み込み
	/// @param outSettings 読み込み結果を受け取る生成設定
	/// @param useSavedSeed 保存済みシード値も適用する場合はtrue
	/// @return 読み込みに成功した場合はtrue
	bool LoadEditorSettings(GenerationSettings& outSettings, bool useSavedSeed) const;

	/// @brief 生成済みMapの高さと主要Objectプレビューを描画
	/// @param player プレビューへ表示するPlayer
	void DrawHeightPreview(const Player::Base* player) const;

	/// @brief 配置候補Colliderが既存イベントオブジェクトと重なるか判定
	/// @param collider 配置候補のワールド座標反映済みCollider
	/// @return 既存Colliderと重なる場合はtrue
	bool IsEventObjectColliderOverlapping(const AABB& collider) const;

	/// @brief Player初期配置Colliderが配置禁止Objectと重なるか判定
	/// @param collider Player初期配置時の移動Collider
	/// @return 配置禁止Objectと重なる場合はtrue
	bool IsPlayerSpawnBlocked(const Sphere& collider) const;

	/// @brief Map上にJarをランダム配置
	void GenerateJars();

	/// @brief Map上にChestをランダム配置
	void GenerateChests();

	/// @brief Map上にKarmaをランダム配置
	void GenerateKarmas();

	/// @brief 通常ブロック上にBossSpawnerを配置
	void GenerateBossSpawner();

	/// @brief Map上のイベントオブジェクトを更新
	/// @param player 相互作用するPlayer
	/// @param deltaTime 前フレームからの経過時間
	void UpdateEventObjects(Player::Base& player, float deltaTime);

	/// @brief Playerとイベントオブジェクトの相互作用を処理
	/// @param player 相互作用するPlayer
	void HandleEventObjectInteraction(Player::Base& player);

	/// @brief 操作案内Modelの表示と配置を現在の接触対象へ同期
	/// @param deltaTime 前フレームからの経過時間
	void UpdateInteractionMarker(float deltaTime);

	/// @brief 操作案内Textの表示内容を現在の接触対象へ同期
	/// @param player 相互作用するPlayer
	void UpdateInteractionText(const Player::Base& player);

	/// @brief 地形生成用の高さ設定を有効範囲に補正
	void ClampHeightSettings();

	/// @brief 指定座標のブロック高さを取得
	/// @param x Map上のX座標
	/// @param z Map上のZ座標
	/// @return ブロックの高さ
	uint32_t GetBlockHeight(int x, int z) const;

	std::vector<std::vector<MapBlock>> mapBlocks_;
	std::vector<std::unique_ptr<MapEventObjectBase>> eventObjects_;
	std::vector<MapEventRequest> pendingEventRequests_;
	MapEventObjectBase* currentHitEventObject_ = nullptr;

	MadoEngine::ModelHandle interactionMarkerModel_{};
	MadoEngine::TextHandle interactionText_{};
	Vector3 interactionMarkerStartScale_ = { 0.35f, 0.35f, 0.35f };
	Vector3 interactionMarkerEndScale_ = { 0.5f, 0.5f, 0.5f };
	GameTimer interactionMarkerScaleTimer_;

	Random terrainRandom_;
	Random eventObjectRandom_;
	GenerationSettings editorSettings_;
	std::optional<GenerationSettings> pendingGenerationSettings_;
	uint32_t currentSeed_ = 0;
	
	int mapWidth_ = 20;
	int mapHeight_ = 20;
	int jarSpawnCount_ = 100;
	int moneyJarSpawnCount_ = 50;
	int expJarSpawnCount_ = 50;
	int chestSpawnCount_ = 50;
	int normalChestSpawnCount_ = 25;
	int freeChestSpawnCount_ = 25;
	int karmaSpawnCount_ = 50;

	bool isModelDraw_ = true;

	Vector3 blockSize_ = { 15.0f, 7.5f, 15.0f };

	int minHeight_ = 1;
	int maxHeight_ = 10;

	int minStartHeight_ = 1;
	int maxStartHeight_ = 2;

	int minRangeHeight_ = -1;
	int maxRangeHeight_ = 1;

	float slopeSpawnRate_ = 1.0f;
};
