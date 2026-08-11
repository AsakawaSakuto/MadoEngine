#pragma once
#include "BeamEffectInstance.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectRenderer3d.h"
#include <filesystem>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine::Beam {

	/// @brief Beam Assetと再生中Instanceを一元管理するSystem
	class BeamEffectSystem3d final {
	public:
		/// @brief Singleton Instanceを取得
		/// @return BeamEffectSystem3d Instance
		static BeamEffectSystem3d& GetInstance();

		/// @brief SystemのCopy構築を禁止
		/// @param other Copy元System
		BeamEffectSystem3d(const BeamEffectSystem3d& other) = delete;

		/// @brief SystemのCopy代入を禁止
		/// @param other Copy元System
		/// @return 代入結果
		BeamEffectSystem3d& operator=(const BeamEffectSystem3d& other) = delete;

		/// @brief Systemを初期化
		/// @param device D3D12 Device
		/// @param commandList 描画Command List
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Systemを終了
		void Finalize();

		/// @brief Directory直下のBeam Assetを読み込み
		/// @param directoryPath Asset Directory
		/// @return 読み込み成功数
		std::size_t LoadAssetsFromDirectory(const std::filesystem::path& directoryPath);

		/// @brief Beam Assetを読み込んで登録
		/// @param filePath 読み込むJsonファイル
		/// @return 登録に成功した場合はtrue
		bool LoadAsset(const std::filesystem::path& filePath);

		/// @brief 登録済みAssetを保存先から再読み込み
		/// @param assetName 再読み込み対象Asset名
		/// @return 再読み込みに成功した場合はtrue
		bool ReloadAsset(const std::string& assetName);

		/// @brief AssetのBackupを編集状態へ読み込み
		/// @param assetName Backup読み込み対象Asset名
		/// @return Backup読み込みに成功した場合はtrue
		bool LoadAssetBackup(const std::string& assetName);

		/// @brief 新規Beam Assetを作成
		/// @param assetName 作成するAsset名
		/// @return 作成に成功した場合はtrue
		bool CreateAsset(const std::string& assetName);

		/// @brief Beam Assetを複製
		/// @param sourceAssetName 複製元Asset名
		/// @param newAssetName 複製先Asset名
		/// @return 複製に成功した場合はtrue
		bool DuplicateAsset(const std::string& sourceAssetName, const std::string& newAssetName);

		/// @brief Beam Asset名とJsonファイル名を変更
		/// @param assetName 変更元Asset名
		/// @param newAssetName 変更後Asset名
		/// @return 変更に成功した場合はtrue
		bool RenameAsset(const std::string& assetName, const std::string& newAssetName);

		/// @brief Beam Assetを登録解除してTrashへ退避
		/// @param assetName 削除対象Asset名
		/// @return 退避と登録解除に成功した場合はtrue
		bool DeleteAsset(const std::string& assetName);

		/// @brief Asset名が新規作成に使用できるか確認
		/// @param assetName 確認対象名
		/// @return 使用可能な場合はtrue
		bool IsAssetNameAvailable(const std::string& assetName) const;

		/// @brief Beam Effectを再生
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生中Instance Handle
		BeamEffectHandle Play(const std::string& assetName, const BeamEffectPlayDesc& desc = {});

		/// @brief Beam Effectを停止
		/// @param handle 停止対象Handle
		/// @param mode 停止方式
		void Stop(BeamEffectHandle handle, BeamStopMode mode = BeamStopMode::Finish);

		/// @brief 指定したBeam Effectを一時停止
		/// @param handle 一時停止するEffect Handle
		/// @return 一時停止できた場合はtrue
		bool Pause(BeamEffectHandle handle);

		/// @brief 指定したBeam Effectを再開
		/// @param handle 再開するEffect Handle
		/// @return 再開できた場合はtrue
		bool Resume(BeamEffectHandle handle);

		/// @brief 指定したBeam Effectの再生速度を設定
		/// @param handle 設定するEffect Handle
		/// @param playbackSpeed 再生速度
		/// @return 設定できた場合はtrue
		bool SetPlaybackSpeed(BeamEffectHandle handle, float playbackSpeed);

		/// @brief 指定したBeam Effectが一時停止中か確認
		/// @param handle 確認するEffect Handle
		/// @return 一時停止中の場合はtrue
		bool IsPaused(BeamEffectHandle handle) const;

		/// @brief 始点と終点を更新
		/// @param handle 更新対象Handle
		/// @param startPosition 新しい始点
		/// @param endPosition 新しい終点
		/// @return 更新に成功した場合はtrue
		bool SetEndpoints(
			BeamEffectHandle handle,
			const Vector3& startPosition,
			const Vector3& endPosition
		);

		/// @brief Beam Effectの表示状態を設定
		/// @param handle 設定するEffect Handle
		/// @param isVisible 表示する場合はtrue
		/// @return 設定できた場合はtrue
		bool SetVisible(BeamEffectHandle handle, bool isVisible);

		/// @brief 始点を更新
		/// @param handle 更新対象Handle
		/// @param position 新しい始点
		/// @return 更新に成功した場合はtrue
		bool SetStartPosition(BeamEffectHandle handle, const Vector3& position);

		/// @brief 終点を更新
		/// @param handle 更新対象Handle
		/// @param position 新しい終点
		/// @return 更新に成功した場合はtrue
		bool SetEndPosition(BeamEffectHandle handle, const Vector3& position);

		/// @brief Handleが現在も有効か確認
		/// @param handle 確認対象Handle
		/// @return 有効な場合はtrue
		bool IsAlive(BeamEffectHandle handle) const;

		/// @brief 再生中Beamを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 今回の描画Commandに対応するFence値を設定
		/// @param submissionFenceValue 次回提出Fence値
		void BeginFrame(uint64_t submissionFenceValue);

		/// @brief 対象SceneとLayerのBeamを描画
		/// @param sceneType 現在Scene
		/// @param camera 描画Camera
		/// @param layerMask 描画対象Layer Mask
		void DrawLayerMask(
			SceneType sceneType,
			const Camera& camera,
			MadoEngine::Render::RenderLayerMask layerMask
		);

		/// @brief GPU完了済みFence値をRendererへ通知
		/// @param completedFenceValue GPU完了済みFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue);

		/// @brief 指定Sceneに属するBeamを破棄
		/// @param sceneType 破棄対象Scene
		void ClearScene(SceneType sceneType);

		/// @brief 全Beamを停止
		/// @param mode 停止方式
		void StopAll(BeamStopMode mode = BeamStopMode::Immediate);

		/// @brief 登録済みAsset名一覧を取得
		/// @return 名前順のAsset名一覧
		std::vector<std::string> GetAssetNames() const;

		/// @brief 登録済みAssetを取得
		/// @param assetName 取得対象Asset名
		/// @return Asset、存在しない場合はnullptr
		const BeamEffectAsset* FindAsset(const std::string& assetName) const;

		/// @brief 編集可能な登録済みAssetを取得
		/// @param assetName 取得対象Asset名
		/// @return Asset、存在しない場合はnullptr
		BeamEffectAsset* FindEditableAsset(const std::string& assetName);

		/// @brief 再生中Beam数を取得
		/// @return 再生中Instance数
		std::size_t GetActiveEffectCount() const;

	private:
		struct EffectSlot {
			std::unique_ptr<BeamEffectInstance> instance;
			uint32_t generation = 1;
			bool isVisible = true;
		};

		/// @brief Singleton Systemを構築
		BeamEffectSystem3d() = default;

		/// @brief Singleton Systemを破棄
		~BeamEffectSystem3d() = default;

		/// @brief HandleからInstanceを取得
		/// @param handle 取得対象Handle
		/// @return Instance、無効な場合はnullptr
		BeamEffectInstance* Resolve(BeamEffectHandle handle);

		/// @brief HandleからInstanceを取得
		/// @param handle 取得対象Handle
		/// @return Instance、無効な場合はnullptr
		const BeamEffectInstance* Resolve(BeamEffectHandle handle) const;

		/// @brief Slotを解放して世代を更新
		/// @param index 解放対象Slot Index
		void ReleaseSlot(uint32_t index);

		MadoEngine::Ribbon::RibbonEffectRenderer3d renderer_;
		std::unordered_map<std::string, std::shared_ptr<BeamEffectAsset>> assets_;
		std::unordered_map<std::string, std::filesystem::path> assetPaths_;
		std::filesystem::path assetDirectoryPath_ = "Assets/Json/BeamEffect";
		std::vector<EffectSlot> effectSlots_;
		std::queue<uint32_t> freeSlotIndices_;
		SceneType preparedSceneType_ = SceneType::None;
		uint64_t currentSubmissionFenceValue_ = 0;
		bool isRenderDataPrepared_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Beam
