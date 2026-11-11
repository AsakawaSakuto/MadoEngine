#include "GameHud.h"
#include "GameObject/Map/Map.h"
#include "Input/MyInput.h"
#include "Math/LerpEndpoints.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <format>

namespace {
	constexpr float kGameStartBandInitialScaleY = 8.0f; // 開始待機時の上下帯の初期縦倍率
	constexpr float kPauseMenuSelectedScale = 1.1f;     // ポーズメニューの選択中項目に適用する拡大倍率

	constexpr float kUpgradeTextColorChangeTime = 0.5f;                   // 強化見出しの色相を1区間進める秒数
	constexpr float kUpgradeTextPulseDuration = 0.6f;                     // 強化見出しの拡大演出を繰り返す周期（秒）
	constexpr Math::LerpEndpoints kUpgradeTextPulseSize{ 75.0f, 100.0f }; // 強化見出しの拡大開始時と終了時の文字サイズ

	constexpr Math::LerpEndpoints kGameStartTextX{ 640.0f, -360.0f }; // 開始テキストの表示位置と退場先のX座標
	constexpr float kGameStartTextY = 256.0f;                         // 開始テキストの移動中に固定するY座標
	constexpr float kGameStartTextMoveDuration = 1.0f;                // プレイ開始から開始テキストの退場完了までの秒数

	constexpr Math::LerpEndpoints kGoalTextX{ -200.0f, 232.0f }; // 目標テキストの登場前と登場後のX座標
	constexpr float kGoalTextMoveDelay = 2.0f;                   // プレイ開始から目標テキストの移動開始までの秒数
	constexpr float kGoalTextMoveDuration = 1.0f;                // 目標テキストが画面内の定位置に到達するまでの移動時間（秒）

	constexpr Math::LerpEndpoints kMassiveEnemySpawnTextY{ 170.0f, -100.0f }; // 大量出現通知の表示位置と退場先のY座標
	constexpr float kMassiveEnemySpawnFadeDuration = 1.0f;                    // 大量出現通知のフェードイン時間（秒）
	constexpr float kMassiveEnemySpawnHoldDuration = 1.0f;                    // フェードイン後に通知を定位置に留める秒数
	constexpr float kMassiveEnemySpawnMoveDuration = 1.0f;                    // 通知が画面上方へ退場するまでの移動時間（秒）

	// 通知タイマーと各段階の進行率の計算に使う演出全体の秒数
	constexpr float kMassiveEnemySpawnTotalDuration = kMassiveEnemySpawnFadeDuration +
		kMassiveEnemySpawnHoldDuration + kMassiveEnemySpawnMoveDuration;
}

namespace UI::Game {

	void GameHud::Initialize(int weaponSlotCount, const Relic::System& relicSystem,
		const MapMiniMapData& miniMapData, std::uint32_t gameSeed) {
		isVisible_ = true;
		isWorldViewsVisible_ = false;
		massiveEnemySpawnTimer_.Reset();
		isMassiveEnemySpawnJustStarted_ = false;
		UpdateMassiveEnemySpawnPresentation(0.0f);
		UpdateUpgradePresentation(0.0f, false);
		UpdatePausePresentation(false);

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
		healthGauge_.DrawImGui();
#endif
	}

	void GameHud::UpdatePausePresentation(bool isPaused, std::size_t selectedIndex) {
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		MadoEngine::UIGroup* pauseGroup = groupManager.TryGet(groupManager.Find("PauseGroup", SceneType::Game));
		if (!pauseGroup) {
			return;
		}

		// 通常のPause中だけ親を表示し、暗幕・見出し・各項目の個別表示設定を維持
		const bool isVisible = isVisible_ && isPaused;
		pauseGroup->SetVisible(isVisible && pauseGroup->GetAuthoringState().visible);

		// 選択番号と一致するようPauseMenuItemの定義順でGroup名を対応付け
		constexpr const char* kPauseMenuGroupNames[] = { "Continue", "ReStart", "Quit" };

		// 各項目の原点を背景の中心に配置し、保存用Transformを基準に位置を固定したまま拡大
		for (std::size_t index = 0; index < std::size(kPauseMenuGroupNames); ++index) {
			MadoEngine::UIGroup* menuGroup = pauseGroup->TryGetGroup(kPauseMenuGroupNames[index]);
			if (!menuGroup) {
				continue;
			}
			auto transform = menuGroup->GetAuthoringState().transform;
			if (isVisible && index == selectedIndex) {
				transform.scale *= kPauseMenuSelectedScale;
			}
			menuGroup->SetTransform(transform);
		}
	}

	void GameHud::ApplyStaticVisibility(bool isVisible) {

		// HUD全体の表示条件を親に適用し、タイマーや通知が管理する子の表示状態を維持
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		if (MadoEngine::UIGroup* group = groupManager.TryGet(groupManager.Find("StaticHud", SceneType::Game))) {
			group->SetVisible(isVisible && group->GetAuthoringState().visible);
		}
	}

	void GameHud::UpdateUpgradePresentation(float deltaTime, bool isVisible) {

		// 親だけでUpgrade中の表示を切り替え、子の個別設定と保存用表示状態を維持
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		const auto upgradeHandle = groupManager.Find("Upgrade", SceneType::Game);
		MadoEngine::UIGroup* upgradeGroup = groupManager.TryGet(upgradeHandle);
		if (upgradeGroup) {
			upgradeGroup->SetVisible(isVisible && upgradeGroup->GetAuthoringState().visible);
		}
		UpdateUpgradeControlFeedback(deltaTime, upgradeHandle);

		// 管理名の変更や再読込に追従し、Upgrade内の登録名に限定して見出しを解決
		MadoEngine::TextHandle textHandle{};
		for (const auto& element : groupManager.GetElements(upgradeHandle)) {
			if (element.resolved && element.type == MadoEngine::UIGroupElementType::Text && element.name == "UpgradeText") {
				textHandle = element.text;
				break;
			}
		}
		MadoEngine::Text* upgradeText = MyText::TryGet(textHandle);
		bool isTextVisible = false;
		if (upgradeText) {
			isTextVisible = isVisible && upgradeText->IsVisibleForRendering();
		}
		if (!isTextVisible || upgradeText_ != textHandle) {

			// 非表示や参照先の変更時は以前の演出だけを解除し、子自身の表示設定を維持
			if (MadoEngine::Text* previousText = MyText::TryGet(upgradeText_)) {
				previousText->SetScale(previousText->GetAuthoringState().transform.scale);
			}
			upgradeTextPulseTimer_.Reset();
			upgradeTextColor_.Reset();
			upgradeText_ = {};
		}
		if (!isTextVisible) {
			return;
		}

		// 演出中の編集や再読込でも保存用Scaleを毎回基準とし、拡大率の累積を防止
		upgradeTextBaseScale_ = upgradeText->GetAuthoringState().transform.scale;
		if (!upgradeTextPulseTimer_.IsActive() || upgradeText_ != textHandle) {
			upgradeText_ = textHandle;
			upgradeTextPulseTimer_.Start(kUpgradeTextPulseDuration, true);
		}
		upgradeTextPulseTimer_.Update(deltaTime);

		// EaseOutQuadで指定サイズ範囲の変化を繰り返し、文字画像の再生成を避けるためScaleへ換算
		const float pulseSize = Easing::Lerp(kUpgradeTextPulseSize.start, kUpgradeTextPulseSize.end,
			upgradeTextPulseTimer_.GetProgress(), EaseType::EaseOutQuad);
		const float pulseScale = pulseSize / (std::max)(1.0f, upgradeText->GetFontSize());
		upgradeText->SetScale(upgradeTextBaseScale_ * pulseScale);

		// 強化選択によるゲーム停止中も実時間で色相を循環し、設定済みAlphaを維持
		Vector4 color = upgradeTextColor_.Update(deltaTime, kUpgradeTextColorChangeTime);
		color.w = upgradeText->GetColor().w;
		upgradeText->SetColor(color);
	}

	void GameHud::UpdateUpgradeControlFeedback(float deltaTime, MadoEngine::UIGroupHandle upgrade) {
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		const auto decision = groupManager.FindChildGroup(upgrade, "Decision");
		const auto reroll = groupManager.FindChildGroup(upgrade, "Reroll");
		const auto rerollButton = groupManager.FindChildGroup(reroll, "Button");
		upgradeDecisionFeedback_.Update(deltaTime, "Decision", decision);
		upgradeRerollFeedback_.Update(deltaTime, "Reroll", rerollButton);

		// 費用アイコンはボタンの拡縮から独立させ、強化画面の暗幕より手前に表示
		const MadoEngine::UIGroup* rerollGroup = groupManager.TryGet(reroll);
		if (rerollGroup && rerollGroup->IsVisibleInHierarchy()) {
			for (const auto& element : groupManager.GetElements(reroll)) {
				if (element.resolved && element.type == MadoEngine::UIGroupElementType::Sprite &&
					element.name == "RerollMoneyIcon") {
					MadoEngine::SpriteManager::GetInstance().MoveToFront(element.sprite);
					break;
				}
			}
		}
	}

	void GameHud::UpdateGameStartPresentation(float progress, float gamePlayElapsedTime) {
		const float scaleY = Easing::Lerp(kGameStartBandInitialScaleY, 0.0f, progress, EaseType::EaseInCirc);

		// Json再読込後もStartBandに接続されているSpriteを演出対象として取得
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		const auto bandGroup = groupManager.Find("StartBand", SceneType::Game);
		for (const auto& element : groupManager.GetElements(bandGroup)) {
			if (!element.resolved || element.type != MadoEngine::UIGroupElementType::Sprite) {
				continue;
			}

			// 上下端の位置を固定するため、子Spriteの高さだけを縮小
			if (Sprite* band = MySprite::TryGet(element.sprite)) {
				Vector2 scale = band->GetScale();
				scale.y = scaleY;
				band->SetScale(scale);
			}
		}

		// Playing開始後2秒間は初期位置を保ち、Pauseを除いた経過時間で目標Textをスライド
		MadoEngine::Text* goalText = MyText::TryGet(MyText::Find("TheGoalText"));
		if (goalText && goalText->GetSceneType() == SceneType::Game) {
			const float goalMoveProgress = (gamePlayElapsedTime - kGoalTextMoveDelay) / kGoalTextMoveDuration;
			Vector2 position = goalText->GetPosition();
			position.x = Easing::Lerp(kGoalTextX.start, kGoalTextX.end, goalMoveProgress, EaseType::EaseOutExpo);
			goalText->SetPosition(position);
		}

		// 帯と同じ開始待機の進行率でAlphaだけを補間し、Editor設定の文字色を維持
		MadoEngine::Text* startText = MyText::TryGet(MyText::Find("StartText"));
		if (!startText || startText->GetSceneType() != SceneType::Game) {
			return;
		}
		Vector4 color = startText->GetColor();
		color.w = Easing::Lerp(0.0f, 1.0f, progress, EaseType::Linear);
		startText->SetColor(color);

		// 縦移動の終了座標がJsonに残っていても高さを中央へ戻し、Sessionの累積時間でX方向だけ移動
		const float moveProgress = gamePlayElapsedTime / kGameStartTextMoveDuration;
		Vector2 position = startText->GetPosition();
		position.x = Easing::Lerp(kGameStartTextX.start, kGameStartTextX.end, moveProgress, EaseType::EaseInBack);
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
		MadoEngine::Text* text = MyText::TryGet(MyText::Find("MassiveEnemySpawnText"));
		if (!text || text->GetSceneType() != SceneType::Game) {
			return;
		}
		const bool hasStarted = massiveEnemySpawnTimer_.IsActive() || massiveEnemySpawnTimer_.IsFinished();
		Vector2 position = text->GetPosition();
		position.y = kMassiveEnemySpawnTextY.start;
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
				position.y = kMassiveEnemySpawnTextY.end;
			} else {
				position.y = Easing::Lerp(kMassiveEnemySpawnTextY.start, kMassiveEnemySpawnTextY.end,
					moveProgress, EaseType::EaseInBack);
			}
		}
		text->SetPosition(position);
		text->SetColor(color);
		text->SetVisible(hasStarted);
	}

	void GameHud::UpdateTexts(float deltaTime, const GameHudState& state) {
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();

		// 表示状態に関係なく現在HPを同期し、開始演出が終わった直後から最新値を表示
		MadoEngine::UIGroup* healthGroup = groupManager.TryGet(groupManager.Find("PlayerHealthUI", SceneType::Game));
		if (healthGroup) {
			if (MadoEngine::Text* healthText = healthGroup->TryGetText("PlayerHealth")) {
				const int currentHealth = static_cast<int>(state.playerStatus.currentHealth);
				const int maxHealth = static_cast<int>(state.playerStatus.maxHealth);
				healthText->SetText(std::format("HP : {} / {}", currentHealth, maxHealth));
			}
		}

		// Upgrade内のRerollへ現在の費用を同期し、同額のままの再読込にも追従
		const auto upgrade = groupManager.Find("Upgrade", SceneType::Game);
		MadoEngine::UIGroup* rerollGroup = groupManager.TryGet(groupManager.FindChildGroup(upgrade, "Reroll"));
		if (rerollGroup) {
			if (MadoEngine::Text* rerollCoinText = rerollGroup->TryGetText("RerollMoneyText")) {
				rerollCoinText->SetText(std::format("{}", state.upgradeRerollCost));
			}
		}

		// 親Group内の登録名から毎回解決し、金額が同じでも再読込後のTextへ同期
		MadoEngine::UIGroup* moneyGroup = groupManager.TryGet(groupManager.Find("MoneyUI", SceneType::Game));
		if (moneyGroup) {
			if (MadoEngine::Text* moneyText = moneyGroup->TryGetText("MoneyText")) {
				const int currentMoney = static_cast<int>(state.playerStatus.currentMoney);
				moneyText->SetText(std::format("{}", currentMoney));
			}
		}

		// 撃破数も親Group内の登録名から解決し、再生成されたTextへ最新値を同期
		MadoEngine::UIGroup* killCountGroup = groupManager.TryGet(groupManager.Find("KillCountUI", SceneType::Game));
		if (killCountGroup) {
			if (MadoEngine::Text* killCountText = killCountGroup->TryGetText("KillCountText")) {
				killCountText->SetText(std::format("{}", state.killCount));
			}
		}

		// Debug表示も親から解決し、再読込後のTextへ現在の敵数とSeedを復元
		MadoEngine::Text* fpsCountText = nullptr;
		MadoEngine::UIGroup* debugGroup = groupManager.TryGet(groupManager.Find("DebugText", SceneType::Game));
		if (debugGroup) {
			if (MadoEngine::Text* enemyCountText = debugGroup->TryGetText("EnemyCountText")) {
				enemyCountText->SetText(std::format("Enemy : {}", state.enemyCount));
			}
			if (MadoEngine::Text* seedValueText = debugGroup->TryGetText("SeedValueText")) {
				seedValueText->SetText(std::format("SEED : {}", gameSeed_));
			}
			fpsCountText = debugGroup->TryGetText("FpsCountText");
		}
		fpsMeasurementView_.Update(deltaTime, fpsCountText);
	}

}
