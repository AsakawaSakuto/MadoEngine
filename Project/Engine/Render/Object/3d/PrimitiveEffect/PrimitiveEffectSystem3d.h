#pragma once
#include "CylinderEffectInstance.h"
#include "CylinderEffectRenderer3d.h"
#include <filesystem>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine::Effect {

	/// @brief Cylinder Assetと再生中Primitive Effectを管理するSystem
	class PrimitiveEffectSystem3d final {
	public:
		/// @brief Singleton Instanceを取得
		/// @return PrimitiveEffectSystem3dのInstance
		static PrimitiveEffectSystem3d& GetInstance();

		PrimitiveEffectSystem3d(const PrimitiveEffectSystem3d&) = delete;
		PrimitiveEffectSystem3d& operator=(const PrimitiveEffectSystem3d&) = delete;
		PrimitiveEffectSystem3d(PrimitiveEffectSystem3d&&) = delete;
		PrimitiveEffectSystem3d& operator=(PrimitiveEffectSystem3d&&) = delete;

		/// @brief Systemを初期化
		/// @param device D3D12Device
		/// @param commandList 描画に使用するCommandList
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Systemを終了
		void Finalize();

		/// @brief ディレクトリ内のCylinder Assetを読み込み
		/// @param directoryPath 読み込み対象ディレクトリ
		/// @return 読み込みに成功したAsset数
		std::size_t LoadAssetsFromDirectory(const std::filesystem::path& directoryPath);

		/// @brief Cylinder Assetを読み込んで登録
		/// @param filePath 読み込むJsonファイル
		/// @return 登録に成功した場合はtrue
		bool LoadAsset(const std::filesystem::path& filePath);

		/// @brief 登録済みAssetを再読み込み
		/// @param assetName 再読み込みするAsset名
		/// @return 再読み込みに成功した場合はtrue
		bool ReloadAsset(const std::string& assetName);

		/// @brief 新規Cylinder Effect Assetを作成
		/// @param assetName 作成するAsset名
		/// @return 作成に成功した場合はtrue
		bool CreateAsset(const std::string& assetName);

		/// @brief Cylinder Effect Assetを複製
		/// @param sourceAssetName 複製元Asset名
		/// @param newAssetName 複製先Asset名
		/// @return 複製に成功した場合はtrue
		bool DuplicateAsset(const std::string& sourceAssetName, const std::string& newAssetName);

		/// @brief Cylinder Effect Assetの名前とJsonファイル名を変更
		/// @param assetName 変更元Asset名
		/// @param newAssetName 変更後Asset名
		/// @return 変更に成功した場合はtrue
		bool RenameAsset(const std::string& assetName, const std::string& newAssetName);

		/// @brief Cylinder Effect Assetを登録解除してJsonファイルを退避
		/// @param assetName 削除するAsset名
		/// @return 退避と登録解除に成功した場合はtrue
		bool DeleteAsset(const std::string& assetName);

		/// @brief 新規Asset名として使用できるか確認
		/// @param assetName 確認するAsset名
		/// @return 名前と保存先が使用可能な場合はtrue
		bool IsAssetNameAvailable(const std::string& assetName) const;

		/// @brief Cylinderエフェクトを再生
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生中Instanceを指すHandle
		PrimitiveEffectHandle Play(
			const std::string& assetName,
			const PrimitiveEffectPlayDesc& desc = {}
		);

		/// @brief エフェクトを停止
		/// @param handle 停止するInstance Handle
		/// @param mode 停止方法
		void Stop(
			PrimitiveEffectHandle handle,
			PrimitiveEffectStopMode mode = PrimitiveEffectStopMode::Finish
		);

		/// @brief エフェクトのTransformを変更
		/// @param handle 変更するInstance Handle
		/// @param transform 設定するTransform
		/// @return 変更に成功した場合はtrue
		bool SetTransform(PrimitiveEffectHandle handle, const Transform3D& transform);

		/// @brief 指定したPrimitive Effectを一時停止
		/// @param handle 一時停止するEffect Handle
		/// @return 一時停止できた場合はtrue
		bool Pause(PrimitiveEffectHandle handle);

		/// @brief 指定したPrimitive Effectを再開
		/// @param handle 再開するEffect Handle
		/// @return 再開できた場合はtrue
		bool Resume(PrimitiveEffectHandle handle);

		/// @brief 指定したPrimitive Effectの再生速度を設定
		/// @param handle 設定するEffect Handle
		/// @param playbackSpeed 再生速度
		/// @return 設定できた場合はtrue
		bool SetPlaybackSpeed(PrimitiveEffectHandle handle, float playbackSpeed);

		/// @brief 指定したPrimitive Effectが一時停止中か確認
		/// @param handle 確認するEffect Handle
		/// @return 一時停止中の場合はtrue
		bool IsPaused(PrimitiveEffectHandle handle) const;

		/// @brief Handleが現在も有効か確認
		/// @param handle 確認するHandle
		/// @return 有効な場合はtrue
		bool IsAlive(PrimitiveEffectHandle handle) const;

		/// @brief 再生中の全エフェクトを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 描画条件に一致するCylinderを描画
		/// @param sceneType 現在のScene
		/// @param camera 描画に使用するCamera
		/// @param layerMask 描画対象LayerMask
		void DrawLayerMask(
			SceneType sceneType,
			const Camera& camera,
			MadoEngine::Render::RenderLayerMask layerMask
		);

		/// @brief 指定Sceneに属するエフェクトを停止
		/// @param sceneType 停止対象Scene
		void ClearScene(SceneType sceneType);

		/// @brief 全エフェクトを停止
		/// @param mode 停止方法
		void StopAll(PrimitiveEffectStopMode mode = PrimitiveEffectStopMode::Immediate);

		/// @brief 登録済みAsset名を取得
		/// @return Asset名一覧
		std::vector<std::string> GetAssetNames() const;

		/// @brief 登録済みAssetを取得
		/// @param assetName 取得するAsset名
		/// @return Asset、存在しない場合はnullptr
		const CylinderEffectAsset* FindAsset(const std::string& assetName) const;

		/// @brief 編集可能な登録済みAssetを取得
		/// @param assetName 取得するAsset名
		/// @return 編集可能なAsset、存在しない場合はnullptr
		CylinderEffectAsset* FindEditableAsset(const std::string& assetName);

		/// @brief 再生中Cylinder Effect数を取得
		/// @return 再生中Cylinder Effect数
		std::size_t GetActiveEffectCount() const;

	private:
		struct EffectSlot {
			std::unique_ptr<CylinderEffectInstance> instance;
			uint32_t generation = 1;
		};

		/// @brief Systemを構築
		PrimitiveEffectSystem3d() = default;

		/// @brief Systemを破棄
		~PrimitiveEffectSystem3d() = default;

		/// @brief HandleからInstanceを取得
		/// @param handle 取得するHandle
		/// @return Instance、無効な場合はnullptr
		CylinderEffectInstance* Resolve(PrimitiveEffectHandle handle);

		/// @brief HandleからInstanceを取得
		/// @param handle 取得するHandle
		/// @return Instance、無効な場合はnullptr
		const CylinderEffectInstance* Resolve(PrimitiveEffectHandle handle) const;

		/// @brief Slotを解放して再利用可能な状態へ変更
		/// @param index 解放するSlot Index
		void ReleaseSlot(uint32_t index);

		CylinderEffectRenderer3d renderer_;
		std::unordered_map<std::string, std::shared_ptr<CylinderEffectAsset>> assets_;
		std::unordered_map<std::string, std::filesystem::path> assetPaths_;
		std::filesystem::path assetDirectoryPath_ = "Assets/Json/PrimitiveEffect";
		std::vector<EffectSlot> effectSlots_;
		std::queue<uint32_t> freeSlotIndices_;
		SceneType preparedSceneType_ = SceneType::None;
		bool isRenderDataPrepared_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Effect
