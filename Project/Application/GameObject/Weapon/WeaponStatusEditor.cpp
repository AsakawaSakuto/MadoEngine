#include "WeaponStatusEditor.h"
#include "WeaponStatusJson.h"
#include "Projectile/ProjectileStatus.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <cstring>
#include <nlohmann/json.hpp>

namespace Weapon {

	namespace {
		/// @brief Json保存に使う名前へ変換
		/// @param name 入力された名前
		/// @return ファイル名として使用できる名前
		std::string SanitizeJsonName(const char* name) {
			std::string sanitizedName = name;
			if (sanitizedName.empty()) {
				return "WeaponStatus";
			}

			const char* invalidCharacters = "\\/:*?\"<>|";

			// Windowsで使用できないFile名文字を安全な区切りへ置換
			for (char& character : sanitizedName) {
				if (std::strchr(invalidCharacters, character)) {
					character = '_';
				}
			}

			return sanitizedName;
		}

#ifdef USE_IMGUI
		constexpr ImGuiTableFlags kUpgradeTableFlags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_SizingStretchProp;

		/// @brief アップグレード値編集用のImGuiテーブルを開始
		/// @param id テーブルの識別名
		/// @return テーブルを描画できる場合はtrue
		bool BeginUpgradeValueTable(const char* id) {
			if (!ImGui::BeginTable(id, 5, kUpgradeTableFlags, ImVec2(-1.0f, 0.0f))) {
				return false;
			}

			ImGui::TableSetupColumn("ステータス", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("初期値", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("固定加算値", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("上昇幅", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("選択肢に表示", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableHeadersRow();
			return true;
		}

		/// @brief アップグレード値のImGuiテーブル行を描画
		/// @param label 表示名
		/// @param value 編集するアップグレード値
		void DrawUpgradeValueTableRow(const char* label, UpgradeValue& value) {

			// 一つの強化値を初期値、固定加算、Rarity加算、候補化の列へ展開
			ImGui::PushID(label);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(label);

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragFloat("##InitialValue", &value.value, 0.01f, -999999.0f, 999999.0f, "%.3f");

			ImGui::TableSetColumnIndex(2);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragFloat("##FixedAddValue", &value.fixedAddValue, 0.01f, -999999.0f, 999999.0f, "%.3f");

			ImGui::TableSetColumnIndex(3);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragFloat("##RarityAddValue", &value.rarityAddValue, 0.01f, -999999.0f, 999999.0f, "%.3f");

			ImGui::TableSetColumnIndex(4);
			ImGui::Checkbox("##IsSelected", &value.isSelected);
			ImGui::PopID();
		}

		/// @brief 継続ダメージ型状態異常の編集項目を描画
		/// @param label 状態異常の表示名
		/// @param status 編集対象の状態異常設定
		void DrawDamageStatusEffectEditor(
			const char* label,
			std::optional<DamageStatusEffectUpgradeStatus>& status) {
			ImGui::PushID(label);
			bool isEnabled = status.has_value();
			if (ImGui::Checkbox("##Enabled", &isEnabled)) {

				// 有効化時だけ既定値を生成し、無効化時はJson出力対象から除外
				if (isEnabled) {
					status.emplace();
				} else {
					status.reset();
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(label);

			if (status) {
				ImGui::Indent();
				if (BeginUpgradeValueTable("StatusEffectUpgradeTable")) {
					DrawUpgradeValueTableRow("付与率(%)", status->applyChance);
					DrawUpgradeValueTableRow("1Tickダメージ", status->damagePerTick);
					DrawUpgradeValueTableRow("持続時間(秒)", status->duration);
					ImGui::EndTable();
				}
				ImGui::Unindent();
			}
			ImGui::PopID();
		}

		/// @brief 凍結状態異常の編集項目を描画
		/// @param status 編集対象の状態異常設定
		void DrawFrozenStatusEffectEditor(std::optional<FrozenStatusEffectUpgradeStatus>& status) {
			ImGui::PushID("Frozen");
			bool isEnabled = status.has_value();
			if (ImGui::Checkbox("##Enabled", &isEnabled)) {

				// 有効化時だけ既定値を生成し、無効化時はJson出力対象から除外
				if (isEnabled) {
					status.emplace();
				} else {
					status.reset();
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("凍結");

			if (status) {
				ImGui::Indent();
				if (BeginUpgradeValueTable("StatusEffectUpgradeTable")) {
					DrawUpgradeValueTableRow("付与率(%)", status->applyChance);
					DrawUpgradeValueTableRow("減速率(%)", status->slowRate);
					DrawUpgradeValueTableRow("持続時間(秒)", status->duration);
					ImGui::EndTable();
				}
				ImGui::Unindent();
			}
			ImGui::PopID();
		}
#endif // USE_IMGUI
	}

	bool StatusEditor::SaveToJson() const {
		const std::string filePath = GetJsonFilePath();
		if (!IsValidStatusEffectUpgradeStatus(editingStatusEffects_)) {
			Logger::Output("[Assets] 状態異常ステータスに不正な値があります: " + filePath, Logger::Level::Error);
			return false;
		}

		nlohmann::json json;

		// Editor用の名前と実行時ステータスを一つのDocumentへ保存
		json["name"] = SanitizeJsonName(statusName_);
		json["upgradeStatus"] = UpgradeStatusToJson(editingStatus_);
		if (HasStatusEffect(editingStatusEffects_)) {
			json["statusEffects"] = StatusEffectUpgradeStatusToJson(editingStatusEffects_);
		}

		const bool isSaved = MadoEngine::Json::JsonFile::Save(filePath, json, 4, true);
		if (isSaved) {
			Logger::Output("[Assets] 武器初期ステータスをJsonへ保存しました: " + filePath, Logger::Level::Assets);
		}

		return isSaved;
	}

	bool StatusEditor::LoadOrCreateJson() {
		const std::string filePath = GetJsonFilePath();
		if (!MadoEngine::Json::JsonFile::Exists(filePath)) {

			// 初回選択時は現在の編集値を初期設定ファイルとして生成
			Logger::Output("[Assets] 武器初期ステータスJsonが存在しないため作成します: " + filePath, Logger::Level::Assets);
			return SaveToJson();
		}

		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}

		const nlohmann::json* statusJson = &json;
		if (json.is_object() && json.contains("upgradeStatus")) {

			// Editor保存形式とステータス単体形式の両方へ対応
			statusJson = &json.at("upgradeStatus");
		}

		UpgradeStatus loadedStatus = editingStatus_;
		if (!UpgradeStatusFromJson(*statusJson, loadedStatus)) {
			Logger::Output("[Assets] 武器初期ステータスJsonに不正な値があります: " + filePath, Logger::Level::Error);
			return false;
		}

		StatusEffectUpgradeStatus loadedStatusEffects;
		if (json.is_object() && json.contains("statusEffects") &&
			!StatusEffectUpgradeStatusFromJson(json.at("statusEffects"), loadedStatusEffects)) {
			Logger::Output("[Assets] 状態異常ステータスJsonに不正な値があります: " + filePath, Logger::Level::Error);
			return false;
		}

		editingStatus_ = loadedStatus;
		editingStatusEffects_ = loadedStatusEffects;

		Logger::Output("[Assets] 武器初期ステータスをJsonから読み込みました: " + filePath, Logger::Level::Assets);
		return true;
	}

	std::string StatusEditor::GetJsonFilePath() const {
		return "Assets/Json/Weapon/" + SanitizeJsonName(statusName_) + ".json";
	}

	void StatusEditor::DrawImGui() {

#ifdef USE_IMGUI

		ImGui::Begin("武器初期ステータス");

		// Jsonファイル名をPlayer向けの武器表示名へ変換
		const char* selectedWeaponDisplayName = statusName_;
		for (const Projectile::Type weaponType : Projectile::kPlayableWeaponTypes) {
			if (Projectile::ProjectileTypeToJsonFileName(weaponType) == statusName_) {
				selectedWeaponDisplayName = Projectile::ProjectileTypeToDisplayName(weaponType);
				break;
			}
		}

		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::BeginCombo("調整する武器", selectedWeaponDisplayName)) {
			for (const Projectile::Type weaponType : Projectile::kPlayableWeaponTypes) {
				const std::string jsonName = Projectile::ProjectileTypeToJsonFileName(weaponType);
				const bool isSelected = jsonName == statusName_;
				if (ImGui::Selectable(Projectile::ProjectileTypeToDisplayName(weaponType), isSelected)) {
					strncpy_s(statusName_, sizeof(statusName_), jsonName.c_str(), _TRUNCATE);
					LoadOrCreateJson();
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("保存")) {
			SaveToJson();
		}
		ImGui::SameLine();
		if (ImGui::Button("読込")) {
			LoadOrCreateJson();
		}

		ImGui::Separator();

		// 全武器で共通する初期値とレアリティ加算設定を一覧編集
		if (BeginUpgradeValueTable("WeaponInitialStatusTable")) {
			DrawUpgradeValueTableRow("ダメージ量", editingStatus_.damage);
			DrawUpgradeValueTableRow("最大射撃数", editingStatus_.shotMaxCount);
			DrawUpgradeValueTableRow("射撃間隔", editingStatus_.shotIntervalTime);
			DrawUpgradeValueTableRow("射撃クールダウン短縮率(%)", editingStatus_.shotCooldown);
			DrawUpgradeValueTableRow("クリティカル率", editingStatus_.criticalChance);
			DrawUpgradeValueTableRow("クリティカル倍率", editingStatus_.criticalDamage);
			DrawUpgradeValueTableRow("サイズ", editingStatus_.size);
			DrawUpgradeValueTableRow("跳弾回数", editingStatus_.bounceCount);
			DrawUpgradeValueTableRow("貫通回数", editingStatus_.penetrationCount);
			DrawUpgradeValueTableRow("ノックバック力", editingStatus_.knockbackPower);
			DrawUpgradeValueTableRow("弾の寿命", editingStatus_.lifeTime);
			DrawUpgradeValueTableRow("弾の速度", editingStatus_.speed);

			ImGui::EndTable();
		}

		ImGui::SeparatorText("状態異常");

		// 使用する状態異常だけを有効化して武器Jsonへ個別保存
		DrawDamageStatusEffectEditor("火傷", editingStatusEffects_.burn);
		DrawDamageStatusEffectEditor("毒", editingStatusEffects_.poison);
		DrawFrozenStatusEffectEditor(editingStatusEffects_.frozen);

		ImGui::End();

#endif // USE_IMGUI
	}
}
