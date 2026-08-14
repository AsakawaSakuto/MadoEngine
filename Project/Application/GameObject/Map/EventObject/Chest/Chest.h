#pragma once
#include "../MapEventObjectBase.h"
#include "ChestType.h"
#include <memory>

namespace Player {
	class Base;
}

class Chest : public MapEventObjectBase {
public:
	class OpenCostState {
	public:
		/// @brief Chestの共有開封費用を初期化
		/// @param initialCost 初回の開封費用
		/// @param costIncrease 開封ごとの増加量
		OpenCostState(int initialCost = 10, int costIncrease = 10);

		/// @brief 現在の開封費用を取得
		/// @return 現在の開封費用
		int GetCurrentCost() const { return currentCost_; }

		/// @brief 開封費用を次回分へ増加
		void IncreaseCost();

	private:
		int currentCost_ = 10;
		int costIncrease_ = 10;
	};

	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		ChestType type = ChestType::Normal;
		std::shared_ptr<OpenCostState> openCostState;
		std::string modelName = "Chest";
		std::string colliderName = "ChestAABB";
	};

	~Chest();

	/// @brief 設定付きでChestを初期化
	/// @param desc 初期化に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief Chestを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief Chestに相互作用した時の処理を実行
	/// @param player 相互作用したPlayer
	/// @return ChestをMapから削除するためtrue
	bool Interact(Player::Base& player) override;

	/// @brief 現在のPlayer状態でChestを開封可能か判定
	/// @param player 相互作用するPlayer
	/// @return 開封可能な場合はtrue
	bool CanInteract(const Player::Base& player) const override;

	/// @brief Chestの操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	std::string_view GetInteractionText() const override;

private:
	ChestType type_ = ChestType::Normal;
	std::shared_ptr<OpenCostState> openCostState_;
	mutable std::string interactionText_ = "10Gで宝箱を開ける";
	mutable int displayedOpenCost_ = -1;
	std::string modelName_ = "Chest";
};
