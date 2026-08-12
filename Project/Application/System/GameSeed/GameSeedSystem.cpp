#include "GameSeedSystem.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>

namespace {
	constexpr int kCurrentFileVersion = 2;
	const std::filesystem::path kSeedHistoryFilePath = "SavedData/GameSeedHistory.json";

	/// @brief JSONから符号なし32bitのSeed値を取得
	/// @param seedJson 読み込み元のJSON
	/// @param outSeed 読み込んだSeed値の出力先
	/// @return Seed値を取得できた場合はtrue
	bool TryParseSeedValue(const nlohmann::json& seedJson, std::uint32_t& outSeed) {
		if (!seedJson.is_number_unsigned()) {
			return false;
		}

		const std::uint64_t seed = seedJson.get<std::uint64_t>();
		if (seed > std::numeric_limits<std::uint32_t>::max()) {
			return false;
		}

		outSeed = static_cast<std::uint32_t>(seed);
		return true;
	}

	/// @brief JSONからSeed履歴一件を取得
	/// @param entryJson 読み込み元のJSON
	/// @param outEntry 読み込んだSeed履歴の出力先
	/// @return Seed履歴を取得できた場合はtrue
	bool TryParseHistoryEntry(
		const nlohmann::json& entryJson,
		System::GameSeedSystem::HistoryEntry& outEntry
	) {

		// Version 1の数値配列をお気に入り未登録の履歴として復元
		if (TryParseSeedValue(entryJson, outEntry.seed)) {
			outEntry.isFavorite = false;
			return true;
		}

		if (!entryJson.is_object() || !entryJson.contains("seed") ||
			!TryParseSeedValue(entryJson.at("seed"), outEntry.seed)) {
			return false;
		}

		// Favorite値が欠損または不正な場合は未登録として安全に復元
		outEntry.isFavorite = entryJson.contains("favorite") && entryJson.at("favorite").is_boolean()
			? entryJson.at("favorite").get<bool>()
			: false;
		return true;
	}
}

namespace System {

	void GameSeedSystem::Initialize() {
		history_.clear();
		requestedSeed_.reset();

		// 初回起動では空の履歴から開始し、Game開始時までファイル生成を保留
		if (!MadoEngine::Json::JsonFile::Exists(kSeedHistoryFilePath)) {
			return;
		}

		nlohmann::json root;
		if (!MadoEngine::Json::JsonFile::Load(kSeedHistoryFilePath, root)) {
			return;
		}

		// 不正なルート構造から部分的な履歴を復元せず安全な空状態を維持
		if (!root.is_object() || !root.contains("seeds") || !root.at("seeds").is_array()) {
			Logger::Output("Seed履歴JSONの形式が不正です: " + kSeedHistoryFilePath.generic_string(), Logger::Level::Warning);
			return;
		}

		bool containsInvalidSeed = false;
		for (const nlohmann::json& entryJson : root.at("seeds")) {
			HistoryEntry entry;
			if (!TryParseHistoryEntry(entryJson, entry)) {
				containsInvalidSeed = true;
				continue;
			}

			history_.push_back(entry);
		}

		// 保存上限を超えた古い要素だけを破棄して最新10件を復元
		if (history_.size() > kMaxHistoryCount) {
			history_.erase(
				history_.begin(),
				history_.begin() + static_cast<std::ptrdiff_t>(history_.size() - kMaxHistoryCount)
			);
		}

		if (containsInvalidSeed) {
			Logger::Output("Seed履歴JSON内の不正な値を除外しました: " + kSeedHistoryFilePath.generic_string(), Logger::Level::Warning);
		}
	}

	void GameSeedSystem::RequestSeed(std::optional<std::uint32_t> requestedSeed) {
		requestedSeed_ = requestedSeed;
	}

	std::uint32_t GameSeedSystem::BeginGame() {

		// Titleで選択されたSeedは履歴を変更せず今回のGameだけで使用
		if (requestedSeed_.has_value()) {
			const std::uint32_t seed = requestedSeed_.value();
			requestedSeed_.reset();
			return seed;
		}

		// 未選択時だけ新規Seedを抽選して履歴を更新
		const std::uint32_t seed = MyRand::CreateSeed();
		RegisterGeneratedSeed(seed);
		Save();
		return seed;
	}

	bool GameSeedSystem::SetFavorite(std::size_t historyIndex, bool isFavorite) {
		if (historyIndex >= history_.size()) {
			return false;
		}

		if (history_[historyIndex].isFavorite == isFavorite) {
			return true;
		}

		// UI操作直後から状態を参照できるよう保存前に共有履歴へ反映
		history_[historyIndex].isFavorite = isFavorite;
		return Save();
	}

	const std::vector<GameSeedSystem::HistoryEntry>& GameSeedSystem::GetHistory() const {
		return history_;
	}

	void GameSeedSystem::RegisterGeneratedSeed(std::uint32_t seed) {

		// 新規Seedの追加前に最古の一件を破棄して保存上限を維持
		if (history_.size() >= kMaxHistoryCount) {
			history_.erase(history_.begin());
		}

		history_.push_back({ seed, false });
	}

	bool GameSeedSystem::Save() const {
		nlohmann::json root = nlohmann::json::object();
		root["version"] = kCurrentFileVersion;
		root["seeds"] = nlohmann::json::array();
		for (const HistoryEntry& entry : history_) {
			root["seeds"].push_back({
				{ "seed", entry.seed },
				{ "favorite", entry.isFavorite }
			});
		}
		return MadoEngine::Json::JsonFile::Save(kSeedHistoryFilePath, root, 4, true);
	}

}
