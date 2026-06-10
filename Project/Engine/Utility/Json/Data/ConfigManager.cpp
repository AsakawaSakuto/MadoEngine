#include "ConfigManager.h"
#include "../Core/JsonFile.h"
#include "../Core/JsonSerializer.h"

namespace MadoEngine::Json {

	void WindowConfig::FromJson(const nlohmann::json& json) {
		if (!json.is_object()) {
			return;
		}

		title = JsonSerializer::GetOrDefault<std::string>(json, "title", title);
		iconPath = JsonSerializer::GetOrDefault<std::string>(json, "iconPath", iconPath);
		width = JsonSerializer::GetOrDefault<int>(json, "width", width);
		height = JsonSerializer::GetOrDefault<int>(json, "height", height);
		isResizable = JsonSerializer::GetOrDefault<bool>(json, "isResizable", isResizable);
		isShowMouseCursor = JsonSerializer::GetOrDefault<bool>(json, "isShowMouseCursor", isShowMouseCursor);
		isFullscreen = JsonSerializer::GetOrDefault<bool>(json, "isFullscreen", isFullscreen);
		isVsync = JsonSerializer::GetOrDefault<bool>(json, "isVsync", isVsync);
	}

	nlohmann::json WindowConfig::ToJson() const {
		return {
			{ "title", title },
			{ "iconPath", iconPath },
			{ "width", width },
			{ "height", height },
			{ "isResizable", isResizable },
			{ "isShowMouseCursor", isShowMouseCursor },
			{ "isFullscreen", isFullscreen },
			{ "isVsync", isVsync }
		};
	}

	bool ConfigManager::Load(const std::filesystem::path& filePath) {
		filePath_ = filePath;
		if (!JsonFile::Load(filePath_, root_)) {
			root_ = nlohmann::json::object();
			return false;
		}

		if (!root_.is_object()) {
			root_ = nlohmann::json::object();
			return false;
		}

		return true;
	}

	bool ConfigManager::Save(bool createBackup) const {
		if (filePath_.empty()) {
			return false;
		}

		return JsonFile::Save(filePath_, root_, 4, createBackup);
	}

	bool ConfigManager::SaveAs(const std::filesystem::path& filePath, bool createBackup) {
		filePath_ = filePath;
		return Save(createBackup);
	}

	void ConfigManager::Clear() {
		root_ = nlohmann::json::object();
	}

	WindowConfig ConfigManager::GetWindowConfig(const WindowConfig& defaultValue) const {
		WindowConfig config = defaultValue;
		if (ContainsSection("window")) {
			config.FromJson(root_.at("window"));
		}

		return config;
	}

	void ConfigManager::SetWindowConfig(const WindowConfig& config) {
		root_["window"] = config.ToJson();
	}

	bool ConfigManager::ContainsSection(const std::string& sectionName) const {
		return root_.is_object() && root_.contains(sectionName);
	}

	const nlohmann::json& ConfigManager::GetSection(const std::string& sectionName) const {
		static const nlohmann::json emptySection = nlohmann::json::object();
		if (!ContainsSection(sectionName)) {
			return emptySection;
		}

		return root_.at(sectionName);
	}

	void ConfigManager::SetSection(const std::string& sectionName, const nlohmann::json& json) {
		root_[sectionName] = json;
	}

}
