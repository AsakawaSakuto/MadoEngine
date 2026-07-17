#include "PrimitiveEffectSystem3d.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <limits>
#include <string_view>
#include <system_error>

namespace {

	constexpr std::size_t kMaximumCylinderAssetNameLength = 100;

	/// @brief UTF-8文字列からFilesystem Pathを生成する
	/// @param value UTF-8文字列
	/// @return 生成したFilesystem Path
	std::filesystem::path MakeUtf8Path(const std::string& value) {
		const auto* begin = reinterpret_cast<const char8_t*>(value.data());
		const auto* end = begin + value.size();
		return std::filesystem::path(std::u8string(begin, end));
	}

	/// @brief ファイルパスをUTF-8文字列へ変換する
	/// @param path 変換するファイルパス
	/// @return UTF-8文字列
	std::string PathToUtf8String(const std::filesystem::path& path) {
		const std::u8string value = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(value.data()), value.size());
	}

	/// @brief Cylinder Asset名からJsonファイルパスを安全に生成する
	/// @param directoryPath Cylinder Assetディレクトリ
	/// @param assetName UTF-8のCylinder Asset名
	/// @param outFilePath 生成したパスの出力先
	/// @return 生成に成功した場合はtrue
	bool TryMakeCylinderAssetFilePath(
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

	/// @brief Cylinder Asset名をJsonファイル名として使用できるか確認する
	/// @param assetName 確認するAsset名
	/// @return 使用できる場合はtrue
	bool IsValidCylinderAssetName(const std::string& assetName) {
		if (
			assetName.empty() ||
			assetName.size() > kMaximumCylinderAssetNameLength ||
			assetName.front() == ' ' ||
			assetName.back() == ' ' ||
			assetName.back() == '.') {
			return false;
		}

		constexpr std::string_view invalidCharacters = "<>:\"/\\|?*";
		for (const unsigned char character : assetName) {
			if (
				character < 32 ||
				invalidCharacters.find(static_cast<char>(character)) != std::string_view::npos) {
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

	/// @brief JsonファイルがCylinder Assetディレクトリ直下にあるか確認する
	/// @param filePath 確認するJsonファイル
	/// @param directoryPath Cylinder Assetディレクトリ
	/// @return 対象ディレクトリ直下のJsonファイルの場合はtrue
	bool IsCylinderAssetFilePath(
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

	/// @brief Cylinder AssetのJsonファイルをTrashディレクトリへ移動する
	/// @param sourcePath 移動するJsonファイル
	/// @param assetDirectoryPath Cylinder Assetディレクトリ
	/// @param outTrashPath 移動先パスの出力先
	/// @return 移動に成功した場合はtrue
	bool MoveCylinderAssetFileToTrash(
		const std::filesystem::path& sourcePath,
		const std::filesystem::path& assetDirectoryPath,
		std::filesystem::path& outTrashPath) {
		if (!IsCylinderAssetFilePath(sourcePath, assetDirectoryPath)) {
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

namespace MadoEngine::Effect {

	PrimitiveEffectSystem3d& PrimitiveEffectSystem3d::GetInstance() {
		static PrimitiveEffectSystem3d instance;
		return instance;
	}

	void PrimitiveEffectSystem3d::Initialize(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		MadoEngine::Render::PSORegistry* psoRegistry) {
		Finalize();
		renderer_.Initialize(device, commandList, psoRegistry);
		const std::size_t assetCount = LoadAssetsFromDirectory(assetDirectoryPath_);
		isInitialized_ = true;
		Logger::Output(
			"PrimitiveEffectSystem3dを初期化しました。Asset数: " + std::to_string(assetCount),
			Logger::Level::Engine
		);
	}

	void PrimitiveEffectSystem3d::Finalize() {
		renderer_.Finalize();
		assets_.clear();
		assetPaths_.clear();
		effectSlots_.clear();
		freeSlotIndices_ = {};
		preparedSceneType_ = SceneType::None;
		isRenderDataPrepared_ = false;
		isInitialized_ = false;
	}

	std::size_t PrimitiveEffectSystem3d::LoadAssetsFromDirectory(const std::filesystem::path& directoryPath) {
		assetDirectoryPath_ = directoryPath;
		std::error_code error;
		if (!std::filesystem::exists(directoryPath, error)) {
			Logger::Output(
				"Primitive Effect Assetディレクトリが見つかりません: " + PathToUtf8String(directoryPath),
				Logger::Level::Warning
			);
			return 0;
		}

		std::vector<std::filesystem::path> filePaths;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath, error)) {
			if (error) {
				break;
			}
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				filePaths.push_back(entry.path());
			}
		}
		std::sort(filePaths.begin(), filePaths.end());

		std::size_t loadedCount = 0;
		for (const std::filesystem::path& filePath : filePaths) {
			if (LoadAsset(filePath)) {
				++loadedCount;
			}
		}
		return loadedCount;
	}

	bool PrimitiveEffectSystem3d::LoadAsset(const std::filesystem::path& filePath) {
		auto asset = std::make_shared<CylinderEffectAsset>();
		if (!asset->LoadFromFile(filePath)) {
			return false;
		}

		const std::string assetName = asset->GetName();
		if (assetName.empty()) {
			Logger::Output("名前が空のCylinder Effect Assetは登録できません。", Logger::Level::Warning);
			return false;
		}
		assets_[assetName] = std::move(asset);
		assetPaths_[assetName] = filePath;
		Logger::Output("Cylinder Effect Assetを登録しました: " + assetName, Logger::Level::Assets);
		return true;
	}

	bool PrimitiveEffectSystem3d::ReloadAsset(const std::string& assetName) {
		const auto found = assetPaths_.find(assetName);
		if (found == assetPaths_.end()) {
			Logger::Output("再読み込み対象のCylinder Effect Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}
		const std::filesystem::path filePath = found->second;
		return LoadAsset(filePath);
	}

	bool PrimitiveEffectSystem3d::CreateAsset(const std::string& assetName) {
		if (!IsAssetNameAvailable(assetName)) {
			Logger::Output("使用できないCylinder Effect Asset名です: " + assetName, Logger::Level::Warning);
			return false;
		}

		CylinderEffectAsset asset;
		asset.SetName(assetName);
		asset.Validate();
		std::filesystem::path filePath;
		if (!TryMakeCylinderAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			Logger::Output("Cylinder Effect Assetのファイルパスを生成できません: " + assetName, Logger::Level::Error);
			return false;
		}
		if (!asset.SaveToFile(filePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<CylinderEffectAsset>();
		if (!loadedAsset->LoadFromFile(filePath)) {
			std::error_code error;
			std::filesystem::remove(filePath, error);
			return false;
		}

		assets_[assetName] = std::move(loadedAsset);
		assetPaths_[assetName] = filePath;
		Logger::Output("Cylinder Effect Assetを作成しました: " + assetName, Logger::Level::Assets);
		return true;
	}

	bool PrimitiveEffectSystem3d::DuplicateAsset(
		const std::string& sourceAssetName,
		const std::string& newAssetName) {
		const auto source = assets_.find(sourceAssetName);
		if (source == assets_.end()) {
			Logger::Output("複製元のCylinder Effect Assetが見つかりません: " + sourceAssetName, Logger::Level::Warning);
			return false;
		}
		if (!IsAssetNameAvailable(newAssetName)) {
			Logger::Output("複製先に使用できないCylinder Effect Asset名です: " + newAssetName, Logger::Level::Warning);
			return false;
		}

		CylinderEffectAsset duplicatedAsset = *source->second;
		duplicatedAsset.SetName(newAssetName);
		duplicatedAsset.Validate();
		std::filesystem::path filePath;
		if (!TryMakeCylinderAssetFilePath(assetDirectoryPath_, newAssetName, filePath)) {
			Logger::Output("複製先のファイルパスを生成できません: " + newAssetName, Logger::Level::Error);
			return false;
		}
		if (!duplicatedAsset.SaveToFile(filePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<CylinderEffectAsset>();
		if (!loadedAsset->LoadFromFile(filePath)) {
			std::error_code error;
			std::filesystem::remove(filePath, error);
			return false;
		}

		assets_[newAssetName] = std::move(loadedAsset);
		assetPaths_[newAssetName] = filePath;
		Logger::Output(
			"Cylinder Effect Assetを複製しました: " + sourceAssetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool PrimitiveEffectSystem3d::RenameAsset(
		const std::string& assetName,
		const std::string& newAssetName) {
		const auto sourceAsset = assets_.find(assetName);
		const auto sourcePath = assetPaths_.find(assetName);
		if (sourceAsset == assets_.end() || sourcePath == assetPaths_.end()) {
			Logger::Output("名前変更対象のCylinder Effect Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}
		if (!IsAssetNameAvailable(newAssetName)) {
			Logger::Output("変更後に使用できないCylinder Effect Asset名です: " + newAssetName, Logger::Level::Warning);
			return false;
		}
		if (!IsCylinderAssetFilePath(sourcePath->second, assetDirectoryPath_)) {
			Logger::Output("Cylinder Effect Assetディレクトリ外のファイルは名前変更できません。", Logger::Level::Warning);
			return false;
		}

		CylinderEffectAsset renamedAsset = *sourceAsset->second;
		renamedAsset.SetName(newAssetName);
		renamedAsset.Validate();
		std::filesystem::path newFilePath;
		if (!TryMakeCylinderAssetFilePath(assetDirectoryPath_, newAssetName, newFilePath)) {
			Logger::Output("変更後のファイルパスを生成できません: " + newAssetName, Logger::Level::Error);
			return false;
		}
		if (!renamedAsset.SaveToFile(newFilePath, false)) {
			return false;
		}

		auto loadedAsset = std::make_shared<CylinderEffectAsset>();
		if (!loadedAsset->LoadFromFile(newFilePath)) {
			std::error_code error;
			std::filesystem::remove(newFilePath, error);
			return false;
		}

		std::filesystem::path trashPath;
		if (!MoveCylinderAssetFileToTrash(sourcePath->second, assetDirectoryPath_, trashPath)) {
			std::error_code error;
			std::filesystem::remove(newFilePath, error);
			Logger::Output("名前変更前のCylinder Effect Assetを退避できませんでした。", Logger::Level::Error);
			return false;
		}

		assets_.erase(sourceAsset);
		assetPaths_.erase(sourcePath);
		assets_[newAssetName] = std::move(loadedAsset);
		assetPaths_[newAssetName] = newFilePath;
		Logger::Output(
			"Cylinder Effect Asset名を変更しました: " + assetName + " -> " + newAssetName,
			Logger::Level::Assets
		);
		return true;
	}

	bool PrimitiveEffectSystem3d::DeleteAsset(const std::string& assetName) {
		const auto asset = assets_.find(assetName);
		const auto filePath = assetPaths_.find(assetName);
		if (asset == assets_.end() || filePath == assetPaths_.end()) {
			Logger::Output("削除対象のCylinder Effect Assetが見つかりません: " + assetName, Logger::Level::Warning);
			return false;
		}

		std::filesystem::path trashPath;
		if (!MoveCylinderAssetFileToTrash(filePath->second, assetDirectoryPath_, trashPath)) {
			Logger::Output("Cylinder Effect AssetをTrashへ退避できませんでした: " + assetName, Logger::Level::Error);
			return false;
		}

		assets_.erase(asset);
		assetPaths_.erase(filePath);
		Logger::Output(
			"Cylinder Effect Assetを削除してTrashへ退避しました: " + PathToUtf8String(trashPath),
			Logger::Level::Assets
		);
		return true;
	}

	bool PrimitiveEffectSystem3d::IsAssetNameAvailable(const std::string& assetName) const {
		if (!IsValidCylinderAssetName(assetName) || assets_.contains(assetName)) {
			return false;
		}

		std::filesystem::path filePath;
		if (!TryMakeCylinderAssetFilePath(assetDirectoryPath_, assetName, filePath)) {
			return false;
		}

		std::error_code error;
		const bool exists = std::filesystem::exists(filePath, error);
		return !error && !exists;
	}

	PrimitiveEffectHandle PrimitiveEffectSystem3d::Play(
		const std::string& assetName,
		const PrimitiveEffectPlayDesc& desc) {
		const auto found = assets_.find(assetName);
		if (found == assets_.end()) {
			Logger::Output("再生するCylinder Effect Assetが見つかりません: " + assetName, Logger::Level::Warning);
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
		slot.instance = std::make_unique<CylinderEffectInstance>();
		slot.instance->Initialize(found->second, desc);
		isRenderDataPrepared_ = false;
		return { slotIndex, slot.generation };
	}

	void PrimitiveEffectSystem3d::Stop(
		PrimitiveEffectHandle handle,
		PrimitiveEffectStopMode mode) {
		if (CylinderEffectInstance* instance = Resolve(handle)) {
			instance->Stop(mode);
			isRenderDataPrepared_ = false;
		}
	}

	bool PrimitiveEffectSystem3d::SetTransform(
		PrimitiveEffectHandle handle,
		const Transform3D& transform) {
		CylinderEffectInstance* instance = Resolve(handle);
		if (!instance) {
			return false;
		}
		instance->SetTransform(transform);
		isRenderDataPrepared_ = false;
		return true;
	}

	bool PrimitiveEffectSystem3d::IsAlive(PrimitiveEffectHandle handle) const {
		return Resolve(handle) != nullptr;
	}

	void PrimitiveEffectSystem3d::Update(float deltaTime) {
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

	void PrimitiveEffectSystem3d::DrawLayerMask(
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

	void PrimitiveEffectSystem3d::ClearScene(SceneType sceneType) {
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

	void PrimitiveEffectSystem3d::StopAll(PrimitiveEffectStopMode mode) {
		for (EffectSlot& slot : effectSlots_) {
			if (slot.instance) {
				slot.instance->Stop(mode);
			}
		}
		isRenderDataPrepared_ = false;
	}

	std::vector<std::string> PrimitiveEffectSystem3d::GetAssetNames() const {
		std::vector<std::string> names;
		names.reserve(assets_.size());
		for (const auto& assetPair : assets_) {
			names.push_back(assetPair.first);
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	const CylinderEffectAsset* PrimitiveEffectSystem3d::FindAsset(const std::string& assetName) const {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	CylinderEffectAsset* PrimitiveEffectSystem3d::FindEditableAsset(const std::string& assetName) {
		const auto found = assets_.find(assetName);
		return found != assets_.end() ? found->second.get() : nullptr;
	}

	std::size_t PrimitiveEffectSystem3d::GetActiveEffectCount() const {
		return static_cast<std::size_t>(std::count_if(
			effectSlots_.begin(),
			effectSlots_.end(),
			[](const EffectSlot& slot) {
				return slot.instance != nullptr;
			}
		));
	}

	CylinderEffectInstance* PrimitiveEffectSystem3d::Resolve(PrimitiveEffectHandle handle) {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}
		EffectSlot& slot = effectSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	const CylinderEffectInstance* PrimitiveEffectSystem3d::Resolve(PrimitiveEffectHandle handle) const {
		if (!handle.HasValue() || handle.index >= effectSlots_.size()) {
			return nullptr;
		}
		const EffectSlot& slot = effectSlots_[handle.index];
		return slot.generation == handle.generation ? slot.instance.get() : nullptr;
	}

	void PrimitiveEffectSystem3d::ReleaseSlot(uint32_t index) {
		EffectSlot& slot = effectSlots_[index];
		slot.instance.reset();
		++slot.generation;
		if (slot.generation == 0) {
			slot.generation = 1;
		}
		freeSlotIndices_.push(index);
	}

} // namespace MadoEngine::Effect
