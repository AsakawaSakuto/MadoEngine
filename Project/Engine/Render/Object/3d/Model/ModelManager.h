#pragma once
#include "InstancedModel.h"
#include "Model.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>

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

	Model* Create(const std::string& name, const std::string& modelName, SceneType sceneType = SceneType::None);
	Model* Get(const std::string& name) const;
	void Destroy(const std::string& name);

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

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
	Camera activeCamera_;

	std::unordered_map<std::string, std::unique_ptr<ModelSharedData>> sharedData_;
	std::unordered_map<std::string, std::string> aliases_;
	std::unordered_map<std::string, std::unique_ptr<Model>> models_;
	std::unordered_map<std::string, std::unique_ptr<InstancedModel>> instancedModels_;
};

} // namespace MadoEngine
