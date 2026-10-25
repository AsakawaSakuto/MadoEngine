#pragma once
#include "System/InGameSession/InGamePhase.h"
#include "UI/Game/Chest/ChestOpeningPresentation.h"
#include "UI/Game/Relic/RelicAcquisitionUI.h"
#include "UI/Game/Weapon/WeaponUpgradeUI.h"

namespace System {

	/// @brief 宝箱開封からレリック獲得と武器強化選択までの表示進行の管理
	class RewardSelectionController final {
	public:
		/// @brief 常設HUDの登録後に報酬表示を初期化
		void Initialize();

		/// @brief 報酬表示の参照と演出状態を解放
		void Finalize();

		/// @brief レリック抽選を要求して獲得前の宝箱表示を開始
		/// @param relicSystem 報酬抽選を担当するSystem
		/// @return 報酬要求を受け付けた場合はtrue
		bool RequestRelicReward(Relic::System& relicSystem);

		/// @brief 現在のゲームPhaseに対応する報酬演出と選択入力を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param phase ゲーム全体で確定済みの進行Phase
		/// @param relicSystem レリック獲得の適用先
		/// @param upgradeSystem 武器強化の進行元
		/// @param inventory 武器強化の適用先
		/// @param player リロール料金の支払い元
		void Update(float deltaTime, InGamePhase phase, Relic::System& relicSystem,
			Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory, Player::Base& player);

		/// @brief 死亡確定時に宝箱演出を中断
		void HideChest();

		/// @brief 切り替え後の描画Cameraへ宝箱表示を追従
		/// @param camera 追従対象の描画Camera
		void FollowCamera(const Camera& camera);

		/// @brief 宝箱演出に伴うTPS入力抑止の要否を取得
		/// @return 膨張演出中の場合はtrue
		bool ShouldBlockCameraInput() const;

		/// @brief Player輪郭の描画抑止の要否を取得
		/// @return 宝箱表示または開封フラッシュ中の場合はtrue
		bool ShouldSuppressPlayerOutline() const;

		/// @brief 選択待機と残存演出に応じたBGM停止の要否を取得
		/// @param phase ゲーム全体の進行Phase
		/// @return BGMを一時停止する場合はtrue
		bool ShouldPauseBgm(InGamePhase phase) const;

		/// @brief 選択用BGMの再生区間を識別する値を取得
		/// @param phase ゲーム全体の進行Phase
		/// @param relicSystem レリック獲得待機の読み取り元
		/// @param upgradeSystem 未処理の武器強化回数の読み取り元
		/// @return レリック獲得待機は-1、武器選択は未処理回数、それ以外は0
		int GetSelectionBgmPlaybackId(InGamePhase phase, const Relic::System& relicSystem,
			const Weapon::UpgradeSystem& upgradeSystem) const;

		/// @brief 報酬表示の調整用ImGuiを描画
		/// @param upgradeSystem 武器選択のプレビュー元
		/// @param inventory 武器強化の適用先
		/// @param relicSystem レリック表示のプレビュー元
		void DrawImGui(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory,
			Relic::System& relicSystem);

	private:
		UI::Game::UpgradeUI weaponUpgradeUI_;
		UI::Game::RelicAcquisitionUI relicAcquisitionUI_;
		UI::Game::ChestOpeningPresentation chestOpeningPresentation_;
	};

}
