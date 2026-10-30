#include "ScoreRanking.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <exception>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace System {

	ScoreRanking::ScoreRanking(std::filesystem::path filePath, std::filesystem::path playLogDirectory)
		: filePath_(std::move(filePath)), playLogDirectory_(std::move(playLogDirectory)) {}

	void ScoreRanking::Initialize() {
		scores_.clear();
		lastRecordedRank_.reset();
		try {

			// 初回だけ既存ログを取り込み、次回以降の重複登録を保存済みJSONで防止
			if (!MadoEngine::Json::JsonFile::Exists(filePath_)) {
				ImportPlayLogs();
				if (!scores_.empty()) {
					Save();
				}
				return;
			}

			nlohmann::json root;
			if (!MadoEngine::Json::JsonFile::Load(filePath_, root)) {
				return;
			}
			if (!root.is_object() || !root.contains("scores") || !root.at("scores").is_array()) {
				Logger::Output("スコアランキングJSONの形式が不正です", Logger::Level::Warning);
				return;
			}

			// 不正な値を除外し、外部編集で順序や件数が変わった場合も上位10件へ正規化
			for (const nlohmann::json& value : root.at("scores")) {
				if (value.is_number_unsigned()) {
					scores_.push_back(value.get<std::uint64_t>());
				}
			}
			SortAndTrim();
		} catch (const std::exception& exception) {

			// 保存先のアクセス失敗をSceneの初期化へ伝播させない例外境界
			Logger::Output("スコアランキングの読み込みに失敗しました: " + std::string(exception.what()), Logger::Level::Warning);
		}
	}

	bool ScoreRanking::RecordScore(std::uint64_t score) {
		lastRecordedRank_.reset();

		// 上位10件に入らないスコアでは既存順位と保存ファイルを維持
		if (scores_.size() >= kMaxEntryCount && score <= scores_.back()) {
			return true;
		}

		// 同点は以前の記録の後ろへ挿入し、今回の一件だけを強調表示できる順位を保持
		const auto insertionPosition = std::upper_bound(scores_.begin(), scores_.end(), score, std::greater<std::uint64_t>{});
		lastRecordedRank_ = static_cast<std::size_t>(insertionPosition - scores_.begin());
		scores_.insert(insertionPosition, score);
		if (scores_.size() > kMaxEntryCount) {
			scores_.resize(kMaxEntryCount);
		}
		return Save();
	}

	void ScoreRanking::ImportPlayLogs() {
		if (!std::filesystem::is_directory(playLogDirectory_)) {
			return;
		}

		// 中断とリトライのログを除き、実際にゲームオーバーとなったプレイだけを移行
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(playLogDirectory_)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".json") {
				continue;
			}
			std::ifstream file(entry.path(), std::ios::binary);
			if (!file.is_open()) {
				continue;
			}
			const nlohmann::json root = nlohmann::json::parse(file, nullptr, false);
			if (!root.is_object() || !root.contains("スコア") || !root.at("スコア").is_number_unsigned() ||
				!root.contains("終了理由") || root.at("終了理由") != "プレイヤーの死亡") {
				continue;
			}
			scores_.push_back(root.at("スコア").get<std::uint64_t>());
		}
		SortAndTrim();
	}

	void ScoreRanking::SortAndTrim() {
		std::sort(scores_.begin(), scores_.end(), std::greater<std::uint64_t>{});
		if (scores_.size() > kMaxEntryCount) {
			scores_.resize(kMaxEntryCount);
		}
	}

	bool ScoreRanking::Save() const {
		const nlohmann::json root = { { "version", 1 }, { "scores", scores_ } };
		return MadoEngine::Json::JsonFile::Save(filePath_, root, 4, true);
	}

}
