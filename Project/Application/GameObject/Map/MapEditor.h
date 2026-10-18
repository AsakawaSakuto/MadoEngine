#pragma once
#include "MapGenerationSettings.h"
#include "MapBlock.h"
#include "EventObject/MapEventObjectBase.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Player {
	class Base;
}

/// @brief Map生成条件の編集と再生成予約を管理するクラス
class MapEditor final {
public:
	/// @brief 生成済みMapを参照する描画中だけ有効なプレビュー情報
	struct PreviewData {
		const MapGenerationSettings& settings;
		const std::vector<std::vector<MapBlock>>& blocks;
		const std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects;
		const Player::Base* player = nullptr;
	};

	/// @brief 保存済み設定と指定シードで編集状態を初期化
	/// @param seed Map生成に使用するシード値
	/// @param appliedSettings 現在生成済みMapの設定
	void Initialize(uint32_t seed, const MapGenerationSettings& appliedSettings);

	/// @brief Map生成EditorのImGuiを描画
	/// @param preview 生成済みMapの読み取り専用情報
	void DrawImGui(const PreviewData& preview);

	/// @brief 現在編集中の生成設定を取得
	/// @return 現在編集中の生成設定
	const MapGenerationSettings& GetSettings() const { return editorSettings_; }

	/// @brief 保留中の再生成設定を取得して予約を解除
	/// @return 予約がない場合は空の値
	std::optional<MapGenerationSettings> ConsumePendingGeneration();

	/// @brief 編集中と生成済みの設定をSnapshotへ変換
	/// @param appliedSettings 現在生成済みMapの設定
	/// @return UndoとRedoに使用する文字列Snapshot
	std::string CaptureState(const MapGenerationSettings& appliedSettings) const;

	/// @brief Snapshotから編集状態と再生成予約を復元
	/// @param snapshot 復元する文字列Snapshot
	/// @param currentAppliedSettings 現在生成済みMapの設定
	void RestoreState(const std::string& snapshot, const MapGenerationSettings& currentAppliedSettings);

	/// @brief 編集中の生成設定をJsonへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveSettings() const;

	/// @brief 保存済み生成設定を読み込み再生成を予約
	/// @return 読み込みに成功した場合はtrue
	bool ReloadSettings();

private:
	/// @brief 地形生成条件と再生成操作を描画
	/// @param appliedSettings 現在生成済みMapの設定
	void DrawMapStatusEditor(const MapGenerationSettings& appliedSettings);

	/// @brief イベントオブジェクトの配置数を描画
	void DrawEventObjectStatusEditor();

	/// @brief 生成済みMapの情報と高さプレビューを描画
	/// @param preview 生成済みMapの読み取り専用情報
	void DrawMapViewEditor(const PreviewData& preview) const;

	/// @brief 生成済みMapの高さと主要Objectを描画
	/// @param preview 生成済みMapの読み取り専用情報
	void DrawHeightPreview(const PreviewData& preview) const;

	MapGenerationSettings editorSettings_;
	std::optional<MapGenerationSettings> pendingGenerationSettings_;
};
