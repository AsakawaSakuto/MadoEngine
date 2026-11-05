#include "GameHud.h"
#include "GameObject/Map/Map.h"
#include "Input/MyInput.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
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
	/// @brief Pauseメニューの選択単位と拡大基準の参照名
	struct PauseMenuDefinition {
		const char* groupName;
		const char* pivotSpriteName;
	};
	constexpr const char* kPauseGroupName = "PauseGroup";
	constexpr PauseMenuDefinition kPauseMenuDefinitions[] = {
		{ "Continue", "ContinueSprite" },
		{ "ReStart", "ReStartSprite" },
		{ "Quit", "QuitSprite" },
	};
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
	constexpr const char* kMassiveEnemySpawnTextName = "MassiveEnemySpawnText";
	constexpr float kMassiveEnemySpawnTextInitialY = 170.0f;
	constexpr float kMassiveEnemySpawnTextEndY = -100.0f;
	constexpr float kMassiveEnemySpawnFadeDuration = 1.0f;
	constexpr float kMassiveEnemySpawnHoldDuration = 1.0f;
	constexpr float kMassiveEnemySpawnMoveDuration = 1.0f;
	constexpr float kMassiveEnemySpawnTotalDuration = kMassiveEnemySpawnFadeDuration +
		kMassiveEnemySpawnHoldDuration + kMassiveEnemySpawnMoveDuration;
	constexpr const char* kStaticHudTextNames[] = {
		"GamePlayTimerText", "OverTimeText", "PlayerHealthText", "TheGoalText", kMassiveEnemySpawnTextName,
	};
	constexpr const char* kMoneyGroupName = "MoneyUI";
	constexpr const char* kMoneyTextName = "MoneyText";
	constexpr const char* kKillCountGroupName = "KillCountUI";
	constexpr const char* kKillCountTextName = "KillCountText";
	constexpr const char* kDebugGroupName = "DebugText";
	constexpr const char* kEnemyCountTextName = "EnemyCountText";
	constexpr const char* kFpsCountTextName = "FpsCountText";
	constexpr const char* kSeedValueTextName = "SeedValueText";
	constexpr const char* kStaticHudGroupNames[] = {
		"OperationMethodUI", kMoneyGroupName, kKillCountGroupName, kDebugGroupName,
	};
}

namespace UI::Game {

	void GameHud::Initialize(int weaponSlotCount, const Relic::System& relicSystem,
		const MapMiniMapData& miniMapData, std::uint32_t gameSeed) {
		isVisible_ = true;
		isWorldViewsVisible_ = false;
		staticTexts_.clear();
		massiveEnemySpawnTimer_.Reset();
		isMassiveEnemySpawnJustStarted_ = false;
		UpdateMassiveEnemySpawnPresentation(0.0f);
		UpdateUpgradePresentation(0.0f, false);
		UpdatePausePresentation(false);

		// Editor配置の常設表示だけを管理対象へ登録し、開始演出とMap操作Textを分離
		for (const char* name : kStaticHudTextNames) {
			staticTexts_.push_back(name);
		}

		expGauge_.Initialize();
		healthGauge_.Initialize();
		staminaGauge_.Initialize();
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

		SetSeed(gameSeed);
	}

	void GameHud::Finalize() {
		massiveEnemySpawnTimer_.Reset();
		isMassiveEnemySpawnJustStarted_ = false;
		UpdateMassiveEnemySpawnPresentation(0.0f);
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
		gameSeed_ = 0;
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
		UpdateTexts(deltaTime, state);

		// 致死被弾後もゲーム進行から独立して被ダメージTextを最後まで再生
		playerDamageView_.Update(deltaTime);
		gamePlayTimerView_.Update(state.remainingTime, deltaTime);
		UpdateMassiveEnemySpawnPresentation(deltaTime);
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
		gameSeed_ = gameSeed;
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

	void GameHud::ShowMassiveEnemySpawn() {

		// 各通知で演出時間を初期化し、開始Frameの時間差分をフェードへ持ち込まない状態へ遷移
		massiveEnemySpawnTimer_.Start(kMassiveEnemySpawnTotalDuration);
		isMassiveEnemySpawnJustStarted_ = true;
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
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		MadoEngine::UIGroup* pauseGroup = groupManager.TryGet(groupManager.Find(kPauseGroupName, SceneType::Game));
		if (!pauseGroup) {
			return;
		}

		// 通常のPause中だけ親を表示し、暗幕・見出し・各項目の個別表示設定を維持
		const bool isVisible = isVisible_ && isPaused;
		pauseGroup->SetVisible(isVisible && pauseGroup->GetAuthoringState().visible);

		// 保存用のTransformから毎回計算し、倍率の累積と拡大中の値の保存を防止
		for (std::size_t index = 0; index < std::size(kPauseMenuDefinitions); ++index) {
			const auto& definition = kPauseMenuDefinitions[index];
			MadoEngine::UIGroup* menuGroup = pauseGroup->TryGetGroup(definition.groupName);
			if (!menuGroup) {
				continue;
			}
			const auto baseTransform = menuGroup->GetAuthoringState().transform;
			menuGroup->SetTransform(baseTransform);
			if (!isVisible || index != selectedIndex) {
				continue;
			}

			// 子が画面座標に近い位置でも背景の基準点を固定し、拡大によるボタン全体の移動を相殺
			Vector3 pivot{};
			if (const Sprite* background = menuGroup->TryGetSprite(definition.pivotSpriteName)) {
				const Vector2 position = background->GetAuthoringState().transform.translate;
				pivot = { position.x, position.y, 0.0f };
			}
			const Vector3 basePivot = Matrix::Transform(pivot, menuGroup->GetLocalMatrix());
			menuGroup->SetScale(baseTransform.scale * kPauseMenuSelectedScale);
			const Vector3 scaledPivot = Matrix::Transform(pivot, menuGroup->GetLocalMatrix());
			menuGroup->SetPosition({ baseTransform.translate.x + basePivot.x - scaledPivot.x,
				baseTransform.translate.y + basePivot.y - scaledPivot.y });
		}
	}

	void GameHud::ApplyStaticVisibility(bool isVisible) {

		// 再読込後も親Groupを再取得し、子の個別設定を維持したままHUDの表示条件を合成
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		for (const char* name : kStaticHudGroupNames) {
			MadoEngine::UIGroup* group = groupManager.TryGet(groupManager.Find(name, SceneType::Game));
			if (group) {
				group->SetVisible(isVisible && group->GetAuthoringState().visible);
			}
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

	void GameHud::UpdateMassiveEnemySpawnPresentation(float deltaTime) {

		// 通知成立FrameはAlphaを0に保ち、次Frameから実時間で演出を進行
		if (isMassiveEnemySpawnJustStarted_) {
			isMassiveEnemySpawnJustStarted_ = false;
		} else {
			massiveEnemySpawnTimer_.Update(deltaTime);
		}

		// Json再読込後も現在の演出時刻を最新Textへ反映し、X座標とRGBの編集値を維持
		MadoEngine::Text* text = MyText::TryGet(MyText::Find(kMassiveEnemySpawnTextName));
		if (!text || text->GetSceneType() != SceneType::Game) {
			return;
		}
		const bool hasStarted = massiveEnemySpawnTimer_.IsActive() || massiveEnemySpawnTimer_.IsFinished();
		Vector2 position = text->GetPosition();
		position.y = kMassiveEnemySpawnTextInitialY;
		Vector4 color = text->GetColor();
		color.w = 0.0f;
		if (hasStarted) {

			// フェード完了後はAlpha1とY座標を1秒維持し、その後にEaseInBackで退場
			const float fadeEndProgress = kMassiveEnemySpawnFadeDuration / kMassiveEnemySpawnTotalDuration;
			const float moveStartProgress = (kMassiveEnemySpawnFadeDuration + kMassiveEnemySpawnHoldDuration) /
				kMassiveEnemySpawnTotalDuration;
			color.w = massiveEnemySpawnTimer_.GetProgressUntil(fadeEndProgress);
			const float moveProgress = massiveEnemySpawnTimer_.GetProgressAfter(moveStartProgress);
			if (moveProgress >= 1.0f) {

				// 補間の丸め誤差を残さず、退場完了後は指定された終点へ固定
				position.y = kMassiveEnemySpawnTextEndY;
			} else {
				position.y = Easing::Lerp(kMassiveEnemySpawnTextInitialY, kMassiveEnemySpawnTextEndY,
					moveProgress, EaseType::EaseInBack);
			}
		}
		text->SetPosition(position);
		text->SetColor(color);
		text->SetVisible(hasStarted);
	}

	void GameHud::UpdateTexts(float deltaTime, const GameHudState& state) {

		// リロール成立後の次回費用を数値だけで反映し、再生成されたTextにも追従
		MadoEngine::Text* rerollCoinText = MyText::TryGet(MyText::Find(kUpgradeRerollCoinTextName));
		if (rerollCoinText && rerollCoinText->GetSceneType() == SceneType::Game) {
			rerollCoinText->SetText(std::format("{}", state.upgradeRerollCost));
		}

		// 親Group内の登録名から毎回解決し、金額が同じでも再読込後のTextへ同期
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		MadoEngine::UIGroup* moneyGroup = groupManager.TryGet(groupManager.Find(kMoneyGroupName, SceneType::Game));
		if (moneyGroup) {
			if (MadoEngine::Text* moneyText = moneyGroup->TryGetText(kMoneyTextName)) {
				const int currentMoney = static_cast<int>(state.playerStatus.currentMoney);
				moneyText->SetText(std::format("{}", currentMoney));
			}
		}

		// 撃破数も親Group内の登録名から解決し、再生成されたTextへ最新値を同期
		MadoEngine::UIGroup* killCountGroup = groupManager.TryGet(groupManager.Find(kKillCountGroupName, SceneType::Game));
		if (killCountGroup) {
			if (MadoEngine::Text* killCountText = killCountGroup->TryGetText(kKillCountTextName)) {
				killCountText->SetText(std::format("{}", state.killCount));
			}
		}

		// Debug表示も親から解決し、再読込後のTextへ現在の敵数とSeedを復元
		MadoEngine::Text* fpsCountText = nullptr;
		MadoEngine::UIGroup* debugGroup = groupManager.TryGet(groupManager.Find(kDebugGroupName, SceneType::Game));
		if (debugGroup) {
			if (MadoEngine::Text* enemyCountText = debugGroup->TryGetText(kEnemyCountTextName)) {
				enemyCountText->SetText(std::format("Enemy : {}", state.enemyCount));
			}
			if (MadoEngine::Text* seedValueText = debugGroup->TryGetText(kSeedValueTextName)) {
				seedValueText->SetText(std::format("SEED : {}", gameSeed_));
			}
			fpsCountText = debugGroup->TryGetText(kFpsCountTextName);
		}
		fpsMeasurementView_.Update(deltaTime, fpsCountText);
	}

}
