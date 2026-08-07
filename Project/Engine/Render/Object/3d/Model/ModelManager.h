#pragma once

#include "InstancedModel.h"
#include "Model.h"
#include "Render/Object/ObjectHandle.h"
#include "Render/Object/RenderLayer.h"
#include "Utility/EditorManagementMode.h"
#include ".SceneManager/SceneType.h"
#include <cstddef>
#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine {

/// @brief 通常Model生成情報
struct ModelCreateDesc {
	std::string name;
	std::string modelName;
	SceneType sceneType = SceneType::None;
	EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
};

/// @brief InstancedModel生成情報
struct InstancedModelCreateDesc {
	std::string name;
	std::string modelName;
	SceneType sceneType = SceneType::None;
	EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
};

/// @brief Model描画インスタンスの所有と世代付き参照を管理するManager
class ModelManager {
public:
	/// @brief ModelManagerのシングルトンを取得する
	/// @return ModelManagerのインスタンス
	static ModelManager& GetInstance();

	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

	/// @brief ModelManagerを初期化する
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param psoRegistry PSOレジストリ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry);

	/// @brief 全描画インスタンスとアセット共有データを解放する
	void Finalize();

	/// @brief 通常Modelを生成する
	/// @param name Model名
	/// @param modelName Modelアセット名またはパス
	/// @param sceneType 所属Scene
	/// @param managementMode 管理方式
	/// @return 生成したModelのHandle。失敗した場合は無効Handle
	[[nodiscard]] ModelHandle Create(
		const std::string& name,
		const std::string& modelName,
		SceneType sceneType = SceneType::None,
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly);

	/// @brief 指定した生成情報から通常Modelを生成する
	/// @param desc Model生成情報
	/// @return 生成したModelのHandle。失敗した場合は無効Handle
	[[nodiscard]] ModelHandle Create(const ModelCreateDesc& desc);

	/// @brief 同じ条件の通常Modelを取得し、存在しない場合だけ生成する
	/// @param desc Model生成情報
	/// @return 取得または生成したModelのHandle。条件不一致または生成失敗時は無効Handle
	[[nodiscard]] ModelHandle FindOrCreate(const ModelCreateDesc& desc);

	/// @brief JSONからEditor管理Modelを生成または更新する
	/// @param json Model設定
	/// @return 生成または更新したModelのHandle。失敗した場合は無効Handle
	[[nodiscard]] ModelHandle CreateFromJson(const nlohmann::json& json);

	/// @brief Handleから通常Modelを一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はModel、無効な場合はnullptr
	Model* TryGet(ModelHandle handle);

	/// @brief Handleから通常Modelを読み取り専用の一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はModel、無効な場合はnullptr
	const Model* TryGet(ModelHandle handle) const;

	/// @brief 名前から通常ModelのHandleを検索する
	/// @param name 検索する名前
	/// @return 見つかったModelのHandle。見つからない場合は無効Handle
	[[nodiscard]] ModelHandle Find(const std::string& name) const;

	/// @brief Handleが現在の通常Modelを参照しているか確認する
	/// @param handle 確認するHandle
	/// @return 有効なModelを参照している場合はtrue
	[[nodiscard]] bool IsValid(ModelHandle handle) const;

	/// @brief 名前から通常Modelを一時参照として取得する互換API
	/// @param name Model名
	/// @return 見つかったModel。見つからない場合はnullptr
	Model* Get(const std::string& name);

	/// @brief 名前から通常Modelを読み取り専用の一時参照として取得する互換API
	/// @param name Model名
	/// @return 見つかったModel。見つからない場合はnullptr
	const Model* Get(const std::string& name) const;

	/// @brief 名前を変更してもHandleを維持する
	/// @param handle 名前を変更するModelのHandle
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(ModelHandle handle, const std::string& newName);

	/// @brief 名前を指定して通常Model名を変更する互換API
	/// @param currentName 現在の名前
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(const std::string& currentName, const std::string& newName);

	/// @brief GPUが対象を使用していないことが保証された時点で通常Modelを即時削除する
	/// @param handle 削除対象のHandle
	/// @return 削除できた場合はtrue
	bool Destroy(ModelHandle handle);

	/// @brief 名前を指定して通常Modelを即時削除する互換API
	/// @param name 削除対象の名前
	/// @return 削除できた場合はtrue
	bool Destroy(const std::string& name);

	/// @brief 描画中でも安全な時点まで通常Modelの削除を延期する
	/// @param handle 削除対象のHandle
	void RequestDestroy(ModelHandle handle);

	/// @brief 名前を指定して通常Modelの削除を延期する互換API
	/// @param name 削除対象の名前
	void RequestDestroy(const std::string& name);

	/// @brief InstancedModelを生成する
	/// @param name InstancedModel名
	/// @param modelName Modelアセット名またはパス
	/// @param sceneType 所属Scene
	/// @return 生成したInstancedModelのHandle。失敗した場合は無効Handle
	[[nodiscard]] InstancedModelHandle CreateInstanced(
		const std::string& name,
		const std::string& modelName,
		SceneType sceneType = SceneType::None);

	/// @brief 指定した生成情報からInstancedModelを生成する
	/// @param desc InstancedModel生成情報
	/// @return 生成したInstancedModelのHandle。失敗した場合は無効Handle
	[[nodiscard]] InstancedModelHandle CreateInstanced(const InstancedModelCreateDesc& desc);

	/// @brief 同じ条件のInstancedModelを取得し、存在しない場合だけ生成する
	/// @param desc InstancedModel生成情報
	/// @return 取得または生成したInstancedModelのHandle。条件不一致または生成失敗時は無効Handle
	[[nodiscard]] InstancedModelHandle FindOrCreateInstanced(const InstancedModelCreateDesc& desc);

	/// @brief InstancedModelを取得し、存在しない場合だけ生成する互換API
	/// @param name InstancedModel名
	/// @param modelName Modelアセット名またはパス
	/// @param sceneType 所属Scene
	/// @return 取得または生成したInstancedModelのHandle。条件不一致または生成失敗時は無効Handle
	[[nodiscard]] InstancedModelHandle GetOrCreateInstanced(
		const std::string& name,
		const std::string& modelName,
		SceneType sceneType = SceneType::None);

	/// @brief HandleからInstancedModelを一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はInstancedModel、無効な場合はnullptr
	InstancedModel* TryGet(InstancedModelHandle handle);

	/// @brief HandleからInstancedModelを読み取り専用の一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はInstancedModel、無効な場合はnullptr
	const InstancedModel* TryGet(InstancedModelHandle handle) const;

	/// @brief 名前からInstancedModelのHandleを検索する
	/// @param name 検索する名前
	/// @return 見つかったInstancedModelのHandle。見つからない場合は無効Handle
	[[nodiscard]] InstancedModelHandle FindInstanced(const std::string& name) const;

	/// @brief Handleが現在のInstancedModelを参照しているか確認する
	/// @param handle 確認するHandle
	/// @return 有効なInstancedModelを参照している場合はtrue
	[[nodiscard]] bool IsValid(InstancedModelHandle handle) const;

	/// @brief 名前からInstancedModelを一時参照として取得する互換API
	/// @param name InstancedModel名
	/// @return 見つかったInstancedModel。見つからない場合はnullptr
	InstancedModel* GetInstanced(const std::string& name);

	/// @brief 名前からInstancedModelを読み取り専用の一時参照として取得する互換API
	/// @param name InstancedModel名
	/// @return 見つかったInstancedModel。見つからない場合はnullptr
	const InstancedModel* GetInstanced(const std::string& name) const;

	/// @brief InstancedModelの名前を変更してもHandleを維持する
	/// @param handle 名前を変更するHandle
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool RenameInstanced(InstancedModelHandle handle, const std::string& newName);

	/// @brief GPUが対象を使用していないことが保証された時点でInstancedModelを即時削除する
	/// @param handle 削除対象のHandle
	/// @return 削除できた場合はtrue
	bool Destroy(InstancedModelHandle handle);

	/// @brief 名前を指定してInstancedModelを即時削除する互換API
	/// @param name 削除対象の名前
	/// @return 削除できた場合はtrue
	bool DestroyInstanced(const std::string& name);

	/// @brief 描画中でも安全な時点までInstancedModelの削除を延期する
	/// @param handle 削除対象のHandle
	void RequestDestroy(InstancedModelHandle handle);

	/// @brief 延期されている通常ModelとInstancedModelの削除を安全な時点で実行する
	void FlushPendingDestroys();

	/// @brief 指定Sceneに属する通常ModelとInstancedModelを即時削除する
	/// @param sceneType 削除対象のScene
	void DestroyByScene(SceneType sceneType);

	/// @brief Editor管理ModelをJSONへ変換する
	/// @return Model一覧を含むJSON
	nlohmann::json ToJson() const;

	/// @brief JSONからEditor管理Modelを復元する
	/// @param json Model一覧を含むJSON
	void FromJson(const nlohmann::json& json);

	/// @brief Editor管理ModelをJSONファイルへ保存する
	/// @param filePath 保存先
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief JSONファイルからEditor管理Modelを読み込む
	/// @param filePath 読み込み元
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief 通常Model名一覧を取得する
	/// @return 名前順のModel名一覧
	std::vector<std::string> GetNames() const;

	/// @brief 管理中の通常Modelインスタンス数を取得する
	/// @return 管理中の通常Modelインスタンス数
	std::size_t GetModelCount() const;

	/// @brief Editor管理Model名一覧を取得する
	/// @return 名前順のEditor管理Model名一覧
	std::vector<std::string> GetEditorManagedNames() const;

	/// @brief 読み込み済みModelアセット名一覧を取得する
	/// @return 名前順のModelアセット名一覧
	std::vector<std::string> GetAvailableModelNames() const;

	/// @brief 通常Modelが使用しているアセット名を取得する
	/// @param handle ModelのHandle
	/// @return アセット名。無効Handleの場合は空文字列
	std::string GetModelAssetName(ModelHandle handle) const;

	/// @brief 名前から通常Modelが使用しているアセット名を取得する互換API
	/// @param name Model名
	/// @return アセット名。見つからない場合は空文字列
	std::string GetModelAssetName(const std::string& name) const;

	/// @brief Modelアセット共有データを取得する
	/// @param modelName Modelアセット名またはパス
	/// @return 共有データ。見つからない場合はnullptr
	const ModelSharedData* GetSharedData(const std::string& modelName) const;

	/// @brief レイに最も近くヒットした通常ModelのHandleを取得する
	/// @param currentSceneType 選択対象のScene
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みレイ方向
	/// @param maxDistance 最大距離
	/// @param outDistance ヒット距離の出力先
	/// @return ヒットしたModelのHandle。ヒットしない場合は無効Handle
	[[nodiscard]] ModelHandle PickByRay(
		SceneType currentSceneType,
		const Vector3& rayOrigin,
		const Vector3& rayDirection,
		float maxDistance,
		float* outDistance = nullptr) const;

	/// @brief 描画に使用するCameraを設定する
	/// @param camera Camera
	void SetCamera(const Camera& camera) {
		activeCamera_ = camera;
	}

	/// @brief 現在のCameraを取得する
	/// @return Camera
	Camera GetCamera() const {
		return activeCamera_;
	}

	/// @brief 現在SceneのModelを更新する
	/// @param currentSceneType 現在のScene
	void UpdateAll(SceneType currentSceneType);

	/// @brief 現在SceneのModelを描画する
	/// @param currentSceneType 現在のScene
	void DrawAll(SceneType currentSceneType);

	/// @brief 指定Cameraで現在SceneのModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param camera Camera
	void DrawAll(SceneType currentSceneType, Camera& camera);

	/// @brief ShadowMapへModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param lightViewProjection LightのViewProjection
	void DrawShadowMap(SceneType currentSceneType, const Matrix4x4& lightViewProjection);

	/// @brief 指定LayerのModelをShadowMapへ描画する
	/// @param currentSceneType 現在のScene
	/// @param lightViewProjection LightのViewProjection
	/// @param layer 描画Layer
	void DrawShadowMapLayer(SceneType currentSceneType, const Matrix4x4& lightViewProjection, Render::RenderLayer layer);

	/// @brief 通常描画用ShadowMap情報を設定する
	/// @param currentSceneType 現在のScene
	/// @param shadowMapSrv ShadowMapのSRV
	/// @param lightViewProjection LightのViewProjection
	/// @param width ShadowMap幅
	/// @param height ShadowMap高さ
	void SetShadowMap(SceneType currentSceneType, D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv, const Matrix4x4& lightViewProjection, uint32_t width, uint32_t height);

	/// @brief 指定LayerのModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param layer 描画Layer
	void DrawLayer(SceneType currentSceneType, Render::RenderLayer layer);

	/// @brief 指定CameraとLayerでModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param camera Camera
	/// @param layer 描画Layer
	void DrawLayer(SceneType currentSceneType, Camera& camera, Render::RenderLayer layer);

	/// @brief 指定LayerMaskのModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param layerMask 描画LayerMask
	void DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask);

	/// @brief 指定CameraとLayerMaskでModelを描画する
	/// @param currentSceneType 現在のScene
	/// @param camera Camera
	/// @param layerMask 描画LayerMask
	void DrawLayerMask(SceneType currentSceneType, Camera& camera, Render::RenderLayerMask layerMask);

	/// @brief 指定LayerMaskのModelをShadowMapへ描画する
	/// @param currentSceneType 現在のScene
	/// @param lightViewProjection LightのViewProjection
	/// @param layerMask 描画LayerMask
	void DrawShadowMapLayerMask(SceneType currentSceneType, const Matrix4x4& lightViewProjection, Render::RenderLayerMask layerMask);

	/// @brief 指定LayerMaskのModelへShadowMap情報を設定する
	/// @param currentSceneType 現在のScene
	/// @param shadowMapSrv ShadowMapのSRV
	/// @param lightViewProjection LightのViewProjection
	/// @param width ShadowMap幅
	/// @param height ShadowMap高さ
	/// @param layerMask 描画LayerMask
	void SetShadowMapLayerMask(
		SceneType currentSceneType,
		D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
		const Matrix4x4& lightViewProjection,
		uint32_t width,
		uint32_t height,
		Render::RenderLayerMask layerMask);

private:
	struct ModelSlot {
		std::unique_ptr<Model> model;
		std::string name;
		std::string assetName;
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
		uint32_t generation = 1;
		bool active = false;
	};

	struct InstancedModelSlot {
		std::unique_ptr<InstancedModel> model;
		std::string name;
		std::string assetName;
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
		uint32_t generation = 1;
		bool active = false;
	};

	ModelManager() = default;
	~ModelManager() = default;

	/// @brief Assets配下のModelアセットを読み込む
	void LoadAllModels();

	/// @brief Modelファイルを共有データとして読み込む
	/// @param path Modelファイルパス
	/// @param type Model種別
	void LoadModelFile(const std::string& path, ModelType type);

	/// @brief Modelアセット共有データを検索する
	/// @param modelName Modelアセット名またはパス
	/// @return 共有データ。見つからない場合はnullptr
	const ModelSharedData* FindSharedData(const std::string& modelName) const;

	/// @brief Modelアセット名を正規化したパスへ解決する
	/// @param modelName Modelアセット名またはパス
	/// @return 解決したパス
	std::string ResolveModelPath(const std::string& modelName) const;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	Render::PSORegistry* psoRegistry_ = nullptr;
	Camera activeCamera_;
	std::unordered_map<std::string, std::unique_ptr<ModelSharedData>> sharedData_;
	std::unordered_map<std::string, std::string> aliases_;
	std::vector<ModelSlot> modelSlots_;
	std::vector<uint32_t> freeModelSlots_;
	std::unordered_map<std::string, ModelHandle> modelNameToHandle_;
	std::vector<ModelHandle> pendingDestroyModelHandles_;
	std::vector<InstancedModelSlot> instancedModelSlots_;
	std::vector<uint32_t> freeInstancedModelSlots_;
	std::unordered_map<std::string, InstancedModelHandle> instancedModelNameToHandle_;
	std::vector<InstancedModelHandle> pendingDestroyInstancedModelHandles_;
};

} // namespace MadoEngine
