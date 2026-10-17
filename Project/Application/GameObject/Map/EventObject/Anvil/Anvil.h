#pragma once
#include "../MapEventObjectBase.h"

/// @brief 無料の武器アップグレードを要求するMapイベントオブジェクト
class Anvil final : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		std::string colliderName = "AnvilAABB";
	};

	/// @brief AnvilのColliderと描画Instanceを解放
	~Anvil() override;

	/// @brief 配置設定に従ってAnvilを初期化
	/// @param desc 配置に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief 指定座標に配置するAnvilのColliderを作成
	/// @param position 配置予定のワールド座標
	/// @return ワールド座標反映済みCollider
	static AABB CreatePlacementCollider(const Vector3& position);

	/// @brief AnvilのColliderとDebug表示を更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief 所持金を消費せずAnvilの利用を確定
	/// @param player 相互作用するPlayer
	/// @return 利用後にMapから削除する場合はtrue
	bool Interact(Player::Base& player) override;

	/// @brief Anvilの操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	std::string_view GetInteractionText() const override;

	/// @brief AnvilのEventObject名を取得
	/// @return 音声リソースとの対応に使用するAnvil名
	std::string_view GetEventObjectName() const override { return "Anvil"; }

	/// @brief 無料の武器アップグレード要求を取得
	/// @return Sceneで処理する武器アップグレード要求
	MapEventRequest GetInteractionRequest() const override;
};
