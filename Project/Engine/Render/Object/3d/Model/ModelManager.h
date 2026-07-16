#pragma once
#include "InstancedModel.h"
#include "Model.h"
#include "Render/Object/RenderLayer.h"
#include "Utility/EditorManagementMode.h"
#include ".SceneManager/SceneType.h"
#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace MadoEngine {

class ModelManager {
public:

	static ModelManager& GetInstance();

	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MadoEngine::Render::PSORegistry* psoRegistry);
	void Finalize();

	/// @brief Modelを作成して管理対象へ登録する
	/// @param name Modelの識別名
	/// @param modelName 使用するModelアセット名またはパス
	/// @param sceneType 描画対象のScene
	/// @param managementMode Modelの管理方法
	/// @return 作成または取得したModel。失敗した場合はnullptr
	Model* Create(
		const std::string& name,
		const std::string& modelName,
		SceneType sceneType = SceneType::None,
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly);

	/// @brief JsonからEditor管理対象のModelを作成または更新する
	/// @param json Model設定Json
	/// @return 作成または更新したModel。失敗した場合はnullptr
	Model* CreateFromJson(const nlohmann::json& json);

	Model* Get(const std::string& name) const;

	/// @brief Modelの識別名を変更する
	/// @param currentName 現在の識別名
	/// @param newName 新しい識別名
	/// @return 変更に成功した場合はtrue
	bool Rename(const std::string& currentName, const std::string& newName);

	void Destroy(const std::string& name);

	/// @brief GPU処理完了後にModelを削除するよう予約する
	/// @param name 削除対象のModel名
	void RequestDestroy(const std::string& name);

	/// @brief 予約されたModel削除を実行する
	void FlushPendingDestroys();

	/// @brief Editor管理対象のModel一覧をJsonへ変換する
	/// @return Editor管理対象のModel一覧を格納したJson
	nlohmann::json ToJson() const;

	/// @brief JsonからEditor管理対象のModel一覧を復元する
	/// @param json 復元元のJson
	void FromJson(const nlohmann::json& json);

	/// @brief Editor管理対象のModel一覧をJsonファイルへ保存する
	/// @param filePath 保存先のファイルパス
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief JsonファイルからEditor管理対象のModel一覧を読み込む
	/// @param filePath 読み込み元のファイルパス
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief 管理中のModel名一覧を取得する
	/// @return 名前順に並べたModel名一覧
	std::vector<std::string> GetNames() const;

	/// @brief Editor管理対象のModel名一覧を取得する
	/// @return 名前順に並べたEditor管理対象のModel名一覧
	std::vector<std::string> GetEditorManagedNames() const;

	/// @brief 読み込み済みModelアセットのパス一覧を取得する
	/// @return パス順に並べたModelアセット一覧
	std::vector<std::string> GetAvailableModelNames() const;

	/// @brief Modelが使用中のアセットパスを取得する
	/// @param name Modelの識別名
	/// @return 使用中のアセットパス。見つからない場合は空文字列
	std::string GetModelAssetName(const std::string& name) const;

	/// @brief インスタンス描画モデルを作成します。
	/// @param name 作成するインスタンス描画モデルの名前です。
	/// @param modelName 使用するモデルアセット名です。
	/// @param sceneType 描画を許可するシーン種別です。
	/// @return 作成したインスタンス描画モデルです。失敗時はnullptrです。
	InstancedModel* CreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType = SceneType::None);

	/// @brief インスタンス描画モデルを取得または作成します。
	/// @param name 取得または作成するインスタンス描画モデルの名前です。
	/// @param modelName 使用するモデルアセット名です。
	/// @param sceneType 描画を許可するシーン種別です。
	/// @return 取得または作成したインスタンス描画モデルです。失敗時はnullptrです。
	InstancedModel* GetOrCreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType = SceneType::None);

	/// @brief インスタンス描画モデルを取得します。
	/// @param name 取得対象のインスタンス描画モデル名です。
	/// @return 取得したインスタンス描画モデルです。見つからない場合はnullptrです。
	InstancedModel* GetInstanced(const std::string& name) const;

	/// @brief インスタンス描画モデルを破棄します。
	/// @param name 破棄対象のインスタンス描画モデル名です。
	void DestroyInstanced(const std::string& name);

	/// @brief 指定したシーンに属するModelインスタンスをすべて破棄する
	/// @param sceneType 破棄対象のシーン種別
	void DestroyByScene(SceneType sceneType);

	const ModelSharedData* GetSharedData(const std::string& modelName) const;

	/// @brief レイにヒットする最前面のModelを取得する
	/// @param currentSceneType 選択対象のシーン種別
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みのレイ方向
	/// @param maxDistance 判定する最大距離
	/// @param outDistance ヒット距離の出力先
	/// @return ヒットしたModel。ヒットしない場合はnullptr
	Model* PickByRay(
		SceneType currentSceneType,
		const Vector3& rayOrigin,
		const Vector3& rayDirection,
		float maxDistance,
		float* outDistance = nullptr) const;

	void SetCamera(const Camera& camera) { activeCamera_ = camera; }
	Camera GetCamera() const { return activeCamera_; }

	void UpdateAll(SceneType currentSceneType);
	void DrawAll(SceneType currentSceneType);
	void DrawAll(SceneType currentSceneType, Camera& camera);
	void DrawShadowMap(SceneType currentSceneType, const Matrix4x4& lightViewProjection);
	void DrawShadowMapLayer(SceneType currentSceneType, const Matrix4x4& lightViewProjection, MadoEngine::Render::RenderLayer layer);

	/// @brief 通常描画で使用するシャドウマップ情報を対象Modelへ設定する
	/// @param currentSceneType 現在のシーン種別
	/// @param shadowMapSrv シャドウマップSRVのGPUディスクリプタハンドル
	/// @param lightViewProjection ライト視点のビュー射影行列
	/// @param width シャドウマップの幅
	/// @param height シャドウマップの高さ
	void SetShadowMap(
		SceneType currentSceneType,
		D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
		const Matrix4x4& lightViewProjection,
		uint32_t width,
		uint32_t height
	);

	/// @brief 指定した描画レイヤーのModelのみを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param layer 描画対象のレイヤー
	void DrawLayer(SceneType currentSceneType, MadoEngine::Render::RenderLayer layer);

	/// @brief 指定した描画レイヤーのModelのみを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param camera 使用するカメラ
	/// @param layer 描画対象のレイヤー
	void DrawLayer(SceneType currentSceneType, Camera& camera, MadoEngine::Render::RenderLayer layer);

	/// @brief 指定したレイヤーマスクに含まれるModelを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(SceneType currentSceneType, MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれるModelを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param camera 使用するカメラ
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(SceneType currentSceneType, Camera& camera, MadoEngine::Render::RenderLayerMask layerMask);
	void DrawShadowMapLayerMask(SceneType currentSceneType, const Matrix4x4& lightViewProjection, MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 通常描画で使用するシャドウマップ情報を指定LayerMaskのModelへ設定する
	/// @param currentSceneType 現在のシーン種別
	/// @param shadowMapSrv シャドウマップSRVのGPUディスクリプタハンドル
	/// @param lightViewProjection ライト視点のビュー射影行列
	/// @param width シャドウマップの幅
	/// @param height シャドウマップの高さ
	/// @param layerMask 設定対象の描画レイヤーマスク
	void SetShadowMapLayerMask(
		SceneType currentSceneType,
		D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
		const Matrix4x4& lightViewProjection,
		uint32_t width,
		uint32_t height,
		MadoEngine::Render::RenderLayerMask layerMask
	);

private:
	ModelManager() = default;
	~ModelManager() = default;

	void LoadAllModels();
	void LoadModelFile(const std::string& path, ModelType type);
	const ModelSharedData* FindSharedData(const std::string& modelName) const;
	std::string ResolveModelPath(const std::string& modelName) const;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
	Camera activeCamera_;

	std::unordered_map<std::string, std::unique_ptr<ModelSharedData>> sharedData_;
	std::unordered_map<std::string, std::string> aliases_;
	std::unordered_map<std::string, std::unique_ptr<Model>> models_;
	std::unordered_map<std::string, std::string> modelAssetNames_;
	std::unordered_set<std::string> editorManagedModelNames_;
	std::unordered_set<std::string> pendingDestroyModelNames_;
	std::unordered_map<std::string, std::unique_ptr<InstancedModel>> instancedModels_;
};

} // namespace MadoEngine
