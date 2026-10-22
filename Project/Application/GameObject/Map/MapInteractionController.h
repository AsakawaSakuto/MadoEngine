#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "EventObject/MapEventObjectBase.h"
#include <array>
#include <memory>
#include <vector>

namespace Player {
	class Base;
}

/// @brief Mapイベントの相互作用と操作案内を管理するクラス
class MapInteractionController final {
public:
	/// @brief 全イベント要求の受付状態を初期化
	void Initialize();

	/// @brief 再生成前に接触対象と通知キューと操作案内をリセット
	void Reset();

	/// @brief 接触対象の選択と相互作用と操作案内を更新
	/// @param player 相互作用するPlayer
	/// @param deltaTime 前フレームからの経過時間
	/// @param eventObjects 更新と削除の対象になるイベントオブジェクト
	void Update(Player::Base& player, float deltaTime, std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects);

	/// @brief 操作案内Textの表示可否を設定
	/// @param isVisible 表示する場合はtrue
	void SetInteractionTextVisible(bool isVisible);

	/// @brief 指定イベント要求の受付可否を設定
	/// @param action 設定対象のイベント要求
	/// @param isEnabled 受付可能にする場合はtrue
	void SetEventActionEnabled(MapEventAction action, bool isEnabled);

	/// @brief 未処理のイベント要求を取得してキューをクリア
	/// @return 発生順に格納されたMapイベント要求
	std::vector<MapEventRequest> ConsumeEventRequests();

	/// @brief 成功した相互作用の結果を取得してキューをクリア
	/// @return 発生順に格納されたインタラクト結果
	std::vector<MapInteractionEvent> ConsumeInteractionEvents();

private:
	/// @brief 接触対象への操作とイベントオブジェクトの削除を処理
	/// @param player 相互作用するPlayer
	/// @param eventObjects 削除の対象になるイベントオブジェクト
	void HandleEventObjectInteraction(Player::Base& player, std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects);

	/// @brief 操作案内Modelを現在の接触対象へ同期
	/// @param deltaTime 前フレームからの経過時間
	void UpdateInteractionMarker(float deltaTime);

	/// @brief 操作案内Textを現在の接触対象へ同期
	/// @param player 相互作用するPlayer
	void UpdateInteractionText(const Player::Base& player);

	/// @brief 指定イベント要求の受付可否を確認
	/// @param action 確認対象のイベント要求
	/// @return 受付可能な場合はtrue
	bool IsEventActionEnabled(MapEventAction action) const;

	std::vector<MapEventRequest> pendingEventRequests_;
	std::vector<MapInteractionEvent> pendingInteractionEvents_;
	std::array<bool, static_cast<std::size_t>(MapEventAction::Count)> eventActionEnabled_{};
	MapEventObjectBase* currentHitEventObject_ = nullptr;
	MadoEngine::ModelHandle interactionMarkerModel_{};
	MadoEngine::TextHandle interactionText_{};
	MadoEngine::SpriteHandle interactionIcon_{};
	Vector3 interactionMarkerStartScale_ = { 0.5f, 0.5f, 0.5f };
	Vector3 interactionMarkerEndScale_   = { 0.75f, 0.75f, 0.75f };
	GameTimer interactionMarkerScaleTimer_;
	bool isInteractionTextVisible_ = true;
};
