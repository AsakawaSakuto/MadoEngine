#include "WeaponStatusEditor.h"
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
		/// @brief アップグレード値のImGui編集欄を描画します。
		/// @param label 表示名です。
		/// @param value 編集するアップグレード値です。
		void DrawUpgradeValueImGui(const char* label, UpgradeValue& value) {
			ImGui::PushID(label);
			ImGui::TextUnformatted(label);
			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("現在の初期値", &value.value, 0.01f, -999999.0f, 999999.0f, "%.3f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("固定加算値", &value.fixedAddValue, 0.01f, -999999.0f, 999999.0f, "%.3f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("レアリティ上昇幅", &value.rarityAddValue, 0.01f, -999999.0f, 999999.0f, "%.3f");
			ImGui::SameLine();
			ImGui::Checkbox("選択肢に表示", &value.isSelected);
			ImGui::Separator();
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

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputText("名前", statusName_, sizeof(statusName_));
		ImGui::SameLine();
		if (ImGui::Button("保存")) {
			SaveToJson();
		}
		ImGui::SameLine();
		if (ImGui::Button("読込")) {
			LoadOrCreateJson();
		}

		ImGui::Separator();
		DrawUpgradeValueImGui("ダメージ量", editingStatus_.damage);
		DrawUpgradeValueImGui("最大射撃数", editingStatus_.shotMaxCount);
		DrawUpgradeValueImGui("射撃クールダウン", editingStatus_.shotCooldown);
		DrawUpgradeValueImGui("クリティカル率", editingStatus_.criticalChance);
		DrawUpgradeValueImGui("クリティカル倍率", editingStatus_.criticalDamage);
		DrawUpgradeValueImGui("サイズ", editingStatus_.size);
		DrawUpgradeValueImGui("跳弾回数", editingStatus_.bounceCount);
		DrawUpgradeValueImGui("貫通回数", editingStatus_.penetrationCount);
		DrawUpgradeValueImGui("ノックバック力", editingStatus_.knockbackPower);
		DrawUpgradeValueImGui("弾の寿命", editingStatus_.lifeTime);
		DrawUpgradeValueImGui("弾の速度", editingStatus_.speed);

		ImGui::End();

#endif // USE_IMGUI
	}
}
