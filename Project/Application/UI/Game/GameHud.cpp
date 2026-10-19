#include "GameHud.h"
#include "GameObject/Map/Map.h"
#include "Render/Object/2d/Text/MyText.h"
#include <format>

namespace UI::Game {

	void GameHud::Initialize(int weaponSlotCount, const Relic::System& relicSystem,
		const MapMiniMapData& miniMapData, std::uint32_t gameSeed) {
		expGauge_.Initialize();
		healthGauge_.Initialize();
		staminaGauge_.Initialize();
		enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Game,
			MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
		fpsMeasurementView_.Initialize();
		gamePlayTimerView_.Initialize();
		playerResourceGainView_.Initialize();
		enemyDamageView_.Initialize();
		enemyStatusEffectIconView_.Initialize();
		miniMapUI_.Initialize(miniMapData);
		playerIconUI_.Initialize();

		// 武器Level表示を報酬選択UIより先に登録して選択カードの背面へ配置
		weaponIconUI_.Initialize(weaponSlotCount);
		relicInventoryUI_.Initialize(relicSystem);

		const MadoEngine::TextHandle textHandle = MyText::Find("Text");
		if (MadoEngine::Text* text = MyText::TryGet(textHandle)) {
			text->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
		}
		moneyText_ = MyText::Find("MoneyText");
		killCountText_ = MyText::Find("KillCountText");
		displayedMoney_ = -1;
		SetSeed(gameSeed);
	}

	void GameHud::Finalize() {
		relicInventoryUI_.Finalize();
		healthGauge_.Finalize();
		staminaGauge_.Finalize();
		fpsMeasurementView_.Finalize();
		gamePlayTimerView_.Finalize();
		miniMapUI_.Finalize();

		// 再生成時に使い回す座標Bufferの内容をScene終了時に破棄
		for (std::vector<Vector3>& positions : miniMapObjectPositions_) {
			positions.clear();
		}
		playerResourceGainView_.Finalize();
		enemyDamageView_.Finalize();
		enemyStatusEffectIconView_.Finalize();
		enemyCountText_ = {};
		moneyText_ = {};
		killCountText_ = {};
		displayedMoney_ = -1;
	}

	void GameHud::Update(float deltaTime, float gameDeltaTime, const GameHudState& state,
		const Weapon::Inventory& inventory, const Camera& camera) {

		// 武器のCooldownと攻撃演出はゲーム停止へ追従し、選択中のゲージ演出とFPSは実時間で継続
		weaponIconUI_.Update(gameDeltaTime, inventory);
		expGauge_.Update(state.playerStatus.currentExp, state.playerStatus.expToNextLevel);
		expGauge_.IsUpgrade(state.isWaitingUpgradeSelection, deltaTime);
		healthGauge_.Update(state.playerPosition, state.playerStatus.currentHealth,
			state.playerStatus.maxHealth, camera);
		staminaGauge_.Update(state.wallClimbRemainingTime, state.wallClimbMaxDuration,
			state.isWallClimbGaugeVisible);
		UpdateTexts(state);
		fpsMeasurementView_.Update(deltaTime);
		gamePlayTimerView_.Update(state.remainingTime);
	}

	void GameHud::UpdateWorldViews(float gameDeltaTime, bool isPlaying,
		const Enemy::Manager& enemyManager, const Camera& camera) {

		// 選択待機中の通知表示を隠し、再開時まで表示寿命の進行を停止
		enemyDamageView_.SetVisible(isPlaying);
		enemyDamageView_.Update(gameDeltaTime, camera);
		enemyStatusEffectIconView_.SetVisible(isPlaying);
		enemyStatusEffectIconView_.Update(enemyManager, camera);
		playerResourceGainView_.SetVisible(isPlaying);
		playerResourceGainView_.Update(gameDeltaTime);
	}

	void GameHud::UpdateRelicInventory(const Relic::System& relicSystem) {
		relicInventoryUI_.Update(relicSystem);
	}

	void GameHud::RebuildMiniMap(const MapMiniMapData& data) {
		miniMapUI_.Initialize(data);
	}

	void GameHud::UpdateMiniMap(const Map& map, const Vector3& playerPosition, float playerFacingYaw) {

		// 地形は再構築時だけ更新し、毎Frameは再利用Bufferへ残存Objectの座標だけを収集
		map.CollectMiniMapObjectPositions(miniMapObjectPositions_);
		miniMapUI_.Update(playerPosition, playerFacingYaw, miniMapObjectPositions_, true);
	}

	void GameHud::SetSeed(std::uint32_t gameSeed) {
		const MadoEngine::TextHandle seedValueTextHandle = MyText::Find("SeedValueText");
		if (MadoEngine::Text* seedValueText = MyText::TryGet(seedValueTextHandle)) {
			seedValueText->SetText(std::format("Seed : {}", gameSeed));
		}
	}

	void GameHud::ShowDamage(const Combat::DamageEvent& event) {
		enemyDamageView_.Spawn(event);
	}

	void GameHud::ShowResourceGain(const Player::ResourceGainEvent& event) {
		playerResourceGainView_.Spawn(event.type, event.amount);
	}

	void GameHud::PlayWeaponAttackAnimation(std::size_t slotIndex) {
		weaponIconUI_.PlayAttackAnimation(slotIndex);
	}

	void GameHud::DrawImGui() {
#ifdef USE_IMGUI
		playerResourceGainView_.DrawImGui();
#endif
	}

	void GameHud::UpdateTexts(const GameHudState& state) {
		const int currentMoney = static_cast<int>(state.playerStatus.currentMoney);
		if (currentMoney != displayedMoney_) {

			// 所持金の変化時だけHandleを再解決し、Json再読込による世代変更へ追従
			MadoEngine::Text* moneyText = MyText::TryGet(moneyText_);
			if (!moneyText) {
				moneyText_ = MyText::Find("MoneyText");
				moneyText = MyText::TryGet(moneyText_);
			}
			if (moneyText) {
				moneyText->SetText(std::format("{}", currentMoney));
				displayedMoney_ = currentMoney;
			}
		}

		// 全Weaponの累計撃破数が変化したFrameだけText表示を更新
		MadoEngine::Text* killCountText = MyText::TryGet(killCountText_);
		if (!killCountText) {
			killCountText_ = MyText::Find("KillCountText");
			killCountText = MyText::TryGet(killCountText_);
		}
		if (killCountText) {
			const std::string displayText = std::format("{}", state.killCount);
			if (killCountText->GetText() != displayText) {
				killCountText->SetText(displayText);
			}
		}

		// Editor側で表示Objectが再生成された場合も名前から最新Handleを再取得
		enemyCountText_ = MyText::Find("EnemyCountText");
		if (MadoEngine::Text* enemyCountText = MyText::TryGet(enemyCountText_)) {
			enemyCountText->SetText(std::format("Enemy : {}", state.enemyCount));
		}
	}

}
