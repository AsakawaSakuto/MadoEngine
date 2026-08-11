#include "EffectSequenceSystem.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <string_view>
#include <system_error>

namespace {

	constexpr std::size_t kMaximumEffectSequenceAssetNameLength = 100;

	/// @brief UTF-8文字列からFilesystem Pathを生成
	/// @param value UTF-8文字列
	/// @return 生成したPath
	std::filesystem::path MakeUtf8Path(const std::string& value) {
		const auto* begin = reinterpret_cast<const char8_t*>(value.data());
		return std::filesystem::path(std::u8string(begin, begin + value.size()));
	}

	/// @brief Filesystem PathをUTF-8文字列へ変換
	/// @param path 変換対象Path
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(value.data()), value.size());
	}

	/// @brief Asset名から保存先Pathを安全に生成
	/// @param directoryPath Asset Directory
	/// @param assetName UTF-8 Asset名
	/// @param outFilePath 生成したPathの出力先
	/// @return 生成に成功した場合はtrue
	bool TryMakeAssetFilePath(
		const std::filesystem::path& directoryPath,
		const std::string& assetName,
		std::filesystem::path& outFilePath) {
		try {
			outFilePath = directoryPath / MakeUtf8Path(assetName + ".json");
			return true;
		} catch (const std::system_error&) {
			return false;
		}
	}

	/// @brief Windowsで使用可能なAsset名か確認
	/// @param assetName 確認対象名
	/// @return 使用可能な場合はtrue
	bool IsValidAssetName(const std::string& assetName) {
		if (
			assetName.empty() ||
			assetName.size() > kMaximumEffectSequenceAssetNameLength ||
			assetName.front() == ' ' ||
			assetName.back() == ' ' ||
			assetName.back() == '.') {
			return false;
		}
		constexpr std::string_view invalidCharacters = "<>:\"/\\|?*";
		for (const unsigned char character : assetName) {
			if (character < 32 || invalidCharacters.find(static_cast<char>(character)) != std::string_view::npos) {
				return false;
			}
		}
		std::string reservedName = assetName.substr(0, assetName.find('.'));
		std::transform(
			reservedName.begin(),
			reservedName.end(),
			reservedName.begin(),
			[](unsigned char character) {
				return static_cast<char>(std::toupper(character));
			}
		);
		if (
			reservedName == "CON" || reservedName == "PRN" ||
			reservedName == "AUX" || reservedName == "NUL") {
			return false;
		}
		return !(
			reservedName.size() == 4 &&
			(reservedName.starts_with("COM") || reservedName.starts_with("LPT")) &&
			reservedName[3] >= '1' && reservedName[3] <= '9'
		);
	}

	/// @brief JSONファイルがAsset Directory直下にあるか確認
	/// @param filePath 確認対象ファイル
	/// @param directoryPath Asset Directory
	/// @return Directory直下のJSONの場合はtrue
	bool IsAssetFilePath(
		const std::filesystem::path& filePath,
		const std::filesystem::path& directoryPath) {
		std::error_code error;
		const std::filesystem::path canonicalFile = std::filesystem::weakly_canonical(filePath, error);
		if (error) {
			return false;
		}
		const std::filesystem::path canonicalDirectory = std::filesystem::weakly_canonical(directoryPath, error);
		return !error && canonicalFile.extension() == ".json" && canonicalFile.parent_path() == canonicalDirectory;
	}

	/// @brief Asset JSONをTrash Directoryへ退避
	/// @param sourcePath 退避元JSON
	/// @param assetDirectoryPath Asset Directory
	/// @param outTrashPath 退避先Pathの出力先
	/// @return 退避に成功した場合はtrue
	bool MoveAssetFileToTrash(
		const std::filesystem::path& sourcePath,
		const std::filesystem::path& assetDirectoryPath,
		std::filesystem::path& outTrashPath) {
		if (!IsAssetFilePath(sourcePath, assetDirectoryPath)) {
			return false;
		}
		std::error_code error;
		if (!std::filesystem::exists(sourcePath, error) || error) {
			return false;
		}
		const std::filesystem::path trashDirectory = assetDirectoryPath / ".trash";
		std::filesystem::create_directories(trashDirectory, error);
		if (error) {
			return false;
		}
		const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
		try {
			outTrashPath = trashDirectory / MakeUtf8Path(
				PathToUtf8String(sourcePath.stem()) + "_" + std::to_string(timestamp) + ".json"
			);
		} catch (const std::system_error&) {
			return false;
		}
		std::filesystem::rename(sourcePath, outTrashPath, error);
		return !error;
	}

} // namespace

namespace MadoEngine::EffectSequence {

	EffectSequenceSystem& EffectSequenceSystem::GetInstance() {
		static EffectSequenceSystem instance;
		return instance;
	}

	void EffectSequenceSystem::Initialize() {
		Finalize();
		const std::size_t assetCount = LoadAssetsFromDirectory(assetDirectoryPath_);
		isInitialized_ = true;
		Logger::Output(
			"EffectSequenceSystemを初期化しました。Asset数: " + std::to_string(assetCount),
			Logger::Level::Engine
		);
	}

	void EffectSequenceSystem::Finalize() {
		for (SequenceSlot& slot : sequenceSlots_) {
			if (slot.instance) {
				slot.instance->Destroy(EffectSequenceFinishReason::StopImmediate);
			}
		}
		assets_.clear();
		assetPaths_.clear();
		sequenceSlots_.clear();
		freeSlotIndices_ = {};
		finishedEvents_.clear();
		isInitialized_ = false;
	}

	std::size_t EffectSequenceSystem::LoadAssetsFromDirectory(
		const std::filesystem::path& directoryPath) {
		assetDirectoryPath_ = directoryPath;
		std::error_code error;
		std::filesystem::create_directories(directoryPath, error);
		if (error) {
			Logger::Output(
				"Effect Sequence Asset Directoryを作成できません: " + PathToUtf8String(directoryPath),
				Logger::Level::Error
			);
			return 0;
		}

		std::vector<std::filesystem::path> paths;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath, error)) {
			if (error) {
				break;
			}
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				paths.push_back(entry.path());
			}
		}
		std::sort(paths.begin(), paths.end());
		std::size_t loadedCount = 0;
		for (const std::filesystem::path& path : paths) {
			loadedCount += LoadAsset(path) ? 1 : 0;
		}
		return loadedCount;
	}

	bool EffectSequenceSystem::LoadAsset(const std::filesystem::path& filePath) {
		auto asset = std::make_shared<EffectSequenceAsset>();
		if (!asset->LoadFromFile(filePath) || asset->GetName().empty()) {
			return false;
		}
		const std::string name = asset->GetName();
		assets_[name] = std::move(asset);
		assetPaths_[name] = filePath;
		Logger::Output("Effect Sequence Assetを登録しました: " + name, Logger::Level::Assets);
		return true;
	}

	bool EffectSequenceSystem::ReloadAsset(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		return found != assetPaths_.end() && LoadAsset(found->second);
	}

	bool EffectSequenceSystem::LoadAssetBackup(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		if (found == assetPaths_.end()) {
			return false;
		}
		std::filesystem::path backupPath = found->second;
		backupPath += ".bak";
		auto asset = std::make_shared<EffectSequenceAsset>();
		if (!asset->LoadFromFile(backupPath)) {
			return false;
		}
		asset->SetName(assetName);
		asset->SetFilePath(found->second);
		assets_[assetName] = std::move(asset);
		Logger::Output(
			"Effect Sequence AssetのBackupを読み込みました: " + assetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool EffectSequenceSystem::CreateAsset(const std::string& assetName) {
		if (!IsAssetNameAvailable(assetName)) {
			return false;
		}
		std::filesystem::path filePath;
		if (!TryMakeAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			return false;
		}
		EffectSequenceAsset asset;
		asset.SetName(assetName);
		asset.Validate();
		return asset.SaveToFile(filePath, false) && LoadAsset(filePath);
	}

	bool EffectSequenceSystem::DuplicateAsset(
		const std::string& sourceAssetName,
		const std::string& newAssetName) {
		const auto source = assets_.find(sourceAssetName);
		if (source == assets_.end() || !IsAssetNameAvailable(newAssetName)) {
			return false;
		}
		std::filesystem::path filePath;
		if (!TryMakeAssetFilePath(assetDirectoryPath_, newAssetName, filePath)) {
			return false;
		}
		EffectSequenceAsset copy = *source->second;
		copy.SetName(newAssetName);
		copy.Validate();
		return copy.SaveToFile(filePath, false) && LoadAsset(filePath);
	}

	bool EffectSequenceSystem::RenameAsset(
		const std::string& assetName,
		const std::string& newAssetName) {
		const auto asset = assets_.find(assetName);
		const auto path = assetPaths_.find(assetName);
		if (
			asset == assets_.end() || path == assetPaths_.end() ||
			!IsAssetNameAvailable(newAssetName) ||
			!IsAssetFilePath(path->second, assetDirectoryPath_)) {
			return false;
		}
		std::filesystem::path newPath;
		if (!TryMakeAssetFilePath(assetDirectoryPath_, newAssetName, newPath)) {
			return false;
		}
		EffectSequenceAsset renamed = *asset->second;
		renamed.SetName(newAssetName);
		renamed.Validate();
		if (!renamed.SaveToFile(newPath, false)) {
			return false;
		}
		auto loaded = std::make_shared<EffectSequenceAsset>();
		if (!loaded->LoadFromFile(newPath)) {
			std::error_code error;
			std::filesystem::remove(newPath, error);
			return false;
		}
		std::filesystem::path trashPath;
		if (!MoveAssetFileToTrash(path->second, assetDirectoryPath_, trashPath)) {
			std::error_code error;
			std::filesystem::remove(newPath, error);
			return false;
		}
		assets_.erase(asset);
		assetPaths_.erase(path);
		assets_[newAssetName] = std::move(loaded);
		assetPaths_[newAssetName] = newPath;
		Logger::Output(
			"Effect Sequence Asset名を変更しました: " + assetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool EffectSequenceSystem::DeleteAsset(const std::string& assetName) {
		const auto asset = assets_.find(assetName);
		const auto path = assetPaths_.find(assetName);
		if (asset == assets_.end() || path == assetPaths_.end()) {
			return false;
		}
		std::filesystem::path trashPath;
		if (!MoveAssetFileToTrash(path->second, assetDirectoryPath_, trashPath)) {
			return false;
		}
		assets_.erase(asset);
		assetPaths_.erase(path);
		Logger::Output(
			"Effect Sequence AssetをTrashへ退避しました: " + PathToUtf8String(trashPath),
			Logger::Level::Assets
		);
		return true;
	}

	bool EffectSequenceSystem::IsAssetNameAvailable(const std::string& assetName) const {
		if (!IsValidAssetName(assetName) || assets_.contains(assetName)) {
			return false;
		}
		std::filesystem::path path;
		if (!TryMakeAssetFilePath(assetDirectoryPath_, assetName, path)) {
			return false;
		}
		std::error_code error;
		return !std::filesystem::exists(path, error) && !error;
	}

	EffectSequenceHandle EffectSequenceSystem::Play(
		const std::string& assetName,
		const EffectSequencePlayDesc& desc) {
		if (!isInitialized_) {
			Logger::Output("初期化前にEffect Sequenceは再生できません。", Logger::Level::Warning);
			return {};
		}
		const auto found = assets_.find(assetName);
		if (found == assets_.end()) {
			Logger::Output("Effect Sequence Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return {};
		}

		// 再生終了済みSlotを再利用してHandle Indexの増加を抑制
		uint32_t slotIndex = 0;
		if (!freeSlotIndices_.empty()) {
			slotIndex = freeSlotIndices_.front();
			freeSlotIndices_.pop();
		} else {
			slotIndex = static_cast<uint32_t>(sequenceSlots_.size());
			sequenceSlots_.push_back({});
		}
		SequenceSlot& slot = sequenceSlots_[slotIndex];
		slot.instance = std::make_unique<EffectSequenceInstance>();
		slot.instance->Initialize(found->second, desc, dispatcher_);
		return { slotIndex, slot.generation };
	}

	void EffectSequenceSystem::Stop(
		EffectSequenceHandle handle,
		EffectSequenceStopMode mode) {
		EffectSequenceInstance* instance = Resolve(handle);
		if (!instance) {
			return;
		}
		instance->Stop(mode);
		if (instance->IsFinished()) {
			CompleteAndReleaseSlot(handle.index, true);
		}
	}

	bool EffectSequenceSystem::Pause(EffectSequenceHandle handle) {
		EffectSequenceInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->Pause();
		return true;
	}

	bool EffectSequenceSystem::Resume(EffectSequenceHandle handle) {
		EffectSequenceInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->Resume();
		return true;
	}

	bool EffectSequenceSystem::SetTransform(
		EffectSequenceHandle handle,
		const Transform3D& transform) {
		EffectSequenceInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->SetTransform(transform);
		return true;
	}

	bool EffectSequenceSystem::SetPlaybackSpeed(
		EffectSequenceHandle handle,
		float playbackSpeed) {
		EffectSequenceInstance* instance = Resolve(handle);
		return instance && instance->SetPlaybackSpeed(playbackSpeed);
	}

	bool EffectSequenceSystem::IsAlive(EffectSequenceHandle handle) const {
		return Resolve(handle) != nullptr;
	}

	bool EffectSequenceSystem::IsPaused(EffectSequenceHandle handle) const {
		const EffectSequenceInstance* instance = Resolve(handle);
		return instance && instance->IsPaused();
	}

	std::optional<float> EffectSequenceSystem::GetPlaybackTime(EffectSequenceHandle handle) const {
		const EffectSequenceInstance* instance = Resolve(handle);
		return instance ? std::optional<float>{ instance->GetPlaybackTime() } : std::nullopt;
	}

	void EffectSequenceSystem::Update(float deltaTime) {
		if (!isInitialized_) {
			return;
		}

		// 全Node完了後にCallbackを確定してからSequence Slotを回収
		for (uint32_t index = 0; index < sequenceSlots_.size(); ++index) {
			SequenceSlot& slot = sequenceSlots_[index];
			if (!slot.instance) {
				continue;
			}
			slot.instance->Update(deltaTime);
			if (slot.instance->IsFinished()) {
				CompleteAndReleaseSlot(index, true);
			}
		}
	}

	void EffectSequenceSystem::ClearScene(SceneType sceneType) {
		if (sceneType == SceneType::None) {
			return;
		}
		for (uint32_t index = 0; index < sequenceSlots_.size(); ++index) {
			SequenceSlot& slot = sequenceSlots_[index];
			if (!slot.instance || slot.instance->GetSceneType() != sceneType) {
				continue;
			}
			slot.instance->Destroy(EffectSequenceFinishReason::SceneCleared);
			CompleteAndReleaseSlot(index, false);
		}
	}

	void EffectSequenceSystem::StopAll(EffectSequenceStopMode mode) {
		for (uint32_t index = 0; index < sequenceSlots_.size(); ++index) {
			SequenceSlot& slot = sequenceSlots_[index];
			if (!slot.instance) {
				continue;
			}
			slot.instance->Stop(mode);
			if (slot.instance->IsFinished()) {
				CompleteAndReleaseSlot(index, true);
			}
		}
	}

	std::vector<std::string> EffectSequenceSystem::GetAssetNames() const {
		std::vector<std::string> names;
		names.reserve(assets_.size());
		for (const auto& [name, asset] : assets_) {
			(void)asset;
			names.push_back(name);
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	const EffectSequenceAsset* EffectSequenceSystem::FindAsset(const std::string& assetName) const {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	EffectSequenceAsset* EffectSequenceSystem::FindEditableAsset(const std::string& assetName) {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	std::size_t EffectSequenceSystem::GetActiveSequenceCount() const {
		return static_cast<std::size_t>(std::count_if(
			sequenceSlots_.begin(),
			sequenceSlots_.end(),
			[](const SequenceSlot& slot) {
				return slot.instance != nullptr;
			}
		));
	}

	std::vector<EffectSequenceFinishedEvent> EffectSequenceSystem::ConsumeFinishedEvents() {
		std::vector<EffectSequenceFinishedEvent> events = std::move(finishedEvents_);
		finishedEvents_.clear();
		return events;
	}

	EffectSequenceInstance* EffectSequenceSystem::Resolve(EffectSequenceHandle handle) {
		if (!handle.HasValue() || handle.index >= sequenceSlots_.size()) {
			return nullptr;
		}
		SequenceSlot& slot = sequenceSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	const EffectSequenceInstance* EffectSequenceSystem::Resolve(EffectSequenceHandle handle) const {
		if (!handle.HasValue() || handle.index >= sequenceSlots_.size()) {
			return nullptr;
		}
		const SequenceSlot& slot = sequenceSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	void EffectSequenceSystem::CompleteAndReleaseSlot(uint32_t index, bool emitEvent) {
		SequenceSlot& slot = sequenceSlots_[index];
		if (!slot.instance) {
			return;
		}
		if (emitEvent) {
			finishedEvents_.push_back({
				{ index, slot.generation },
				slot.instance->GetAssetName(),
				slot.instance->GetSceneType(),
				slot.instance->GetFinishReason(),
				slot.instance->GetPlaybackContext(),
			});
		}
		ReleaseSlot(index);
	}

	void EffectSequenceSystem::ReleaseSlot(uint32_t index) {
		SequenceSlot& slot = sequenceSlots_[index];
		slot.instance.reset();
		++slot.generation;
		if (slot.generation == 0) {
			slot.generation = 1;
		}
		freeSlotIndices_.push(index);
	}

} // namespace MadoEngine::EffectSequence
