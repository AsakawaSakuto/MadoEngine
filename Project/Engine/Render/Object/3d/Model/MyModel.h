#pragma once
#include "ModelManager.h"

namespace MyModel {

/// @brief 描画レイヤーを指定してモデルインスタンスを作成する
/// @param name 作成するモデルの識別名
/// @param modelName 使用するモデルアセット名
/// @param sceneType 描画を許可するシーン種別
/// @param layer 設定する描画レイヤー
/// @return 作成したモデルのポインタ。作成に失敗した場合はnullptr
inline Model* Create(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default)
{
	Model* model = MadoEngine::ModelManager::GetInstance().Create(name, modelName, sceneType);
	if (model) {
		model->SetRenderLayer(layer);
	}
	return model;
}

/// @brief モデルインスタンスを取得する
/// @param name 取得対象のモデル名
inline Model* Get(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().Get(name);
}

/// @brief モデルインスタンスを破棄する
/// @param name 破棄対象のモデル名
inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().Destroy(name);
}

/// @brief 指定したシーンに属するModelインスタンスをすべて破棄する
/// @param sceneType 破棄対象のシーン種別
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::ModelManager::GetInstance().DestroyByScene(sceneType);
}

/// @brief カメラをセットする
inline void SetCamera(Camera& camera) {
	MadoEngine::ModelManager::GetInstance().SetCamera(camera);
}

} // namespace MyModel
