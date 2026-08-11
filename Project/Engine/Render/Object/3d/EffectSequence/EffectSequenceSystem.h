#pragma once
#include "EffectSequenceInstance.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine::EffectSequence {

	/// @brief Effect Sequence Assetと再生中Instanceを一元管理するSystem
	class EffectSequenceSystem final {
	public:
		/// @brief Singleton Instanceを取得
		/// @return EffectSequenceSystem Instance
		static EffectSequenceSystem& GetInstance();

		/// @brief Copy構築を禁止
		EffectSequenceSystem(const EffectSequenceSystem&) = delete;

		/// @brief Copy代入を禁止
		/// @return 自身
		EffectSequenceSystem& operator=(const EffectSequenceSystem&) = delete;

		/// @brief Move構築を禁止
		EffectSequenceSystem(EffectSequenceSystem&&) = delete;

		/// @brief Move代入を禁止
		/// @return 自身
		EffectSequenceSystem& operator=(EffectSequenceSystem&&) = delete;

		/// @brief Sequence Systemを初期化してAssetを読み込み
		void Initialize();

		/// @brief 全子Effectを停止してSequence Systemを終了
		void Finalize();

		/// @brief Directory直下のSequence Assetを読み込み
		/// @param directoryPath Asset Directory
		/// @return 読み込みに成功したAsset数
		std::size_t LoadAssetsFromDirectory(const std::filesystem::path& directoryPath);

		/// @brief Sequence Assetを読み込んで登録
		/// @param filePath 読み込むJSONファイル
		/// @return 登録に成功した場合はtrue
		bool LoadAsset(const std::filesystem::path& filePath);

		/// @brief 登録済みSequence Assetを再読み込み
		/// @param assetName 再読み込みするAsset名
		/// @return 再読み込みに成功した場合はtrue
		bool ReloadAsset(const std::string& assetName);

		/// @brief Sequence AssetのBackupを編集状態へ読み込み
		/// @param assetName Backupを読み込むAsset名
		/// @return 読み込みに成功した場合はtrue
		bool LoadAssetBackup(const std::string& assetName);

		/// @brief 新規Sequence Assetを作成
		/// @param assetName 作成するAsset名
		/// @return 作成に成功した場合はtrue
		bool CreateAsset(const std::string& assetName);

		/// @brief Sequence Assetを複製
		/// @param sourceAssetName 複製元Asset名
		/// @param newAssetName 複製先Asset名
		/// @return 複製に成功した場合はtrue
		bool DuplicateAsset(const std::string& sourceAssetName, const std::string& newAssetName);

		/// @brief Sequence Asset名とJSONファイル名を変更
		/// @param assetName 変更元Asset名
		/// @param newAssetName 変更後Asset名
		/// @return 変更に成功した場合はtrue
		bool RenameAsset(const std::string& assetName, const std::string& newAssetName);

		/// @brief Sequence Assetを登録解除してJSONをTrashへ退避
		/// @param assetName 削除するAsset名
		/// @return 退避と登録解除に成功した場合はtrue
		bool DeleteAsset(const std::string& assetName);

		/// @brief Asset名が新規作成に使用できるか確認
		/// @param assetName 確認するAsset名
		/// @return 使用可能な場合はtrue
		bool IsAssetNameAvailable(const std::string& assetName) const;

		/// @brief Effect Sequenceを再生
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生中Instance Handle
		EffectSequenceHandle Play(
			const std::string& assetName,
			const EffectSequencePlayDesc& desc = {}
		);

		/// @brief Effect Sequenceを停止
		/// @param handle 停止するSequence Handle
		/// @param mode 停止方式
		void Stop(
			EffectSequenceHandle handle,
			EffectSequenceStopMode mode = EffectSequenceStopMode::Finish
		);

		/// @brief Effect Sequenceを一時停止
		/// @param handle 一時停止するSequence Handle
		/// @return 一時停止できた場合はtrue
		bool Pause(EffectSequenceHandle handle);

		/// @brief Effect Sequenceを再開
		/// @param handle 再開するSequence Handle
		/// @return 再開できた場合はtrue
		bool Resume(EffectSequenceHandle handle);

		/// @brief Sequence Root Transformを更新
		/// @param handle 更新するSequence Handle
		/// @param transform 新しいRoot Transform
		/// @return 更新できた場合はtrue
		bool SetTransform(EffectSequenceHandle handle, const Transform3D& transform);

		/// @brief Sequenceと子Effectの表示状態を設定
		/// @param handle 設定するSequence Handle
		/// @param isVisible 表示する場合はtrue
		/// @return 設定できた場合はtrue
		bool SetVisible(EffectSequenceHandle handle, bool isVisible);

		/// @brief Sequence再生速度を設定
		/// @param handle 設定するSequence Handle
		/// @param playbackSpeed 再生速度
		/// @return 設定できた場合はtrue
		bool SetPlaybackSpeed(EffectSequenceHandle handle, float playbackSpeed);

		/// @brief Sequence Handleが現在有効か確認
		/// @param handle 確認するSequence Handle
		/// @return 有効な場合はtrue
		bool IsAlive(EffectSequenceHandle handle) const;

		/// @brief Sequenceが一時停止中か確認
		/// @param handle 確認するSequence Handle
		/// @return 一時停止中の場合はtrue
		bool IsPaused(EffectSequenceHandle handle) const;

		/// @brief Sequenceの現在再生時間を取得
		/// @param handle 確認するSequence Handle
		/// @return 有効なHandleの場合は再生時間
		std::optional<float> GetPlaybackTime(EffectSequenceHandle handle) const;

		/// @brief 全Sequenceを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 指定SceneのSequenceと所有子Effectを破棄
		/// @param sceneType 破棄対象Scene
		void ClearScene(SceneType sceneType);

		/// @brief 全Sequenceを停止
		/// @param mode 停止方式
		void StopAll(EffectSequenceStopMode mode = EffectSequenceStopMode::Immediate);

		/// @brief 登録済みAsset名一覧を取得
		/// @return 名前順のAsset名一覧
		std::vector<std::string> GetAssetNames() const;

		/// @brief 登録済みSequence Assetを取得
		/// @param assetName 取得するAsset名
		/// @return Asset、存在しない場合はnullptr
		const EffectSequenceAsset* FindAsset(const std::string& assetName) const;

		/// @brief 編集可能なSequence Assetを取得
		/// @param assetName 取得するAsset名
		/// @return Asset、存在しない場合はnullptr
		EffectSequenceAsset* FindEditableAsset(const std::string& assetName);

		/// @brief 再生中Sequence数を取得
		/// @return 再生中Sequence数
		std::size_t GetActiveSequenceCount() const;

		/// @brief 未消費の再生終了イベントを取得してQueueをクリア
		/// @return 再生終了イベント一覧
		std::vector<EffectSequenceFinishedEvent> ConsumeFinishedEvents();

	private:
		struct SequenceSlot {
			std::unique_ptr<EffectSequenceInstance> instance;
			uint32_t generation = 1;
		};

		/// @brief Singleton Systemを構築
		EffectSequenceSystem() = default;

		/// @brief Singleton Systemを破棄
		~EffectSequenceSystem() = default;

		/// @brief HandleからInstanceを取得
		/// @param handle 取得対象Handle
		/// @return Instance、無効な場合はnullptr
		EffectSequenceInstance* Resolve(EffectSequenceHandle handle);

		/// @brief HandleからInstanceを取得
		/// @param handle 取得対象Handle
		/// @return Instance、無効な場合はnullptr
		const EffectSequenceInstance* Resolve(EffectSequenceHandle handle) const;

		/// @brief 終了イベントをQueueへ追加してSlotを解放
		/// @param index 解放対象Slot Index
		/// @param emitEvent 終了イベントを発行する場合はtrue
		void CompleteAndReleaseSlot(uint32_t index, bool emitEvent);

		/// @brief Slotを解放して世代を更新
		/// @param index 解放対象Slot Index
		void ReleaseSlot(uint32_t index);

		EffectSequenceNodeDispatcher dispatcher_;
		std::unordered_map<std::string, std::shared_ptr<EffectSequenceAsset>> assets_;
		std::unordered_map<std::string, std::filesystem::path> assetPaths_;
		std::filesystem::path assetDirectoryPath_ = "Assets/Json/EffectSequence";
		std::vector<SequenceSlot> sequenceSlots_;
		std::queue<uint32_t> freeSlotIndices_;
		std::vector<EffectSequenceFinishedEvent> finishedEvents_;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::EffectSequence
