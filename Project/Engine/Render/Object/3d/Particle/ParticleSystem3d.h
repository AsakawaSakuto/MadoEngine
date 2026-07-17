#pragma once
#include "ParticleEffectInstance.h"
#include "ParticleRenderer3d.h"
#include <filesystem>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine::Particle {

	/// @brief 3D Particle Assetと再生中Effectを一元管理するSystem
	class ParticleSystem3d final {
	public:
		/// @brief Singleton Instanceを取得する
		/// @return ParticleSystem3dのInstance
		static ParticleSystem3d& GetInstance();

		ParticleSystem3d(const ParticleSystem3d&) = delete;
		ParticleSystem3d& operator=(const ParticleSystem3d&) = delete;
		ParticleSystem3d(ParticleSystem3d&&) = delete;
		ParticleSystem3d& operator=(ParticleSystem3d&&) = delete;

		/// @brief Particle Systemを初期化する
		/// @param device D3D12Device
		/// @param commandList 描画に使用するCommandList
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Particle Systemを終了する
		void Finalize();

		/// @brief 指定ディレクトリ内のParticle Assetを読み込む
		/// @param directoryPath 読み込み対象ディレクトリ
		/// @return 読み込みに成功したAsset数
		std::size_t LoadAssetsFromDirectory(const std::filesystem::path& directoryPath);

		/// @brief Particle Assetを読み込んで登録する
		/// @param filePath 読み込むJsonファイル
		/// @return 登録に成功した場合はtrue
		bool LoadAsset(const std::filesystem::path& filePath);

		/// @brief 登録済みParticle Assetを再読み込みする
		/// @param assetName 再読み込みするAsset名
		/// @return 再読み込みに成功した場合はtrue
		bool ReloadAsset(const std::string& assetName);

		/// @brief 新規Particle Assetを作成する
		/// @param assetName 作成するAsset名
		/// @return 作成に成功した場合はtrue
		bool CreateAsset(const std::string& assetName);

		/// @brief Particle Assetを複製する
		/// @param sourceAssetName 複製元Asset名
		/// @param newAssetName 複製先Asset名
		/// @return 複製に成功した場合はtrue
		bool DuplicateAsset(const std::string& sourceAssetName, const std::string& newAssetName);

		/// @brief Particle Assetの名前とJsonファイル名を変更する
		/// @param assetName 変更元Asset名
		/// @param newAssetName 変更後Asset名
		/// @return 変更に成功した場合はtrue
		bool RenameAsset(const std::string& assetName, const std::string& newAssetName);

		/// @brief Particle Assetを登録解除してJsonファイルを退避する
		/// @param assetName 削除するAsset名
		/// @return 退避と登録解除に成功した場合はtrue
		bool DeleteAsset(const std::string& assetName);

		/// @brief 新規Asset名として使用できるか確認する
		/// @param assetName 確認するAsset名
		/// @return 名前と保存先が使用可能な場合はtrue
		bool IsAssetNameAvailable(const std::string& assetName) const;

		/// @brief Particle Effectを再生する
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生中Effectを指すHandle
		EffectHandle Play(const std::string& assetName, const PlayDesc& desc = {});

		/// @brief Particle Effectを停止する
		/// @param handle 停止するEffect Handle
		/// @param mode 停止方法
		void Stop(EffectHandle handle, StopMode mode = StopMode::Finish);

		/// @brief Particle EffectのTransformを変更する
		/// @param handle 変更するEffect Handle
		/// @param transform 設定するTransform
		/// @return 変更に成功した場合はtrue
		bool SetTransform(EffectHandle handle, const Transform3D& transform);

		/// @brief Effect Handleが現在も有効か確認する
		/// @param handle 確認するEffect Handle
		/// @return 有効な場合はtrue
		bool IsAlive(EffectHandle handle) const;

		/// @brief 再生中の全Particle Effectを更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 描画条件に一致するParticleを描画する
		/// @param sceneType 現在のScene種別
		/// @param camera 描画に使用するCamera
		/// @param layerMask 描画対象LayerMask
		void DrawLayerMask(
			SceneType sceneType,
			const Camera& camera,
			MadoEngine::Render::RenderLayerMask layerMask
		);

		/// @brief 指定Sceneに属するEffectを即時停止する
		/// @param sceneType 停止対象Scene
		void ClearScene(SceneType sceneType);

		/// @brief 全Effectを停止する
		/// @param mode 停止方法
		void StopAll(StopMode mode = StopMode::Immediate);

		/// @brief 登録済みAsset名一覧を取得する
		/// @return 名前順に並べたAsset名一覧
		std::vector<std::string> GetAssetNames() const;

		/// @brief Particle Assetを取得する
		/// @param assetName 取得するAsset名
		/// @return Asset。存在しない場合はnullptr
		const ParticleEffectAsset* FindAsset(const std::string& assetName) const;

		/// @brief 編集可能なParticle Assetを取得する
		/// @param assetName 取得するAsset名
		/// @return Asset。存在しない場合はnullptr
		ParticleEffectAsset* FindEditableAsset(const std::string& assetName);

		/// @brief 再生中Effect数を取得する
		/// @return 再生中Effect数
		std::size_t GetActiveEffectCount() const;

		/// @brief 生存Particle総数を取得する
		/// @return 生存Particle総数
		std::size_t GetAliveParticleCount() const;

	private:
		struct EffectSlot {
			std::unique_ptr<ParticleEffectInstance> instance;
			uint32_t generation = 1;
		};

		ParticleSystem3d() = default;
		~ParticleSystem3d() = default;

		/// @brief HandleからEffect Instanceを取得する
		/// @param handle 取得するEffect Handle
		/// @return Effect Instance。無効な場合はnullptr
		ParticleEffectInstance* Resolve(EffectHandle handle);

		/// @brief HandleからEffect Instanceを取得する
		/// @param handle 取得するEffect Handle
		/// @return Effect Instance。無効な場合はnullptr
		const ParticleEffectInstance* Resolve(EffectHandle handle) const;

		/// @brief Slotを解放して再利用可能にする
		/// @param index 解放するSlot Index
		void ReleaseSlot(uint32_t index);

		ParticleRenderer3d renderer_;
		std::unordered_map<std::string, std::shared_ptr<ParticleEffectAsset>> assets_;
		std::unordered_map<std::string, std::filesystem::path> assetPaths_;
		std::filesystem::path assetDirectoryPath_ = "Assets/Json/Particle";
		std::vector<EffectSlot> effectSlots_;
		std::queue<uint32_t> freeSlotIndices_;
		SceneType preparedSceneType_ = SceneType::None;
		bool isRenderDataPrepared_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Particle
