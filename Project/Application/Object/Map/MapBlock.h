#pragma once
#include "../IGameObject.h"
#include "MapBlockType.h"

class MapBlock : public IGameObject {
public:
	struct InitializeDesc {
		int x = 0;
		int z = 0;
		int mapWidth = 0;
		uint32_t height = 1;
		MapBlockType type = MapBlockType::Ground;
		SlopeDirection slopeDirection = SlopeDirection::PulsX;
		Vector3 blockSize = { 1.0f, 1.0f, 1.0f };
		bool isModelDraw = true;
	};

	/// @brief MapBlockを設定付きで初期化します。
	/// @param desc 初期化に使用する設定です。
	void Initialize(const InitializeDesc& desc);

	/// @brief MapBlockを更新します。
	/// @param deltaTime 前フレームからの経過時間です。
	void Update(float deltaTime) override;

	/// @brief モデルの表示状態を設定します。
	/// @param isVisible 表示する場合はtrueです。
	void SetVisible(bool isVisible);

	/// @brief デバッグラインを描画します。
	void DrawDebugLine() const;

	/// @brief ブロックの高さを設定します。
	/// @param height 設定する高さです。
	void SetHeight(uint32_t height);

	/// @brief ブロックの高さを取得します。
	/// @return ブロックの高さです。
	uint32_t GetHeight() const;

	/// @brief ブロックの種類を取得します。
	/// @return ブロックの種類です。
	MapBlockType GetType() const;

	/// @brief 坂ブロックの向きを取得します。
	/// @return 坂ブロックの向きです。
	SlopeDirection GetSlopeDirection() const;

private:
	/// @brief 通常ブロック用のCollider形状を作成します。
	/// @param blockSize ブロック共通サイズです。
	void CreateGroundShape(const Vector3& blockSize);

	/// @brief 坂ブロック用のCollider形状を作成します。
	/// @param blockSize ブロック共通サイズです。
	void CreateSlopeShape(const Vector3& blockSize);

	/// @brief 通常ブロック用のモデルを作成します。
	/// @param blockSize ブロック共通サイズです。
	void CreateGroundModel(const Vector3& blockSize);

	/// @brief 坂ブロック用のモデルを作成します。
	/// @param blockSize ブロック共通サイズです。
	void CreateSlopeModel(const Vector3& blockSize);

	/// @brief Collider登録名を作成します。
	/// @return Collider登録名です。
	std::string CreateColliderName() const;

	/// @brief 通常ブロック用のモデル登録名を作成します。
	/// @return モデル登録名です。
	std::string CreateGroundModelName() const;

	/// @brief 坂ブロック用のモデル登録名を作成します。
	/// @return モデル登録名です。
	std::string CreateSlopeModelName() const;

	int x_ = 0;
	int z_ = 0;
	int mapWidth_ = 0;
	uint32_t height_ = 1;
	MapBlockType type_ = MapBlockType::Ground;
	SlopeDirection slopeDirection_ = SlopeDirection::PulsX;
	bool isModelDraw_ = true;
	bool isColliderRegistered_ = false;
	InstancedModel* groundInstancedModel_ = nullptr;
	InstancedModel* slopeInstancedModel_ = nullptr;
	uint32_t groundInstanceHandle_ = UINT32_MAX;
	uint32_t slopeInstanceHandle_ = UINT32_MAX;
};
