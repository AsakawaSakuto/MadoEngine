#include "GameHud.h"
#include "GameObject/Map/Map.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <format>

namespace {
	constexpr UI::Game::InputControlFeedbackDefinition kUpgradeControlDefinitions[] = {
		{ "Decision", { "UpgradeDecisionBottom", "UpgradeDecisionBack1", "UpgradeDecisionBack2" }, "UpgradeDecisionText" },
		{ "Reroll", { "UpgradeRerollBottom", "UpgradeRerollBack1", "UpgradeRerollBack2" }, "UpgradeRerollText" },
	};
	constexpr const char* kGameStartBandNames[] = { "Band_Up", "Band_Bottom" };
	constexpr float kGameStartBandInitialScaleY = 8.0f;
	constexpr const char* kGameStartTextName = "StartText";
	constexpr const char* kUpgradeTextName = "UpgradeText";
	constexpr const char* kWeaponUpgradeOverlayName = "WeaponUpgradeOverlay";
	constexpr const char* kPauseSpriteNames[] = {
		"PauseSheet",
		"PauseContinueBack1", "PauseContinueBack2",
		"PauseReStartBack1", "PauseReStartBack2",
		"PauseQuitBack1", "PauseQuitBack2",
	};
	constexpr const char* kPauseTextNames[] = {
		"PauseText", "PauseContinueText", "PauseReStartText", "PauseQuitText",
	};
	constexpr Vector2 kPauseMenuSpriteBaseScales[] = {
		{ 10.75f, 4.5f },
		{ 9.75f, 3.5f },
	};
	constexpr Vector2 kPauseMenuTextBaseScale = { 1.0f, 1.0f };
	constexpr float kPauseMenuSelectedScale = 1.1f;
	constexpr const char* kUpgradeRerollCoinTextName = "UpgradeRerollCoinText";
	constexpr const char* kUpgradeSpriteNames[] = {
		"UpgradeDecisionBack1", "UpgradeDecisionBack2",
		"UpgradeRerollBack1", "UpgradeRerollBack2",
		"UpgradeDecisionBottom", "UpgradeRerollBottom", "UpgradeRerollCoin",
	};
	constexpr const char* kUpgradeControlTextNames[] = {
		"UpgradeDecisionText", "UpgradeRerollText", kUpgradeRerollCoinTextName,
	};
	constexpr float kUpgradeTextColorChangeTime = 0.5f;
	constexpr float kUpgradeTextPulseDuration = 0.6f;
	constexpr float kUpgradeTextPulseStartSize = 45.0f;
	constexpr float kUpgradeTextPulseEndSize = 60.0f;
	constexpr float kGameStartTextInitialX = 640.0f;
	constexpr float kGameStartTextEndX = -360.0f;
	constexpr float kGameStartTextY = 256.0f;
	constexpr float kGameStartTextMoveDuration = 1.0f;
	constexpr const char* kGoalTextName = "TheGoalText";
	constexpr float kGoalTextInitialX = -200.0f;
	constexpr float kGoalTextEndX = 232.0f;
	constexpr float kGoalTextMoveDelay = 2.0f;
	constexpr float kGoalTextMoveDuration = 1.0f;
	constexpr const char* kStaticHudSpriteNames[] = {
		"MoneyUI", "KillUI",
		"InputUI1_Pad", "InputUI2_Pad", "InputUI3_Pad", "InputUI4_Pad",
		"InputUI1_Key", "InputUI2_Key", "InputUI3_Key", "InputUI4_Key",
	};
	constexpr const char* kStaticHudTextNames[] = {
		"EnemyCountText", "GamePlayTimerText", "PlayerHealthText", "FpsText",
		"KillCountText", "SeedValueText", "MoneyText", "TheGoalText", "Text", "Text1", "Text2", "Text3", "Text4",
	};
}

namespace UI::Game {

	void GameHud::Initialize(int weaponSlotCount, const Relic::System& relicSystem,
		const MapMiniMapData& miniMapData, std::uint32_t gameSeed) {
		isVisible_ = true;
		isWorldViewsVisible_ = false;
		staticSprites_.clear();
		staticTexts_.clear();
		UpdateUpgradePresentation(0.0f, false);
		UpdatePausePresentation(false);

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
		UpdateGameStartPresentation(0.0f, 0.0f);
		playerResourceGainView_.Initialize();
		playerDamageView_.Initialize();
		dropObjectGetView_.Initialize();
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
		UpdateUpgradePresentation(0.0f, false);
		UpdatePausePresentation(false);
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
		playerDamageView_.Finalize();
		dropObjectGetView_.Finalize();
		enemyDamageView_.Finalize();
		enemyStatusEffectIconView_.Finalize();
		enemyCountText_ = {};
		moneyText_ = {};
		killCountText_ = {};
		displayedMoney_ = -1;
	}

	void GameHud::SetVisible(bool isVisible) {
		ApplyStaticVisibility(isVisible);
		if (!isVisible) {
			UpdateUpgradePresentation(0.0f, false);
			UpdatePausePresentation(false);
		}
		if (isVisible_ == isVisible) {
			return;
		}
		isVisible_ = isVisible;
		playerDamageView_.SetVisible(isVisible_);

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
		dropObjectGetView_.SetVisible(areNotificationsVisible);
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

		// 致死被弾後もゲーム進行から独立して被ダメージTextを最後まで再生
		playerDamageView_.Update(deltaTime);
		fpsMeasurementView_.Update(deltaTime);
		gamePlayTimerView_.Update(state.remainingTime);
		UpdateGameStartPresentation(state.gameStartProgress, state.gamePlayElapsedTime);
		ApplyStaticVisibility(isVisible_);
		UpdateUpgradePresentation(deltaTime, isVisible_ && state.isWaitingUpgradeSelection);
		UpdatePausePresentation(state.isPaused, state.pauseMenuSelectedIndex);
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
		dropObjectGetView_.SetVisible(areNotificationsVisible);
		dropObjectGetView_.Update(gameDeltaTime);
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
			seedValueText->SetText(std::format("SEED : {}", gameSeed));
		}
	}

	void GameHud::ShowDamage(const Combat::DamageEvent& event) {
		enemyDamageView_.Spawn(event);
	}

	void GameHud::ShowResourceGain(const Player::ResourceGainEvent& event) {
		playerResourceGainView_.Spawn(event.type, event.amount);
	}

	void GameHud::ShowPlayerDamage(float damage) {
		playerDamageView_.Spawn(damage);
	}

	void GameHud::ShowDropObjectGet(DropObject::Type type) {
		dropObjectGetView_.Spawn(type);
	}

	void GameHud::PlayWeaponAttackAnimation(std::size_t slotIndex) {
		weaponIconUI_.PlayAttackAnimation(slotIndex);
	}

	void GameHud::DrawImGui() {
#ifdef USE_IMGUI
		playerResourceGainView_.DrawImGui();
#endif
	}

	void GameHud::UpdatePausePresentation(bool isPaused, std::size_t selectedIndex) {
		const bool isVisible = isVisible_ && isPaused;

		// 武器強化や報酬選択による停止と区別し、暗幕とメニュー背景を通常のPause中だけ表示
		for (const char* name : kPauseSpriteNames) {
			Sprite* sprite = MySprite::TryGet(MySprite::Find(name));
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			sprite->SetVisible(isVisible);
		}

		// Json再読込やEditor削除後も存在するTextだけへ背景と同じ表示条件を反映
		for (const char* name : kPauseTextNames) {
			MadoEngine::Text* text = MyText::TryGet(MyText::Find(name));
			if (!text || text->GetSceneType() != SceneType::Game) {
				continue;
			}
			text->SetVisible(isVisible);
		}

		// 色と配置を維持し、選択項目の背景2枚と文字を基準Scaleから同じ倍率で拡大
		for (std::size_t index = 0; index < std::size(kPauseTextNames) - 1; ++index) {
			float scaleMultiplier = 1.0f;
			if (isVisible && index == selectedIndex) {
				scaleMultiplier = kPauseMenuSelectedScale;
			}

			// 拡大中の保存値を次回の基準へ持ち込まないようBack1とBack2の固定寸法から毎回計算
			for (std::size_t spriteIndex = 0; spriteIndex < std::size(kPauseMenuSpriteBaseScales); ++spriteIndex) {
				const std::size_t nameIndex = 1 + index * std::size(kPauseMenuSpriteBaseScales) + spriteIndex;
				Sprite* sprite = MySprite::TryGet(MySprite::Find(kPauseSpriteNames[nameIndex]));
				if (!sprite || sprite->GetSceneType() != SceneType::Game) {
					continue;
				}
				sprite->SetScale(kPauseMenuSpriteBaseScales[spriteIndex] * scaleMultiplier);
			}

			// Textも保存済みScaleを基準にせず、非選択時とPause解除時は通常寸法へ復元
			MadoEngine::Text* text = MyText::TryGet(MyText::Find(kPauseTextNames[index + 1]));
			if (text && text->GetSceneType() == SceneType::Game) {
				text->SetScale(kPauseMenuTextBaseScale * scaleMultiplier);
			}
		}
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

	void GameHud::UpdateUpgradePresentation(float deltaTime, bool isVisible) {

		// Editor配置の操作案内Spriteを最新Handleから取得し、武器強化選択中だけ表示
		MadoEngine::SpriteHandle previousSprite = MySprite::Find(kWeaponUpgradeOverlayName);
		for (const char* name : kUpgradeSpriteNames) {
			const MadoEngine::SpriteHandle spriteHandle = MySprite::Find(name);
			Sprite* sprite = MySprite::TryGet(spriteHandle);
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			sprite->SetVisible(isVisible);

			// 暗幕の直後へ案内を順に配置し、再生成後も既存インスタンスを手前に描画
			if (isVisible && MySprite::MoveAfter(spriteHandle, previousSprite)) {
				previousSprite = spriteHandle;
			}
		}

		// Json再読込やEditor削除後も存在する操作案内Textだけへ同じ表示条件を反映
		for (const char* name : kUpgradeControlTextNames) {
			MadoEngine::Text* text = MyText::TryGet(MyText::Find(name));
			if (!text || text->GetSceneType() != SceneType::Game) {
				continue;
			}
			text->SetVisible(isVisible);
		}
		UpdateUpgradeControlFeedback(deltaTime, isVisible);
		if (!isVisible) {

			// 次の武器強化イベントで赤色から循環を開始するため非表示中は時刻を初期化
			upgradeTextColor_.Reset();
		}

		// Json再読込やEditor削除後も現在のTextだけへイベントの表示状態を反映
		const MadoEngine::TextHandle textHandle = MyText::Find(kUpgradeTextName);
		MadoEngine::Text* upgradeText = MyText::TryGet(textHandle);
		if (!upgradeText || upgradeText->GetSceneType() != SceneType::Game) {
			upgradeTextPulseTimer_.Reset();
			upgradeText_ = {};
			return;
		}
		upgradeText->SetVisible(isVisible);
		if (!isVisible) {

			// 演出で変更したScaleを非表示時に戻し、次回の基準Scaleへの拡大率の累積を防止
			if (upgradeTextPulseTimer_.IsActive() && upgradeText_ == textHandle) {
				upgradeText->SetScale(upgradeTextBaseScale_);
			}
			upgradeTextPulseTimer_.Reset();
			upgradeText_ = {};
			return;
		}

		// 表示開始とText再生成時に基準Scaleを取得し、Editor設定の縦横比を維持
		if (!upgradeTextPulseTimer_.IsActive() || upgradeText_ != textHandle) {
			upgradeText_ = textHandle;
			upgradeTextBaseScale_ = upgradeText->GetScale();
			upgradeTextPulseTimer_.Start(kUpgradeTextPulseDuration, true);
		}
		upgradeTextPulseTimer_.Update(deltaTime);

		// EaseOutQuadで指定サイズ範囲の変化を繰り返し、文字画像の再生成を避けるためScaleへ換算
		const float pulseSize = Easing::Lerp(kUpgradeTextPulseStartSize, kUpgradeTextPulseEndSize,
			upgradeTextPulseTimer_.GetProgress(), EaseType::EaseOutQuad);
		const float pulseScale = pulseSize / (std::max)(1.0f, upgradeText->GetFontSize());
		upgradeText->SetScale(upgradeTextBaseScale_ * pulseScale);

		// 強化選択によるゲーム停止中も実時間で色相を循環し、設定済みAlphaを維持
		Vector4 color = upgradeTextColor_.Update(deltaTime, kUpgradeTextColorChangeTime);
		color.w = upgradeText->GetColor().w;
		upgradeText->SetColor(color);
	}

	void GameHud::UpdateUpgradeControlFeedback(float deltaTime, bool isVisible) {
		for (std::size_t index = 0; index < upgradeControlFeedback_.size(); ++index) {
			upgradeControlFeedback_[index].Update(deltaTime, isVisible, kUpgradeControlDefinitions[index]);
		}
	}

	void GameHud::UpdateGameStartPresentation(float progress, float gamePlayElapsedTime) {
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

		// Playing開始後2秒間は初期位置を保ち、Pauseを除いた経過時間で目標Textをスライド
		MadoEngine::Text* goalText = MyText::TryGet(MyText::Find(kGoalTextName));
		if (goalText && goalText->GetSceneType() == SceneType::Game) {
			const float goalMoveProgress = (gamePlayElapsedTime - kGoalTextMoveDelay) / kGoalTextMoveDuration;
			Vector2 position = goalText->GetPosition();
			position.x = Easing::Lerp(kGoalTextInitialX, kGoalTextEndX, goalMoveProgress, EaseType::EaseOutExpo);
			goalText->SetPosition(position);
		}

		// 帯と同じ開始待機の進行率でAlphaだけを補間し、Editor設定の文字色を維持
		MadoEngine::Text* startText = MyText::TryGet(MyText::Find(kGameStartTextName));
		if (!startText || startText->GetSceneType() != SceneType::Game) {
			return;
		}
		Vector4 color = startText->GetColor();
		color.w = Easing::Lerp(0.0f, 1.0f, progress, EaseType::Linear);
		startText->SetColor(color);

		// 縦移動の終了座標がJsonに残っていても高さを中央へ戻し、Sessionの累積時間でX方向だけ移動
		const float moveProgress = gamePlayElapsedTime / kGameStartTextMoveDuration;
		Vector2 position = startText->GetPosition();
		position.x = Easing::Lerp(kGameStartTextInitialX, kGameStartTextEndX, moveProgress, EaseType::EaseInBack);
		position.y = kGameStartTextY;
		startText->SetPosition(position);
	}

	void GameHud::UpdateTexts(const GameHudState& state) {

		// リロール成立後の次回費用を数値だけで反映し、再生成されたTextにも追従
		MadoEngine::Text* rerollCoinText = MyText::TryGet(MyText::Find(kUpgradeRerollCoinTextName));
		if (rerollCoinText && rerollCoinText->GetSceneType() == SceneType::Game) {
			rerollCoinText->SetText(std::format("{}", state.upgradeRerollCost));
		}
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
