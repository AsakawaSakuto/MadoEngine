#pragma once
#include "../MapEventObjectBase.h"

/// @brief 配置中のExpすべての吸着を要求するMapイベントオブジェクト
class Magnet : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		std::string colliderName = "MagnetAABB";
	};

	/// @brief MagnetのColliderと描画Instanceを解放
	~Magnet() override;

	/// @brief 設定付きでMagnetを初期化
	/// @param desc 配置に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief 指定座標に配置するMagnetのColliderを作成
	/// @param position 配置予定のワールド座標
	/// @return ワールド座標反映済みCollider
	static AABB CreatePlacementCollider(const Vector3& position);

	/// @brief MagnetのColliderとDebug表示を更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief Magnet取得を確定
	/// @param player 相互作用するPlayer
	/// @return 取得後にMapから削除するためtrue
	bool Interact(Player::Base& player) override;

	/// @brief Magnetの操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	std::string_view GetInteractionText() const override;

	/// @brief MagnetのEventObject名を取得
	/// @return 音声リソースとの対応に使用するMagnet名
	std::string_view GetEventObjectName() const override { return "Magnet"; }

	/// @brief 配置中のExpすべての吸着要求を取得
	/// @return Sceneで処理するExp吸着要求
	MapEventRequest GetInteractionRequest() const override;
};
