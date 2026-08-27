#include "EnemyEditor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace Enemy {

	namespace {
#ifdef USE_IMGUI
		constexpr std::array<Data::Type, 4> kEditableTypes = {
			Data::Type::Normal,
			Data::Type::Runner,
			Data::Type::Tank,
			Data::Type::Boss,
		};

		/// @brief Wave時間帯が他Waveと重複しているか判定
		/// @param waves 全Wave設定
		/// @param targetIndex 判定対象のWave位置
		/// @return 他Waveと時間帯が重複する場合はtrue
		bool HasOverlappingWave(const std::vector<WaveSettings>& waves, std::size_t targetIndex) {
			if (targetIndex >= waves.size()) {
				return false;
			}

			const WaveSettings& target = waves[targetIndex];
			for (std::size_t index = 0; index < waves.size(); ++index) {
				if (index == targetIndex) {
					continue;
				}

				const WaveSettings& other = waves[index];
				if (target.startTime < other.endTime && target.endTime > other.startTime) {
					return true;
				}
			}

			return false;
		}

		/// @brief Wave設定時間内の予定Enemy出現数を計算
		/// @param wave 算出対象のWave設定
		/// @return 最大生存数と配置成否を考慮しない予定出現数
		std::uint64_t CalculatePlannedEnemySpawnCount(const WaveSettings& wave) {
			if (!std::isfinite(wave.startTime) || !std::isfinite(wave.endTime) ||
				!std::isfinite(wave.spawnInterval) || wave.endTime <= wave.startTime ||
				wave.spawnCount == 0) {
				return 0;
			}

			const long double duration = static_cast<long double>(wave.endTime) - static_cast<long double>(wave.startTime);
			const long double spawnInterval = (std::max)(0.01L, static_cast<long double>(wave.spawnInterval));
			const long double spawnBatchCount = std::floor(duration / spawnInterval);
			const long double enemySpawnCount = spawnBatchCount * static_cast<long double>(wave.spawnCount);

			// 極端なJson設定でも表示用整数への変換で桁あふれしないよう上限へ飽和
			const long double maximumCount = static_cast<long double>((std::numeric_limits<std::uint64_t>::max)());
			if (enemySpawnCount >= maximumCount) {
				return (std::numeric_limits<std::uint64_t>::max)();
			}

			return static_cast<std::uint64_t>(enemySpawnCount);
		}
#endif // USE_IMGUI
	} // namespace

	void Editor::Initialize(Spawner* spawner, Manager* manager) {
		spawner_ = spawner;
		manager_ = manager;
		selectedType_ = Data::Type::Normal;
		immediateSpawnCount_ = 1;
	}

	void Editor::DrawImGui() {
#ifdef USE_IMGUI
		Settings& settings = Settings::GetInstance();
		bool settingsChanged = false;

		ImGui::SetNextWindowSize(ImVec2(680.0f, 760.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("EnemyEditor")) {
			ImGui::End();
			return;
		}

		if (ImGui::Button("保存")) {
			settings.Normalize();
			settings.Save();
		}
		ImGui::SameLine();
		if (ImGui::Button("再読込")) {
			settings.LoadOrCreate();
		}
		ImGui::SameLine();
		if (ImGui::Button("初期値へ戻す")) {
			settings.ResetToDefaults();
			settingsChanged = true;
		}

		ImGui::Separator();
		if (ImGui::BeginTabBar("EnemyEditorTabs")) {
			if (ImGui::BeginTabItem("EnemyStatus")) {
				settingsChanged |= DrawEnemyStatus();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("EnemySpawner")) {
				settingsChanged |= DrawEnemySpawner();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (settingsChanged) {

			// Editor入力値を次Frameの生成処理へ渡す前に実行可能な範囲へ統一
			settings.Normalize();
		}

		ImGui::End();
#endif // USE_IMGUI
	}

	bool Editor::DrawEnemyStatus() {
#ifdef USE_IMGUI
		Settings& settings = Settings::GetInstance();
		bool settingsChanged = false;

		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::BeginCombo("Enemy Type", EnemyTypeToString(selectedType_))) {
			for (const Data::Type type : kEditableTypes) {
				const bool isSelected = type == selectedType_;
				if (ImGui::Selectable(EnemyTypeToString(type), isSelected)) {
					selectedType_ = type;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		TypeSettings& typeSettings = settings.EditTypeSettings(selectedType_);
		ImGui::SeparatorText("初期ステータス");
		settingsChanged |= ImGui::DragFloat(
			"HP", &typeSettings.status.currentHealth, 1.0f, 0.0f, 999999.0f, "%.1f");
		settingsChanged |= ImGui::DragFloat(
			"攻撃力", &typeSettings.status.power, 0.1f, 0.0f, 999999.0f, "%.1f");
		settingsChanged |= ImGui::DragFloat(
			"移動速度", &typeSettings.status.moveSpeed, 0.05f, 0.0f, 9999.0f, "%.2f");

		ImGui::SeparatorText("外形");
		settingsChanged |= ImGui::DragFloat(
			"Scale係数", &typeSettings.scaleMultiplier, 0.01f, 0.01f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat(
			"移動Collider半径", &typeSettings.movementColliderRadius, 0.01f, 0.01f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat3(
			"Hitbox Min", &typeSettings.hitboxMin.x, 0.01f, -100.0f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat3(
			"Hitbox Max", &typeSettings.hitboxMax.x, 0.01f, -100.0f, 100.0f, "%.2f");
		ImGui::TextDisabled("変更内容は新しく生成されるEnemyから適用");

		ImGui::SeparatorText("表示");
		settingsChanged |= ImGui::ColorEdit4(
			"Model色", &typeSettings.color.x, ImGuiColorEditFlags_AlphaBar);
		ImGui::TextDisabled("色変更は生成済みEnemyにも即時反映");

		ImGui::SeparatorText("Elite属性");
		EliteSettings& eliteSettings = settings.EditEliteSettings();
		settingsChanged |= ImGui::DragFloat(
			"Elite HP倍率", &eliteSettings.healthMultiplier, 0.05f, 0.0f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat(
			"Elite 攻撃力倍率", &eliteSettings.powerMultiplier, 0.05f, 0.0f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat(
			"Elite 体格倍率", &eliteSettings.bodyScaleMultiplier, 0.01f, 0.01f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat(
			"Marker高さ", &eliteSettings.markerHeightOffset, 0.01f, 0.0f, 100.0f, "%.2f");
		settingsChanged |= ImGui::DragFloat3(
			"Marker Scale", &eliteSettings.markerScale.x, 0.01f, 0.01f, 100.0f, "%.2f");

		return settingsChanged;
#else
		return false;
#endif // USE_IMGUI
	}

	bool Editor::DrawEnemySpawner() {
#ifdef USE_IMGUI
		Settings& settings = Settings::GetInstance();
		bool settingsChanged = false;

		ImGui::SeparatorText("実行状態");
		ImGui::Text("Enemy数 : %zu", manager_ ? manager_->GetEnemyCount() : 0);
		ImGui::Text("経過時間 : %.1f秒", spawner_ ? spawner_->GetElapsedTime() : 0.0f);

		std::size_t activeWaveIndex = 0;
		const std::vector<WaveSettings>& constWaves = settings.GetWaves();
		const bool isBonusWaveActive = spawner_ && spawner_->IsBonusWaveActive();
		if (isBonusWaveActive) {
			ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.2f, 1.0f), "処理中Wave : Bonus Wave");
			ImGui::Text("次のEliteまで : %u体", spawner_->GetRemainingSpawnCountUntilElite());
		} else if (spawner_ && spawner_->TryGetActiveWaveIndex(activeWaveIndex) && activeWaveIndex < constWaves.size()) {
			ImGui::Text("処理中Wave : %s", constWaves[activeWaveIndex].name.c_str());
			ImGui::Text("次のEliteまで : %u体", spawner_->GetRemainingSpawnCountUntilElite());
		} else {
			ImGui::TextDisabled("処理中Wave : なし");
			ImGui::TextDisabled("次のEliteまで : -");
		}

		if (spawner_) {
			bool isActive = spawner_->IsActive();
			if (ImGui::Checkbox("自動生成", &isActive)) {
				spawner_->SetActive(isActive);
			}
			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragScalar("即時生成数", ImGuiDataType_U32, &immediateSpawnCount_, 1.0f);
			immediateSpawnCount_ = std::max<std::uint32_t>(1, immediateSpawnCount_);
			ImGui::SameLine();
			if (ImGui::Button("現在Waveで生成")) {
				spawner_->SpawnImmediately(immediateSpawnCount_);
			}
		}

		ImGui::SeparatorText("共通生成設定");
		std::uint64_t maxAliveEnemies = static_cast<std::uint64_t>(settings.GetMaxAliveEnemies());
		if (ImGui::DragScalar("最大生存数", ImGuiDataType_U64, &maxAliveEnemies, 1.0f)) {
			settings.SetMaxAliveEnemies(static_cast<std::size_t>(maxAliveEnemies));
			settingsChanged = true;
		}

		ImGui::SeparatorText("Wave一覧");
		if (ImGui::Button("Waveを追加")) {
			settings.AddWave();
			settingsChanged = true;
		}
		ImGui::TextDisabled("有効時間は開始以上・終了未満、重複時は開始時間が遅いWaveを優先");
		ImGui::TextDisabled("予定出現数は最大生存数による抑制と配置失敗を含まない生成要求数");
		std::vector<WaveSettings>& waves = settings.EditWaves();
		std::size_t removeIndex = static_cast<std::size_t>(-1);
		for (std::size_t index = 0; index < waves.size(); ++index) {
			bool removeRequested = false;
			settingsChanged |= DrawWave(index, waves[index], removeRequested);
			if (removeRequested) {
				removeIndex = index;
			}
		}

		if (removeIndex != static_cast<std::size_t>(-1)) {
			settingsChanged |= settings.RemoveWave(removeIndex);
		}

		ImGui::SeparatorText("Bonus Wave");
		if (spawner_) {
			ImGui::TextDisabled("制限時間 %.1f秒以降に継続適用", spawner_->GetTimeLimit());
		} else {
			ImGui::TextDisabled("制限時間以降に継続適用");
		}
		if (isBonusWaveActive) {
			ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.2f, 1.0f), "現在実行中");
		}
		ImGui::PushID("BonusWave");
		if (ImGui::CollapsingHeader("時間切れ以降の生成設定", ImGuiTreeNodeFlags_DefaultOpen)) {
			settingsChanged |= DrawWaveSpawnSettings(settings.EditBonusWaveSettings());
		}
		ImGui::PopID();
		return settingsChanged;
#else
		return false;
#endif // USE_IMGUI
	}

	bool Editor::DrawWave(std::size_t index, WaveSettings& wave, bool& outRemoveRequested) {
		outRemoveRequested = false;
#ifdef USE_IMGUI
		bool settingsChanged = false;
		ImGui::PushID(static_cast<int>(index));
		const std::uint64_t plannedEnemySpawnCount = CalculatePlannedEnemySpawnCount(wave);
		const std::string headerLabel = wave.name + "  [予定出現数 " +
			std::to_string(plannedEnemySpawnCount) + "体]##WaveHeader";
		bool isExpanded = false;

		// 展開状態に依存せず削除操作へアクセスできる見出し行を構築
		if (ImGui::BeginTable("WaveHeaderRow", 2, ImGuiTableFlags_SizingStretchProp)) {
			const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::TableSetupColumn("Wave", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, deleteButtonWidth);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			isExpanded = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("削除")) {
				outRemoveRequested = true;
			}
			ImGui::EndTable();
		}

		if (isExpanded) {
			char nameBuffer[128] = {};
			strncpy_s(nameBuffer, sizeof(nameBuffer), wave.name.c_str(), _TRUNCATE);
			if (ImGui::InputText("名前", nameBuffer, sizeof(nameBuffer))) {
				wave.name = nameBuffer;
				settingsChanged = true;
			}

			settingsChanged |= ImGui::DragFloat(
				"開始時間", &wave.startTime, 0.1f, 0.0f, 86400.0f, "%.1f秒");
			settingsChanged |= ImGui::DragFloat(
				"終了時間", &wave.endTime, 0.1f, 0.0f, 86400.0f, "%.1f秒");
			if (HasOverlappingWave(Settings::GetInstance().GetWaves(), index)) {
				ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "他Waveと時間帯が重複");
			}

			settingsChanged |= DrawWaveSpawnSettings(wave);
		}
		ImGui::PopID();
		return settingsChanged;
#else
		(void)index;
		(void)wave;
		return false;
#endif // USE_IMGUI
	}

	bool Editor::DrawWaveSpawnSettings(WaveSpawnSettings& waveSettings) {
#ifdef USE_IMGUI
		bool settingsChanged = false;
		ImGui::SeparatorText("生成");
		settingsChanged |= ImGui::DragFloat(
			"生成間隔", &waveSettings.spawnInterval, 0.01f, 0.01f, 600.0f, "%.2f秒");
		settingsChanged |= ImGui::DragScalar(
			"生成数", ImGuiDataType_U32, &waveSettings.spawnCount, 1.0f);
		settingsChanged |= ImGui::DragScalar(
			"Elite生成周期", ImGuiDataType_U32, &waveSettings.eliteSpawnInterval, 1.0f);
		settingsChanged |= ImGui::DragFloat(
			"最小生成半径", &waveSettings.minSpawnRadius, 0.1f, 0.0f, 10000.0f, "%.1f");
		settingsChanged |= ImGui::DragFloat(
			"最大生成半径", &waveSettings.maxSpawnRadius, 0.1f, 0.0f, 10000.0f, "%.1f");

		ImGui::SeparatorText("種類別生成率");
		float normalPercent = waveSettings.normalSpawnRate * 100.0f;
		float runnerPercent = waveSettings.runnerSpawnRate * 100.0f;
		float tankPercent = waveSettings.tankSpawnRate * 100.0f;
		if (ImGui::DragFloat("Normal", &normalPercent, 1.0f, 0.0f, 10000.0f, "%.1f%%")) {
			waveSettings.normalSpawnRate = normalPercent / 100.0f;
			settingsChanged = true;
		}
		if (ImGui::DragFloat("Runner", &runnerPercent, 1.0f, 0.0f, 10000.0f, "%.1f%%")) {
			waveSettings.runnerSpawnRate = runnerPercent / 100.0f;
			settingsChanged = true;
		}
		if (ImGui::DragFloat("Tank", &tankPercent, 1.0f, 0.0f, 10000.0f, "%.1f%%")) {
			waveSettings.tankSpawnRate = tankPercent / 100.0f;
			settingsChanged = true;
		}
		const float totalRate =
			waveSettings.normalSpawnRate + waveSettings.runnerSpawnRate + waveSettings.tankSpawnRate;
		if (totalRate > 0.0f) {
			ImGui::TextDisabled(
				"実効生成率  Normal %.1f%% / Runner %.1f%% / Tank %.1f%%",
				waveSettings.normalSpawnRate / totalRate * 100.0f,
				waveSettings.runnerSpawnRate / totalRate * 100.0f,
				waveSettings.tankSpawnRate / totalRate * 100.0f);
		}
		ImGui::TextDisabled("入力値は比率として自動正規化");

		ImGui::SeparatorText("時間経過強化");
		settingsChanged |= ImGui::DragFloat(
			"HP・攻撃力強化率（毎分）",
			&waveSettings.healthPowerGrowthRatePerMinute,
			0.01f,
			0.0f,
			100.0f,
			"%.2f");
		settingsChanged |= ImGui::DragFloat(
			"移動速度強化率（毎分）",
			&waveSettings.moveSpeedGrowthRatePerMinute,
			0.01f,
			0.0f,
			100.0f,
			"%.2f");
		return settingsChanged;
#else
		(void)waveSettings;
		return false;
#endif // USE_IMGUI
	}

} // namespace Enemy
