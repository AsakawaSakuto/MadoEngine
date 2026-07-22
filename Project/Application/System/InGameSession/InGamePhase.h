#pragma once

/// @brief ゲーム中の進行状態を表す列挙型
enum class InGamePhase {
	Starting,       // ゲーム開始中
	Playing,        // ゲームプレイ中
	Paused,         // ゲーム一時停止中
	WaitingUpgrade, // 武器アップグレード選択中
	WaitingGetItem, // アイテム取得中
	GameOver,       // ゲームオーバー
};
