#include "WeaponStatusEditor.h"
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
		/// @brief Json保存に使う名前へ変換します。
		/// @param name 入力された名前です。
		/// @return ファイル名として使用できる名前です。
		std::string SanitizeJsonName(const char* name) {
			std::string sanitizedName = name;
			if (sanitizedName.empty()) {
				return "WeaponStatus";
			}

			const char* invalidCharacters = "\\/:*?\"<>|";
			for (char& character : sanitizedName) {
				if (std::strchr(invalidCharacters, character)) {
					character = '_';
				}
			}

			return sanitizedName;
		}

#ifdef USE_IMGUI
		/// @brief アップグレード値のImGuiテーブル行を描画します。
		/// @param label 表示名です。
		/// @param value 編集するアップグレード値です。
		void DrawUpgradeValueTableRow(const char* label, UpgradeValue& value) {
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
#endif // USE_IMGUI
	}

	bool StatusEditor::SaveToJson() const {
		const std::string filePath = GetJsonFilePath();
		nlohmann::json json;
		json["name"] = SanitizeJsonName(statusName_);
		json["upgradeStatus"] = UpgradeStatusToJson(editingStatus_);

		const bool isSaved = MadoEngine::Json::JsonFile::Save(filePath, json, 4, true);
		if (isSaved) {
			Logger::Output("[Assets] 武器初期ステータスをJsonへ保存しました: " + filePath, Logger::Level::Assets);
		}

		return isSaved;
	}

	bool StatusEditor::LoadOrCreateJson() {
		const std::string filePath = GetJsonFilePath();
		if (!MadoEngine::Json::JsonFile::Exists(filePath)) {
			Logger::Output("[Assets] 武器初期ステータスJsonが存在しないため作成します: " + filePath, Logger::Level::Assets);
			return SaveToJson();
		}

		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
			return false;
		}

		const nlohmann::json* statusJson = &json;
		if (json.is_object() && json.contains("upgradeStatus")) {
			statusJson = &json.at("upgradeStatus");
		}

		if (!UpgradeStatusFromJson(*statusJson, editingStatus_)) {
			Logger::Output("[Assets] 武器初期ステータスJsonに不正な値があります: " + filePath, Logger::Level::Error);
			return false;
		}

		Logger::Output("[Assets] 武器初期ステータスをJsonから読み込みました: " + filePath, Logger::Level::Assets);
		return true;
	}

	std::string StatusEditor::GetJsonFilePath() const {
		return "Assets/Json/Weapon/" + SanitizeJsonName(statusName_) + ".json";
	}

	void StatusEditor::DrawImGui() {

#ifdef USE_IMGUI

		ImGui::Begin("武器初期ステータス");

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

		constexpr ImGuiTableFlags tableFlags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("WeaponInitialStatusTable", 5, tableFlags, ImVec2(-1.0f, 0.0f))) {
			ImGui::TableSetupColumn("ステータス", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("初期値", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("固定加算値", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("上昇幅", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("選択肢に表示", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableHeadersRow();

			DrawUpgradeValueTableRow("ダメージ量", editingStatus_.damage);
			DrawUpgradeValueTableRow("最大射撃数", editingStatus_.shotMaxCount);
			DrawUpgradeValueTableRow("射撃クールダウン", editingStatus_.shotCooldown);
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

		ImGui::End();

#endif // USE_IMGUI
	}
}
