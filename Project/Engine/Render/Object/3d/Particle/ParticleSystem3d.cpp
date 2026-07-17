#include "ParticleSystem3d.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <string_view>
#include <system_error>

namespace {

	constexpr std::size_t kMaximumParticleAssetNameLength = 100;

	/// @brief UTF-8文字列からFilesystem Pathを生成する
	/// @param value UTF-8文字列
	/// @return 生成したFilesystem Path
	std::filesystem::path MakeUtf8Path(const std::string& value) {
		const auto* begin = reinterpret_cast<const char8_t*>(value.data());
		const auto* end = begin + value.size();
		return std::filesystem::path(std::u8string(begin, end));
	}

	/// @brief Filesystem PathをUTF-8文字列へ変換する
	/// @param path 変換するPath
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(
			reinterpret_cast<const char*>(value.data()),
			value.size()
		);
	}

	/// @brief Particle Asset名からJsonファイルパスを安全に生成する
	/// @param directoryPath Particle Assetディレクトリ
	/// @param assetName UTF-8のParticle Asset名
	/// @param outFilePath 生成したPathの出力先
	/// @return 生成に成功した場合はtrue
	bool TryMakeParticleAssetFilePath(
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

	/// @brief Particle Asset名をJsonファイル名として使用できるか確認する
	/// @param assetName 確認するAsset名
	/// @return 使用できる場合はtrue
	bool IsValidParticleAssetName(const std::string& assetName) {
		if (
			assetName.empty() ||
			assetName.size() > kMaximumParticleAssetNameLength ||
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
		std::transform(reservedName.begin(), reservedName.end(), reservedName.begin(), [](unsigned char character) {
			return static_cast<char>(std::toupper(character));
		});
		if (
			reservedName == "CON" ||
			reservedName == "PRN" ||
			reservedName == "AUX" ||
			reservedName == "NUL") {
			return false;
		}
		if (
			reservedName.size() == 4 &&
			(reservedName.starts_with("COM") || reservedName.starts_with("LPT")) &&
			reservedName[3] >= '1' &&
			reservedName[3] <= '9') {
			return false;
		}

		return true;
	}

	/// @brief JsonファイルがParticle Assetディレクトリ直下にあるか確認する
	/// @param filePath 確認するJsonファイル
	/// @param directoryPath Particle Assetディレクトリ
	/// @return 対象ディレクトリ直下のJsonファイルの場合はtrue
	bool IsParticleAssetFilePath(
		const std::filesystem::path& filePath,
		const std::filesystem::path& directoryPath) {
		std::error_code error;
		const std::filesystem::path canonicalFilePath = std::filesystem::weakly_canonical(filePath, error);
		if (error) {
			return false;
		}

		const std::filesystem::path canonicalDirectoryPath = std::filesystem::weakly_canonical(directoryPath, error);
		if (error) {
			return false;
		}

		return
			canonicalFilePath.extension() == ".json" &&
			canonicalFilePath.parent_path() == canonicalDirectoryPath;
	}

	/// @brief Particle AssetのJsonファイルをTrashディレクトリへ移動する
	/// @param sourcePath 移動するJsonファイル
	/// @param assetDirectoryPath Particle Assetディレクトリ
	/// @param outTrashPath 移動先パスの出力先
	/// @return 移動に成功した場合はtrue
	bool MoveParticleAssetFileToTrash(
		const std::filesystem::path& sourcePath,
		const std::filesystem::path& assetDirectoryPath,
		std::filesystem::path& outTrashPath) {
		if (!IsParticleAssetFilePath(sourcePath, assetDirectoryPath)) {
			return false;
		}

		std::error_code error;
		if (!std::filesystem::exists(sourcePath, error) || error) {
			return false;
		}

		const std::filesystem::path trashDirectoryPath = assetDirectoryPath / ".trash";
		std::filesystem::create_directories(trashDirectoryPath, error);
		if (error) {
			return false;
		}

		const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
		const std::string trashFileName =
			PathToUtf8String(sourcePath.stem()) +
			"_" +
			std::to_string(timestamp) +
			PathToUtf8String(sourcePath.extension());
		try {
			outTrashPath = trashDirectoryPath / MakeUtf8Path(trashFileName);
		}
		catch (const std::system_error&) {
			return false;
		}
		std::filesystem::rename(sourcePath, outTrashPath, error);
		return !error;
	}

} // namespace

namespace MadoEngine::Particle {

	ParticleSystem3d& ParticleSystem3d::GetInstance() {
		static ParticleSystem3d instance;
		return instance;
	}

	void ParticleSystem3d::Initialize(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		MadoEngine::Render::PSORegistry* psoRegistry) {
		Finalize();
		renderer_.Initialize(device, commandList, psoRegistry);
		isInitialized_ = true;
		const std::size_t assetCount = LoadAssetsFromDirectory(assetDirectoryPath_);
		Logger::Output(
			"ParticleSystem3dを初期化しました。Asset数: " + std::to_string(assetCount),
			Logger::Level::Engine
		);
	}

	void ParticleSystem3d::Finalize() {
		while (!freeSlotIndices_.empty()) {
			freeSlotIndices_.pop();
		}
		effectSlots_.clear();
		assets_.clear();
		assetPaths_.clear();
		renderer_.Finalize();
		preparedSceneType_ = SceneType::None;
		isRenderDataPrepared_ = false;
		isInitialized_ = false;
	}

	std::size_t ParticleSystem3d::LoadAssetsFromDirectory(const std::filesystem::path& directoryPath) {
		std::error_code error;
		if (!std::filesystem::exists(directoryPath, error)) {
			Logger::Output("Particle Assetディレクトリが見つかりません: " + PathToUtf8String(directoryPath), Logger::Level::Warning);
			return 0;
		}

		std::size_t loadCount = 0;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath, error)) {
			if (error) {
				break;
			}
			if (!entry.is_regular_file() || entry.path().extension() != ".json") {
				continue;
			}
			if (LoadAsset(entry.path())) {
				++loadCount;
			}
		}

		return loadCount;
	}

	bool ParticleSystem3d::LoadAsset(const std::filesystem::path& filePath) {
		auto asset = std::make_shared<ParticleEffectAsset>();
		if (!asset->LoadFromFile(filePath)) {
			return false;
		}

		const std::string assetName = asset->GetName().empty()
			? PathToUtf8String(filePath.stem())
			: asset->GetName();
		assets_[assetName] = std::move(asset);
		assetPaths_[assetName] = filePath;
		Logger::Output("Particle Assetを登録しました: " + assetName, Logger::Level::Assets);
		return true;
	}

	bool ParticleSystem3d::ReloadAsset(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		if (found == assetPaths_.end()) {
			Logger::Output("再読み込み対象のParticle Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}

		return LoadAsset(found->second);
	}

	bool ParticleSystem3d::CreateAsset(const std::string& assetName) {
		if (!IsAssetNameAvailable(assetName)) {
			Logger::Output("使用できないParticle Asset名です: " + assetName, Logger::Level::Warning);
			return false;
		}

		ParticleEffectAsset asset;
		asset.SetName(assetName);
		asset.GetEmitters().push_back({});
		asset.Validate();
		std::filesystem::path filePath;
		if (!TryMakeParticleAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			Logger::Output("Particle Assetのファイルパスを生成できません: " + assetName, Logger::Level::Error);
			return false;
		}
		if (!asset.SaveToFile(filePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<ParticleEffectAsset>();
		if (!loadedAsset->LoadFromFile(filePath)) {
			std::error_code error;
			std::filesystem::remove(filePath, error);
			return false;
		}

		assets_[assetName] = std::move(loadedAsset);
		assetPaths_[assetName] = filePath;
		Logger::Output("Particle Assetを作成しました: " + assetName, Logger::Level::Assets);
		return true;
	}

	bool ParticleSystem3d::DuplicateAsset(
		const std::string& sourceAssetName,
		const std::string& newAssetName) {
		const auto source = assets_.find(sourceAssetName);
		if (source == assets_.end()) {
			Logger::Output("複製元のParticle Assetが見つかりません: " + sourceAssetName, Logger::Level::Warning);
			return false;
		}
		if (!IsAssetNameAvailable(newAssetName)) {
			Logger::Output("複製先に使用できないParticle Asset名です: " + newAssetName, Logger::Level::Warning);
			return false;
		}

		ParticleEffectAsset duplicatedAsset = *source->second;
		duplicatedAsset.SetName(newAssetName);
		duplicatedAsset.Validate();
		std::filesystem::path filePath;
		if (!TryMakeParticleAssetFilePath(assetDirectoryPath_, newAssetName, filePath)) {
			Logger::Output("複製先のファイルパスを生成できません: " + newAssetName, Logger::Level::Error);
			return false;
		}
		if (!duplicatedAsset.SaveToFile(filePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<ParticleEffectAsset>();
		if (!loadedAsset->LoadFromFile(filePath)) {
			std::error_code error;
			std::filesystem::remove(filePath, error);
			return false;
		}

		assets_[newAssetName] = std::move(loadedAsset);
		assetPaths_[newAssetName] = filePath;
		Logger::Output(
			"Particle Assetを複製しました: " + sourceAssetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool ParticleSystem3d::RenameAsset(
		const std::string& assetName,
		const std::string& newAssetName) {
		const auto sourceAsset = assets_.find(assetName);
		const auto sourcePath = assetPaths_.find(assetName);
		if (sourceAsset == assets_.end() || sourcePath == assetPaths_.end()) {
			Logger::Output("名前変更対象のParticle Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}
		if (!IsAssetNameAvailable(newAssetName)) {
			Logger::Output("変更後に使用できないParticle Asset名です: " + newAssetName, Logger::Level::Warning);
			return false;
		}
		if (!IsParticleAssetFilePath(sourcePath->second, assetDirectoryPath_)) {
			Logger::Output("Particle Assetディレクトリ外のファイルは名前変更できません。", Logger::Level::Warning);
			return false;
		}

		ParticleEffectAsset renamedAsset = *sourceAsset->second;
		renamedAsset.SetName(newAssetName);
		renamedAsset.Validate();
		std::filesystem::path newFilePath;
		if (!TryMakeParticleAssetFilePath(assetDirectoryPath_, newAssetName, newFilePath)) {
			Logger::Output("変更後のファイルパスを生成できません: " + newAssetName, Logger::Level::Error);
			return false;
		}
		if (!renamedAsset.SaveToFile(newFilePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<ParticleEffectAsset>();
		if (!loadedAsset->LoadFromFile(newFilePath)) {
			std::error_code error;
			std::filesystem::remove(newFilePath, error);
			return false;
		}

		std::filesystem::path trashPath;
		if (!MoveParticleAssetFileToTrash(sourcePath->second, assetDirectoryPath_, trashPath)) {
			std::error_code error;
			std::filesystem::remove(newFilePath, error);
			Logger::Output("名前変更前のParticle Assetを退避できませんでした。", Logger::Level::Error);
			return false;
		}

		assets_.erase(sourceAsset);
		assetPaths_.erase(sourcePath);
		assets_[newAssetName] = std::move(loadedAsset);
		assetPaths_[newAssetName] = newFilePath;
		Logger::Output(
			"Particle Asset名を変更しました: " + assetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool ParticleSystem3d::DeleteAsset(const std::string& assetName) {
		const auto asset = assets_.find(assetName);
		const auto filePath = assetPaths_.find(assetName);
		if (asset == assets_.end() || filePath == assetPaths_.end()) {
			Logger::Output("削除対象のParticle Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}

		std::filesystem::path trashPath;
		if (!MoveParticleAssetFileToTrash(filePath->second, assetDirectoryPath_, trashPath)) {
			Logger::Output("Particle AssetをTrashへ退避できませんでした: " + assetName, Logger::Level::Error);
			return false;
		}

		assets_.erase(asset);
		assetPaths_.erase(filePath);
		Logger::Output(
			"Particle Assetを削除してTrashへ退避しました: " + PathToUtf8String(trashPath),
			Logger::Level::Assets
		);
		return true;
	}

	bool ParticleSystem3d::IsAssetNameAvailable(const std::string& assetName) const {
		if (!IsValidParticleAssetName(assetName) || assets_.contains(assetName)) {
			return false;
		}

		std::filesystem::path filePath;
		if (!TryMakeParticleAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			return false;
		}

		std::error_code error;
		const bool exists = std::filesystem::exists(filePath, error);
		return !error && !exists;
	}

	EffectHandle ParticleSystem3d::Play(const std::string& assetName, const PlayDesc& desc) {
		const auto found = assets_.find(assetName);
		if (found == assets_.end()) {
			Logger::Output("再生するParticle Assetが見つかりません: " + assetName, Logger::Level::Warning);
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

		PlayDesc resolvedDesc = desc;
		if (resolvedDesc.randomSeed == 0) {
			resolvedDesc.randomSeed = MyRand::CreateSeed();
		}

		EffectSlot& slot = effectSlots_[slotIndex];
		slot.instance = std::make_unique<ParticleEffectInstance>();
		slot.instance->Initialize(found->second, resolvedDesc);
		isRenderDataPrepared_ = false;
		return { slotIndex, slot.generation };
	}

	void ParticleSystem3d::Stop(EffectHandle handle, StopMode mode) {
		if (ParticleEffectInstance* instance = Resolve(handle)) {
			instance->Stop(mode);
			isRenderDataPrepared_ = false;
		}
	}

	bool ParticleSystem3d::SetTransform(EffectHandle handle, const Transform3D& transform) {
		ParticleEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}

		instance->SetTransform(transform);
		isRenderDataPrepared_ = false;
		return true;
	}

	bool ParticleSystem3d::IsAlive(EffectHandle handle) const {
		return Resolve(handle) != nullptr;
	}

	void ParticleSystem3d::Update(float deltaTime) {
		if (!isInitialized_) {
			return;
		}

		isRenderDataPrepared_ = false;
		const float safeDeltaTime = std::clamp(deltaTime, 0.0f, 0.1f);
		for (uint32_t index = 0; index < effectSlots_.size(); ++index) {
			EffectSlot& slot = effectSlots_[index];
			if (!slot.instance) {
				continue;
			}

			slot.instance->Update(safeDeltaTime);
			if (slot.instance->IsFinished()) {
				ReleaseSlot(index);
			}
		}
	}

	void ParticleSystem3d::DrawLayerMask(
		SceneType sceneType,
		const Camera& camera,
		MadoEngine::Render::RenderLayerMask layerMask) {
		if (!isInitialized_ || layerMask == 0) {
			return;
		}

		if (!isRenderDataPrepared_ || preparedSceneType_ != sceneType) {
			renderer_.Begin(camera);
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

	void ParticleSystem3d::ClearScene(SceneType sceneType) {
		if (sceneType == SceneType::None) {
			return;
		}

		for (uint32_t index = 0; index < effectSlots_.size(); ++index) {
			EffectSlot& slot = effectSlots_[index];
			if (!slot.instance || slot.instance->GetSceneType() != sceneType) {
				continue;
			}
			ReleaseSlot(index);
		}
		isRenderDataPrepared_ = false;
	}

	void ParticleSystem3d::StopAll(StopMode mode) {
		for (EffectSlot& slot : effectSlots_) {
			if (slot.instance) {
				slot.instance->Stop(mode);
			}
		}
		isRenderDataPrepared_ = false;
	}

	std::vector<std::string> ParticleSystem3d::GetAssetNames() const {
		std::vector<std::string> names;
		names.reserve(assets_.size());
		for (const auto& [name, asset] : assets_) {
			(void)asset;
			names.push_back(name);
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	const ParticleEffectAsset* ParticleSystem3d::FindAsset(const std::string& assetName) const {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	ParticleEffectAsset* ParticleSystem3d::FindEditableAsset(const std::string& assetName) {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	std::size_t ParticleSystem3d::GetActiveEffectCount() const {
		return static_cast<std::size_t>(std::count_if(effectSlots_.begin(), effectSlots_.end(), [](const EffectSlot& slot) {
			return slot.instance != nullptr;
		}));
	}

	std::size_t ParticleSystem3d::GetAliveParticleCount() const {
		std::size_t count = 0;
		for (const EffectSlot& slot : effectSlots_) {
			if (slot.instance) {
				count += slot.instance->GetAliveCount();
			}
		}
		return count;
	}

	ParticleEffectInstance* ParticleSystem3d::Resolve(EffectHandle handle) {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}

		EffectSlot& slot = effectSlots_[handle.index];
		if (slot.generation != handle.generation) {
			return nullptr;
		}
		return slot.instance.get();
	}

	const ParticleEffectInstance* ParticleSystem3d::Resolve(EffectHandle handle) const {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}

		const EffectSlot& slot = effectSlots_[handle.index];
		if (slot.generation != handle.generation) {
			return nullptr;
		}
		return slot.instance.get();
	}

	void ParticleSystem3d::ReleaseSlot(uint32_t index) {
		EffectSlot& slot = effectSlots_[index];
		if (!slot.instance) {
			return;
		}

		slot.instance.reset();
		++slot.generation;
		if (slot.generation == 0) {
			slot.generation = 1;
		}
		freeSlotIndices_.push(index);
	}

} // namespace MadoEngine::Particle
