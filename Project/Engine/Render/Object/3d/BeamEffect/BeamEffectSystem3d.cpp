#include "BeamEffectSystem3d.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <string_view>
#include <system_error>

namespace {

	constexpr std::size_t kMaximumBeamAssetNameLength = 100;

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
		}
		catch (const std::system_error&) {
			return false;
		}
	}

	/// @brief Windowsで使用可能なAsset名か確認
	/// @param assetName 確認対象名
	/// @return 使用可能な場合はtrue
	bool IsValidAssetName(const std::string& assetName) {
		if (
			assetName.empty() ||
			assetName.size() > kMaximumBeamAssetNameLength ||
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

	/// @brief JsonファイルがAsset Directory直下にあるか確認
	/// @param filePath 確認対象ファイル
	/// @param directoryPath Asset Directory
	/// @return Directory直下のJsonの場合はtrue
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

	/// @brief Asset JsonをTrash Directoryへ退避
	/// @param sourcePath 退避元Json
	/// @param assetDirectoryPath Asset Directory
	/// @param outTrashPath 退避先Path出力
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
		}
		catch (const std::system_error&) {
			return false;
		}
		std::filesystem::rename(sourcePath, outTrashPath, error);
		return !error;
	}

} // namespace

namespace MadoEngine::Beam {

	BeamEffectSystem3d& BeamEffectSystem3d::GetInstance() {
		static BeamEffectSystem3d instance;
		return instance;
	}

	void BeamEffectSystem3d::Initialize(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		MadoEngine::Render::PSORegistry* psoRegistry) {
		Finalize();
		renderer_.Initialize(device, commandList, psoRegistry);
		const std::size_t assetCount = LoadAssetsFromDirectory(assetDirectoryPath_);
		isInitialized_ = true;
		Logger::Output(
			"BeamEffectSystem3dを初期化しました。Asset数: " + std::to_string(assetCount),
			Logger::Level::Engine
		);
	}

	void BeamEffectSystem3d::Finalize() {
		renderer_.Finalize();
		assets_.clear();
		assetPaths_.clear();
		effectSlots_.clear();
		freeSlotIndices_ = {};
		preparedSceneType_ = SceneType::None;
		currentSubmissionFenceValue_ = 0;
		isRenderDataPrepared_ = false;
		isInitialized_ = false;
	}

	std::size_t BeamEffectSystem3d::LoadAssetsFromDirectory(
		const std::filesystem::path& directoryPath) {
		assetDirectoryPath_ = directoryPath;
		std::error_code error;
		std::filesystem::create_directories(directoryPath, error);
		if (error) {
			Logger::Output(
				"Beam Effect Asset Directoryを作成できません: " + PathToUtf8String(directoryPath),
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

	bool BeamEffectSystem3d::LoadAsset(const std::filesystem::path& filePath) {
		auto asset = std::make_shared<BeamEffectAsset>();
		if (!asset->LoadFromFile(filePath) || asset->GetName().empty()) {
			return false;
		}
		const std::string name = asset->GetName();
		assets_[name] = std::move(asset);
		assetPaths_[name] = filePath;
		Logger::Output("Beam Effect Assetを登録しました: " + name, Logger::Level::Assets);
		return true;
	}

	bool BeamEffectSystem3d::ReloadAsset(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		return found != assetPaths_.end() && LoadAsset(found->second);
	}

	bool BeamEffectSystem3d::LoadAssetBackup(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		if (found == assetPaths_.end()) {
			return false;
		}
		std::filesystem::path backupPath = found->second;
		backupPath += ".bak";
		auto asset = std::make_shared<BeamEffectAsset>();
		if (!asset->LoadFromFile(backupPath)) {
			return false;
		}
		asset->SetName(assetName);
		asset->SetFilePath(found->second);
		assets_[assetName] = std::move(asset);
		Logger::Output("Beam Effect AssetのBackupを読み込みました: " + assetName, Logger::Level::Assets);
		return true;
	}

	bool BeamEffectSystem3d::CreateAsset(const std::string& assetName) {
		if (!IsAssetNameAvailable(assetName)) {
			return false;
		}
		std::filesystem::path filePath;
		if (!TryMakeAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			return false;
		}
		BeamEffectAsset asset;
		asset.SetName(assetName);
		asset.Validate();
		return asset.SaveToFile(filePath, false) && LoadAsset(filePath);
	}

	bool BeamEffectSystem3d::DuplicateAsset(
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
		BeamEffectAsset copy = *source->second;
		copy.SetName(newAssetName);
		copy.Validate();
		return copy.SaveToFile(filePath, false) && LoadAsset(filePath);
	}

	bool BeamEffectSystem3d::RenameAsset(
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
		BeamEffectAsset renamed = *asset->second;
		renamed.SetName(newAssetName);
		renamed.Validate();
		if (!renamed.SaveToFile(newPath, false)) {
			return false;
		}
		auto loaded = std::make_shared<BeamEffectAsset>();
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
			"Beam Effect Asset名を変更しました: " + assetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool BeamEffectSystem3d::DeleteAsset(const std::string& assetName) {
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
			"Beam Effect AssetをTrashへ退避しました: " + PathToUtf8String(trashPath),
			Logger::Level::Assets
		);
		return true;
	}

	bool BeamEffectSystem3d::IsAssetNameAvailable(const std::string& assetName) const {
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

	BeamEffectHandle BeamEffectSystem3d::Play(
		const std::string& assetName,
		const BeamEffectPlayDesc& desc) {
		if (!isInitialized_) {
			Logger::Output("初期化前にはBeam Effectを再生できません。", Logger::Level::Warning);
			return {};
		}
		const auto found = assets_.find(assetName);
		if (found == assets_.end()) {
			Logger::Output("Beam Effect Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return {};
		}
		uint32_t slotIndex = 0;
		if (!freeSlotIndices_.empty()) {
			slotIndex = freeSlotIndices_.front();
			freeSlotIndices_.pop();
		} else {
			slotIndex = static_cast<uint32_t>(effectSlots_.size());
			effectSlots_.push_back({});
		}
		EffectSlot& slot = effectSlots_[slotIndex];
		slot.instance = std::make_unique<BeamEffectInstance>();
		slot.instance->Initialize(found->second, desc);
		isRenderDataPrepared_ = false;
		return { slotIndex, slot.generation };
	}

	void BeamEffectSystem3d::Stop(BeamEffectHandle handle, BeamStopMode mode) {
		if (BeamEffectInstance* instance = Resolve(handle)) {
			instance->Stop(mode);
			isRenderDataPrepared_ = false;
		}
	}

	bool BeamEffectSystem3d::Pause(BeamEffectHandle handle) {
		BeamEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->Pause();
		return true;
	}

	bool BeamEffectSystem3d::Resume(BeamEffectHandle handle) {
		BeamEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->Resume();
		return true;
	}

	bool BeamEffectSystem3d::SetPlaybackSpeed(
		BeamEffectHandle handle,
		float playbackSpeed) {
		BeamEffectInstance* instance = Resolve(handle);
		return instance && instance->SetPlaybackSpeed(playbackSpeed);
	}

	bool BeamEffectSystem3d::IsPaused(BeamEffectHandle handle) const {
		const BeamEffectInstance* instance = Resolve(handle);
		return instance && instance->IsPaused();
	}

	bool BeamEffectSystem3d::SetEndpoints(
		BeamEffectHandle handle,
		const Vector3& startPosition,
		const Vector3& endPosition) {
		BeamEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->SetEndpoints(startPosition, endPosition);
		isRenderDataPrepared_ = false;
		return true;
	}

	bool BeamEffectSystem3d::SetStartPosition(
		BeamEffectHandle handle,
		const Vector3& position) {
		BeamEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->SetStartPosition(position);
		isRenderDataPrepared_ = false;
		return true;
	}

	bool BeamEffectSystem3d::SetEndPosition(
		BeamEffectHandle handle,
		const Vector3& position) {
		BeamEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->SetEndPosition(position);
		isRenderDataPrepared_ = false;
		return true;
	}

	bool BeamEffectSystem3d::IsAlive(BeamEffectHandle handle) const {
		return Resolve(handle) != nullptr;
	}

	void BeamEffectSystem3d::Update(float deltaTime) {
		if (!isInitialized_) {
			return;
		}
		isRenderDataPrepared_ = false;
		for (uint32_t index = 0; index < effectSlots_.size(); ++index) {
			EffectSlot& slot = effectSlots_[index];
			if (!slot.instance) {
				continue;
			}
			slot.instance->Update(deltaTime);
			if (slot.instance->IsFinished()) {
				ReleaseSlot(index);
			}
		}
	}

	void BeamEffectSystem3d::BeginFrame(uint64_t submissionFenceValue) {
		currentSubmissionFenceValue_ = submissionFenceValue;
		isRenderDataPrepared_ = false;
	}

	void BeamEffectSystem3d::DrawLayerMask(
		SceneType sceneType,
		const Camera& camera,
		MadoEngine::Render::RenderLayerMask layerMask) {
		if (!isInitialized_ || layerMask == 0) {
			return;
		}
		if (!isRenderDataPrepared_ || preparedSceneType_ != sceneType) {
			renderer_.Begin(camera, currentSubmissionFenceValue_);
			for (const EffectSlot& slot : effectSlots_) {
				if (!slot.instance || !slot.instance->Matches(sceneType, MadoEngine::Render::kAllRenderLayers)) {
					continue;
				}
				slot.instance->SubmitRenderData(renderer_);
			}
			preparedSceneType_ = sceneType;
			isRenderDataPrepared_ = true;
		}
		renderer_.Draw(layerMask);
	}

	void BeamEffectSystem3d::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		renderer_.OnGpuFrameCompleted(completedFenceValue);
	}

	void BeamEffectSystem3d::ClearScene(SceneType sceneType) {
		if (sceneType == SceneType::None) {
			return;
		}
		for (uint32_t index = 0; index < effectSlots_.size(); ++index) {
			EffectSlot& slot = effectSlots_[index];
			if (slot.instance && slot.instance->GetSceneType() == sceneType) {
				ReleaseSlot(index);
			}
		}
		isRenderDataPrepared_ = false;
	}

	void BeamEffectSystem3d::StopAll(BeamStopMode mode) {
		for (EffectSlot& slot : effectSlots_) {
			if (slot.instance) {
				slot.instance->Stop(mode);
			}
		}
		isRenderDataPrepared_ = false;
	}

	std::vector<std::string> BeamEffectSystem3d::GetAssetNames() const {
		std::vector<std::string> names;
		names.reserve(assets_.size());
		for (const auto& [name, asset] : assets_) {
			(void)asset;
			names.push_back(name);
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	const BeamEffectAsset* BeamEffectSystem3d::FindAsset(const std::string& assetName) const {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	BeamEffectAsset* BeamEffectSystem3d::FindEditableAsset(const std::string& assetName) {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	std::size_t BeamEffectSystem3d::GetActiveEffectCount() const {
		return static_cast<std::size_t>(std::count_if(
			effectSlots_.begin(),
			effectSlots_.end(),
			[](const EffectSlot& slot) {
				return slot.instance != nullptr;
			}
		));
	}

	BeamEffectInstance* BeamEffectSystem3d::Resolve(BeamEffectHandle handle) {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}
		EffectSlot& slot = effectSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	const BeamEffectInstance* BeamEffectSystem3d::Resolve(BeamEffectHandle handle) const {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}
		const EffectSlot& slot = effectSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	void BeamEffectSystem3d::ReleaseSlot(uint32_t index) {
		EffectSlot& slot = effectSlots_[index];
		slot.instance.reset();
		++slot.generation;
		if (slot.generation == 0) {
			slot.generation = 1;
		}
		freeSlotIndices_.push(index);
	}

} // namespace MadoEngine::Beam
