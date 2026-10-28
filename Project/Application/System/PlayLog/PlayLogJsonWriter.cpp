#include "PlayLogJsonWriter.h"
#include "Utility/Logger/Logger.h"
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace System {

	namespace {

		/// @brief 秒数を小数第1位へ四捨五入
		/// @param seconds 保存対象の秒数
		/// @return 小数第1位までに丸めた秒数
		double RoundTimeSeconds(double seconds) {
			return std::round(seconds * 10.0) / 10.0;
		}

		/// @brief 累計ダメージの小数部を切り捨て
		/// @param damage 保存対象の累計ダメージ
		/// @return 小数部を除いた非負の整数ダメージ
		std::uint64_t TruncateDamage(double damage) {

			// 不正値と整数型の範囲外を変換前に除外して未定義動作を防止
			if (!std::isfinite(damage) || damage <= 0.0) {
				return 0;
			}
			const auto maxDamage = (std::numeric_limits<std::uint64_t>::max)();
			if (damage >= static_cast<double>(maxDamage)) {
				return maxDamage;
			}
			return static_cast<std::uint64_t>(damage);
		}

		/// @brief 終了理由を保存用の日本語へ変換
		/// @param reason プレイの終了理由
		/// @return 終了理由の日本語表記
		const char* GetEndReasonName(PlayEndReason reason) {
			switch (reason) {
			case PlayEndReason::Death: return "プレイヤーの死亡";
			case PlayEndReason::Restart: return "リスタート";
			case PlayEndReason::ReturnToTitle: return "タイトルに戻る";
			case PlayEndReason::ApplicationExit: return "アプリ終了";
			case PlayEndReason::SceneChange: return "その他のシーン遷移";
			}
			return "不明";
		}

		/// @brief 発生源別の戦績をJSONへ変換
		/// @param stats 変換対象の戦績
		/// @return 撃破数と実ダメージ量を含むJSON
		nlohmann::ordered_json CombatStatsToJson(const PlayCombatStats& stats) {
			return { { "撃破数", stats.killCount }, { "与ダメージ総量", TruncateDamage(stats.damage) } };
		}

	}

	PlayLogJsonWriter::PlayLogJsonWriter(std::filesystem::path directory)
		: directory_(std::move(directory)) {}

	nlohmann::ordered_json PlayLogJsonWriter::ToJson(const PlayLogData& data) const {
		nlohmann::ordered_json root;
		root["ゲームシード"] = data.seed;
		root["終了理由"] = GetEndReasonName(data.endReason);

		// 集計精度を維持し、保存用の値へ変換する段階だけで丸めを適用
		root["経過時間秒"] = RoundTimeSeconds(data.elapsedSeconds);
		root["スコア"] = data.score;
		root["撃破総数"] = data.totalCombat.killCount;
		root["与ダメージ総量"] = TruncateDamage(data.totalCombat.damage);
		root["獲得金額総量"] = data.earnedMoney;
		root["終了時所持金"] = data.endingMoney;

		// 空きSlotを架空の武器として保存せず、取得履歴をSlot番号付きの配列で保持
		root["武器"] = nlohmann::ordered_json::array();
		for (const PlayWeaponRecord& weapon : data.weapons) {
			root["武器"].push_back({
				{ "スロット番号", weapon.slotIndex + 1 },
				{ "武器名", weapon.name },
				{ "レベル", weapon.level },
				{ "撃破数", weapon.combat.killCount },
				{ "与ダメージ総量", TruncateDamage(weapon.combat.damage) },
				{ "取得時刻秒", RoundTimeSeconds(weapon.acquiredAtSeconds) },
				{ "装備時間秒", RoundTimeSeconds(weapon.equippedSeconds) },
				{ "終了時に装備", weapon.equippedAtEnd },
			});
		}
		root["状態異常"] = {
			{ "毒", CombatStatsToJson(data.poison) },
			{ "火傷", CombatStatsToJson(data.burn) },
		};
		root["リロール"] = { { "成功回数", data.rerollCount }, { "支払総額", data.rerollSpentMoney } };
		root["Chest"] = {
			{ "成功回数", data.paidChestCount + data.freeChestCount },
			{ "有料Chest数", data.paidChestCount },
			{ "無料Chest数", data.freeChestCount },
			{ "支払総額", data.chestSpentMoney },
		};
		root["レリック"] = nlohmann::ordered_json::array();
		for (const PlayRelicRecord& relic : data.relics) {
			root["レリック"].push_back({
				{ "レリック名", relic.name }, { "所持数", relic.count }, { "レアリティ", GetRarityDisplayName(relic.rarity) },
			});
		}
		root["Jar"] = {
			{ "成功回数", data.expJarCount + data.moneyJarCount },
			{ "経験値ツボ数", data.expJarCount },
			{ "お金ツボ数", data.moneyJarCount },
		};
		nlohmann::ordered_json karmaTimes = nlohmann::ordered_json::array();
		for (const double seconds : data.karmaInteractionSeconds) {
			karmaTimes.push_back(RoundTimeSeconds(seconds));
		}
		root["Karma"] = {
			{ "成功回数", data.karmaInteractionSeconds.size() },
			{ "インタラクト時刻秒", std::move(karmaTimes) },
		};
		root["終了時難易度"] = data.endingDifficulty;
		return root;
	}

	std::filesystem::path PlayLogJsonWriter::CreateFilePath(std::uint32_t seed) const {
		const auto now = std::chrono::system_clock::now();
		const std::time_t time = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
		if (localtime_s(&localTime, &time) != 0) {
			throw std::runtime_error("保存日時の取得失敗");
		}
		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
		std::ostringstream name;
		name << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << '_'
			<< std::setfill('0') << std::setw(3) << milliseconds << '_' << seed;
		const std::string baseName = name.str();
		std::filesystem::path path = directory_ / (baseName + ".json");
		std::filesystem::path temporaryPath = path;
		temporaryPath += ".tmp";
		std::uint64_t suffix = 0;

		// 短時間の再開や同一Seedの繰り返しでも既存記録と未完了ファイルを上書きしない連番
		while (std::filesystem::exists(path) || std::filesystem::exists(temporaryPath)) {
			path = directory_ / (baseName + '_' + std::to_string(++suffix) + ".json");
			temporaryPath = path;
			temporaryPath += ".tmp";
		}
		return path;
	}

	bool PlayLogJsonWriter::Save(const PlayLogData& data) const {
		std::filesystem::path temporaryPath;
		try {
			std::filesystem::create_directories(directory_);
			const std::filesystem::path path = CreateFilePath(data.seed);
			temporaryPath = path;
			temporaryPath += ".tmp";

			// 完全なUTF-8 JSONを書き終えた場合だけ正式名へ変更し、不完全な記録の混入を防止
			std::ofstream file(temporaryPath, std::ios::binary | std::ios::out);
			if (!file.is_open()) {
				throw std::runtime_error("保存ファイルの作成失敗");
			}
			file << ToJson(data).dump(2) << '\n';
			file.flush();
			if (!file) {
				throw std::runtime_error("保存ファイルの書き込み失敗");
			}
			file.close();
			if (file.fail()) {
				throw std::runtime_error("保存ファイルのクローズ失敗");
			}
			std::filesystem::rename(temporaryPath, path);
			return true;
		} catch (const std::exception& exception) {

			// 保存失敗をGame終了処理へ伝播させず、今回作成した一時ファイルだけを除去
			if (!temporaryPath.empty()) {
				std::error_code error;
				std::filesystem::remove(temporaryPath, error);
			}
			Logger::Output("プレイLogの保存に失敗しました: " + std::string(exception.what()), Logger::Level::Error);
			return false;
		}
	}

}
