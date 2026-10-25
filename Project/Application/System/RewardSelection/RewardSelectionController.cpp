#include "RewardSelectionController.h"
#include "GameObject/Relic/RelicSystem.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"

namespace {
	constexpr int kRelicAcquisitionBgmPlaybackId = -1;
}

namespace System {

	void RewardSelectionController::Initialize() {
		weaponUpgradeUI_.Initialize();
		relicAcquisitionUI_.Initialize();
		chestOpeningPresentation_.Initialize();
	}

	void RewardSelectionController::Finalize() {
		chestOpeningPresentation_.Finalize();
		relicAcquisitionUI_.Finalize();
		weaponUpgradeUI_.Finalize();
	}

	bool RewardSelectionController::RequestRelicReward(Relic::System& relicSystem) {
		if (!relicSystem.RequestReward()) {
			return false;
		}

		// 抽選結果の確定後に宝箱だけを表示し、獲得操作の開始を開封完了まで保留
		if (const Relic::Definition* reward = relicSystem.GetPendingReward()) {
			chestOpeningPresentation_.Show(reward->rarity);
		}
		return true;
	}

	void RewardSelectionController::Update(float deltaTime, InGamePhase phase, Relic::System& relicSystem,
		Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory, Player::Base& player) {

		// Editorによる報酬解除にも追従し、獲得対象のない宝箱表示を中断
		if (chestOpeningPresentation_.IsActive() && !relicSystem.HasPendingReward()) {
			chestOpeningPresentation_.Hide();
		}

		// 選択待機中も実時間で進行し、獲得確定後もフラッシュの透明化を継続
		const bool didFinishChestPresentation = chestOpeningPresentation_.Update(
			deltaTime, phase == InGamePhase::WaitingGetItem);
		relicAcquisitionUI_.SetDisplayEnabled(!chestOpeningPresentation_.IsActive());
		relicAcquisitionUI_.SetBackgroundEnabled(!chestOpeningPresentation_.IsFlashActive());
		if (phase == InGamePhase::WaitingGetItem) {
			if (!chestOpeningPresentation_.IsActive()) {

				// 開封完了Frameの決定入力が表示直後の獲得カードへ伝わることを抑止
				relicAcquisitionUI_.Update(deltaTime, relicSystem, !didFinishChestPresentation);
			}
		} else if (phase == InGamePhase::WaitingUpgrade) {
			weaponUpgradeUI_.Update(deltaTime, upgradeSystem, inventory, player);
		}
	}

	void RewardSelectionController::HideChest() {
		chestOpeningPresentation_.Hide();
	}

	void RewardSelectionController::FollowCamera(const Camera& camera) {
		chestOpeningPresentation_.FollowCamera(camera);
	}

	bool RewardSelectionController::ShouldBlockCameraInput() const {
		return chestOpeningPresentation_.IsActive();
	}

	bool RewardSelectionController::ShouldSuppressPlayerOutline() const {
		return chestOpeningPresentation_.IsActive() || chestOpeningPresentation_.IsFlashActive();
	}

	bool RewardSelectionController::ShouldPauseBgm(InGamePhase phase) const {

		// レリック獲得から武器選択へ連続遷移する場合も途中でBGMを再開しない条件
		if (phase == InGamePhase::WaitingUpgrade || phase == InGamePhase::WaitingGetItem) {
			return true;
		}

		// 獲得確定後に残る開封フラッシュの終了まで一時停止を継続
		return ShouldSuppressPlayerOutline();
	}

	int RewardSelectionController::GetSelectionBgmPlaybackId(InGamePhase phase,
		const Relic::System& relicSystem, const Weapon::UpgradeSystem& upgradeSystem) const {
		if (phase == InGamePhase::WaitingGetItem) {

			// 膨張中は無音とし、開封完了から獲得確定まで同じBGMを継続
			if (relicSystem.HasPendingReward() && !chestOpeningPresentation_.IsActive()) {
				return kRelicAcquisitionBgmPlaybackId;
			}
			return 0;
		}
		if (phase != InGamePhase::WaitingUpgrade) {
			return 0;
		}

		// リロールでは曲を継続し、選択確定で未処理回数が変化した時だけ曲を再開
		return upgradeSystem.GetPendingUpgradeCount();
	}

	void RewardSelectionController::DrawImGui(Weapon::UpgradeSystem& upgradeSystem,
		Weapon::Inventory& inventory, Relic::System& relicSystem) {
#ifdef USE_IMGUI
		weaponUpgradeUI_.DrawImGui(upgradeSystem, inventory);
		relicAcquisitionUI_.DrawImGui(relicSystem);
#else
		(void)upgradeSystem;
		(void)inventory;
		(void)relicSystem;
#endif
	}

}
