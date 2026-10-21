#include "GameHud.h"
#include "GameObject/Map/Map.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Easing/Easing.h"
#include <format>

namespace {
	constexpr const char* kGameStartBandNames[] = { "Band_Up", "Band_Bottom" };
	constexpr float kGameStartBandInitialScaleY = 8.0f;
	constexpr const char* kGameStartTextName = "StartText";
	constexpr const char* kStaticHudSpriteNames[] = {
		"MoneyUI", "KillUI",
		"InputUI1_Pad", "InputUI2_Pad", "InputUI3_Pad", "InputUI4_Pad",
		"InputUI1_Key", "InputUI2_Key", "InputUI3_Key", "InputUI4_Key",
	};
	constexpr const char* kStaticHudTextNames[] = {
		"EnemyCountText", "GamePlayTimerText", "PlayerHealthText", "FpsText",
		"KillCountText", "SeedValueText", "MoneyText", "Text", "Text1", "Text2", "Text3", "Text4",
	};
}

namespace UI::Game {

	void GameHud::Initialize(int weaponSlotCount, const Relic::System& relicSystem,
		const MapMiniMapData& miniMapData, std::uint32_t gameSeed) {
		isVisible_ = true;
		isWorldViewsVisible_ = false;
		staticSprites_.clear();
		staticTexts_.clear();

		// Editor配置の常設表示だけを管理対象へ登録し、開始演出とMap操作Textを分離
		for (const char* name : kStaticHudSpriteNames) {
			staticSprites_.push_back(name);
		}
		for (const char* name : kStaticHudTextNames) {
			staticTexts_.push_back(name);
		}

		expGauge_.Initialize();
		healthGauge_.Initialize();
		staminaGauge_.Initialize();
		enemyCountText_ = MyText::Create("EnemyCountText", "Enemy : 0", SceneType::Game,
			MadoEngine::EditorManagementMode::EditorManaged, MadoEngine::Render::RenderLayer::UI);
		fpsMeasurementView_.Initialize();
		gamePlayTimerView_.Initialize();
		UpdateGameStartPresentation(0.0f);
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

	void GameHud::SetVisible(bool isVisible) {
		ApplyStaticVisibility(isVisible);
		if (isVisible_ == isVisible) {
			return;
		}
		isVisible_ = isVisible;

		// 各部品の所有Objectを通して切り替え、装備や壁登りなどの個別表示条件を維持
		expGauge_.SetVisible(isVisible_);
		healthGauge_.SetVisible(isVisible_);
		staminaGauge_.SetVisible(isVisible_);
		playerIconUI_.SetVisible(isVisible_);
		weaponIconUI_.SetVisible(isVisible_);
		relicInventoryUI_.SetVisible(isVisible_);
		miniMapUI_.SetVisible(isVisible_);

		const bool areNotificationsVisible = isVisible_ && isWorldViewsVisible_;
		enemyDamageView_.SetVisible(areNotificationsVisible);
		enemyStatusEffectIconView_.SetVisible(areNotificationsVisible);
		playerResourceGainView_.SetVisible(areNotificationsVisible);
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
		UpdateGameStartPresentation(state.gameStartProgress);
		ApplyStaticVisibility(isVisible_);
	}

	void GameHud::UpdateWorldViews(float gameDeltaTime, bool isPlaying,
		const Enemy::Manager& enemyManager, const Camera& camera) {

		// 選択待機中の通知表示を隠し、再開時まで表示寿命の進行を停止
		isWorldViewsVisible_ = isPlaying;
		const bool areNotificationsVisible = isVisible_ && isWorldViewsVisible_;
		enemyDamageView_.SetVisible(areNotificationsVisible);
		enemyDamageView_.Update(gameDeltaTime, camera);
		enemyStatusEffectIconView_.SetVisible(areNotificationsVisible);
		enemyStatusEffectIconView_.Update(enemyManager, camera);
		playerResourceGainView_.SetVisible(areNotificationsVisible);
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
		miniMapUI_.Update(playerPosition, playerFacingYaw, miniMapObjectPositions_, isVisible_);
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

	void GameHud::ApplyStaticVisibility(bool isVisible) {

		// 保存用のVisibleを変更せず、一時的なHUD非表示だけを描画判定へ反映
		for (const char* name : staticSprites_) {
			Sprite* sprite = MySprite::TryGet(MySprite::Find(name));
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			sprite->SetRuntimeVisible(isVisible);
		}

		// 非表示中も名前から再取得し、Json再読込で復元された常設Textの表示漏れを防止
		for (const char* name : staticTexts_) {
			MadoEngine::Text* text = MyText::TryGet(MyText::Find(name));
			if (!text || text->GetSceneType() != SceneType::Game) {
				continue;
			}
			text->SetRuntimeVisible(isVisible);
		}
	}

	void GameHud::UpdateGameStartPresentation(float progress) {
		const float scaleY = Easing::Lerp(kGameStartBandInitialScaleY, 0.0f, progress, EaseType::EaseInCirc);

		// Json再読込やEditor削除に対応するため現在のHandleを名前から毎Frame取得
		for (const char* name : kGameStartBandNames) {
			Sprite* band = MySprite::TryGet(MySprite::Find(name));
			if (!band || band->GetSceneType() != SceneType::Game) {
				continue;
			}

			// 横幅と上下端のAnchorを維持し、高さだけを同じ進行率で縮小
			Vector2 scale = band->GetScale();
			scale.y = scaleY;
			band->SetScale(scale);
		}

		// 帯と同じ開始待機の進行率でAlphaだけを補間し、Editor設定の文字色を維持
		MadoEngine::Text* startText = MyText::TryGet(MyText::Find(kGameStartTextName));
		if (!startText || startText->GetSceneType() != SceneType::Game) {
			return;
		}
		Vector4 color = startText->GetColor();
		color.w = Easing::Lerp(0.0f, 1.0f, progress, EaseType::Linear);
		startText->SetColor(color);
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
