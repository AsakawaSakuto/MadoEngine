#include "LoggerEditor.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

	namespace {

		constexpr std::array kLogLevels = {
			Logger::Level::Engine,
			Logger::Level::Application,
			Logger::Level::Assets,
			Logger::Level::Warning,
			Logger::Level::Error,
			Logger::Level::Debug
		};
		constexpr std::size_t kLogLevelCount = kLogLevels.size();

		/// @brief ログレベルの表示名を取得する
		/// @param level 表示名を取得するログレベル
		/// @return ログレベルの表示名
		const char* GetLevelName(Logger::Level level) {
			switch (level) {
			case Logger::Level::Engine:
				return "Engine";
			case Logger::Level::Application:
				return "Application";
			case Logger::Level::Assets:
				return "Assets";
			case Logger::Level::Warning:
				return "Warning";
			case Logger::Level::Error:
				return "Error";
			case Logger::Level::Debug:
				return "Debug";
			default:
				return "Unknown";
			}
		}

		/// @brief ログレベルに対応する表示色を取得する
		/// @param level 表示色を取得するログレベル
		/// @return ImGuiで使用する表示色
		ImVec4 GetLevelColor(Logger::Level level) {
			switch (level) {
			case Logger::Level::Engine:
				return ImVec4(0.72f, 0.86f, 1.0f, 1.0f);
			case Logger::Level::Application:
				return ImVec4(0.82f, 1.0f, 0.76f, 1.0f);
			case Logger::Level::Assets:
				return ImVec4(0.95f, 0.86f, 0.58f, 1.0f);
			case Logger::Level::Warning:
				return ImVec4(1.0f, 0.72f, 0.28f, 1.0f);
			case Logger::Level::Error:
				return ImVec4(1.0f, 0.36f, 0.36f, 1.0f);
			case Logger::Level::Debug:
				return ImVec4(0.74f, 0.74f, 0.82f, 1.0f);
			default:
				return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		/// @brief ログレベルがフィルタで表示対象か判定する
		/// @param level 判定するログレベル
		/// @param levelFilters レベル別の表示フラグ
		/// @return 表示対象の場合はtrue
		bool IsLevelEnabled(Logger::Level level, const std::array<bool, kLogLevelCount>& levelFilters) {
			const std::size_t index = static_cast<std::size_t>(level);
			if (index >= levelFilters.size()) {
				return true;
			}

			return levelFilters[index];
		}

		/// @brief ログが検索文字列に一致するか判定する
		/// @param entry 判定するログ
		/// @param searchText 検索文字列
		/// @return 検索条件に一致する場合はtrue
		bool MatchesSearchText(const Logger::LogEntry& entry, const char* searchText) {
			if (searchText == nullptr || searchText[0] == '\0') {
				return true;
			}

			return entry.formattedText.find(searchText) != std::string::npos;
		}

		/// @brief ログが現在の表示条件に一致するか判定する
		/// @param entry 判定するログ
		/// @param levelFilters レベル別の表示フラグ
		/// @param searchText 検索文字列
		/// @return 表示対象の場合はtrue
		bool ShouldShowLogEntry(
			const Logger::LogEntry& entry,
			const std::array<bool, kLogLevelCount>& levelFilters,
			const char* searchText) {
			return IsLevelEnabled(entry.level, levelFilters) && MatchesSearchText(entry, searchText);
		}

		/// @brief 表示対象ログの件数を数える
		/// @param entries ログ履歴
		/// @param levelFilters レベル別の表示フラグ
		/// @param searchText 検索文字列
		/// @return 表示対象ログの件数
		std::size_t CountVisibleEntries(
			const std::vector<Logger::LogEntry>& entries,
			const std::array<bool, kLogLevelCount>& levelFilters,
			const char* searchText) {
			std::size_t count = 0;
			for (const Logger::LogEntry& entry : entries) {
				if (ShouldShowLogEntry(entry, levelFilters, searchText)) {
					++count;
				}
			}

			return count;
		}

		/// @brief 現在の表示条件に一致するログをクリップボード用文字列に変換する
		/// @param entries ログ履歴
		/// @param levelFilters レベル別の表示フラグ
		/// @param searchText 検索文字列
		/// @return クリップボードへ設定する文字列
		std::string BuildClipboardText(
			const std::vector<Logger::LogEntry>& entries,
			const std::array<bool, kLogLevelCount>& levelFilters,
			const char* searchText) {
			std::string text;
			for (const Logger::LogEntry& entry : entries) {
				if (!ShouldShowLogEntry(entry, levelFilters, searchText)) {
					continue;
				}

				text += entry.formattedText;
				text += '\n';
			}

			return text;
		}

	}

	void DrawLoggerEditorUI() {
		static std::array<bool, kLogLevelCount> levelFilters = []() {
			std::array<bool, kLogLevelCount> filters{};
			filters.fill(true);
			return filters;
		}();
		static bool autoScroll = true;
		static char searchText[128] = {};
		static std::uint64_t lastTailSequence = 0;

		std::vector<Logger::LogEntry> entries = Logger::GetEntries();
		std::uint64_t tailSequence = entries.empty() ? 0 : entries.back().sequence;
		bool hasNewLog = tailSequence != lastTailSequence;

		if (ImGui::Begin("Logger")) {
			ImGui::Checkbox("自動スクロール", &autoScroll);
			ImGui::SameLine();

			if (ImGui::Button("クリア")) {
				Logger::ClearEntries();
				entries.clear();
				tailSequence = 0;
				hasNewLog = false;
			}

			ImGui::SameLine();
			if (ImGui::Button("コピー")) {
				const std::string clipboardText = BuildClipboardText(entries, levelFilters, searchText);
				ImGui::SetClipboardText(clipboardText.c_str());
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(220.0f);
			ImGui::InputText("検索", searchText, sizeof(searchText));

			for (std::size_t i = 0; i < kLogLevels.size(); ++i) {
				if (i != 0) {
					ImGui::SameLine();
				}

				ImGui::Checkbox(GetLevelName(kLogLevels[i]), &levelFilters[i]);
			}

			const std::size_t visibleCount = CountVisibleEntries(entries, levelFilters, searchText);
			ImGui::Text(
				"表示: %d / %d",
				static_cast<int>(visibleCount),
				static_cast<int>(entries.size())
			);
			ImGui::Separator();

			ImGui::BeginChild("LoggerScroll", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
			for (const Logger::LogEntry& entry : entries) {
				if (!ShouldShowLogEntry(entry, levelFilters, searchText)) {
					continue;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));
				ImGui::TextUnformatted(entry.formattedText.c_str());
				ImGui::PopStyleColor();
			}

			if (autoScroll && hasNewLog) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}

		ImGui::End();
		lastTailSequence = tailSequence;
	}

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
