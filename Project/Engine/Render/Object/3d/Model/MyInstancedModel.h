#pragma once
#include "ModelManager.h"

namespace MyInstancedModel {

/// @brief インスタンス描画モデルを作成する
/// @param name 作成するインスタンス描画モデルの名前
/// @param modelName 使用するモデルアセット名
/// @param sceneType 描画を許可するシーン種別
/// @param layer 設定する描画レイヤー
/// @return 作成したインスタンス描画モデル。失敗時はnullptr
inline InstancedModel* Create(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default)
{
	InstancedModel* model = MadoEngine::ModelManager::GetInstance().CreateInstanced(name, modelName, sceneType);
	if (model) {
		model->SetRenderLayer(layer);
	}
	return model;
}

/// @brief インスタンス描画モデルを取得または作成する
/// @param name 取得または作成するインスタンス描画モデルの名前
/// @param modelName 使用するモデルアセット名
/// @param sceneType 描画を許可するシーン種別
/// @param layer 設定する描画レイヤー
/// @return 取得または作成したインスタンス描画モデル。失敗時はnullptr
inline InstancedModel* GetOrCreate(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default)
{
	InstancedModel* model = MadoEngine::ModelManager::GetInstance().GetOrCreateInstanced(name, modelName, sceneType);
	if (model) {
		model->SetRenderLayer(layer);
	}
	return model;
}

/// @brief インスタンス描画モデルを取得する
/// @param name 取得対象のインスタンス描画モデル名
/// @return 取得したインスタンス描画モデルです。見つからない場合はnullptr
inline InstancedModel* Get(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().GetInstanced(name);
}

/// @brief インスタンス描画モデルを破棄する
/// @param name 破棄対象のインスタンス描画モデル名
inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().DestroyInstanced(name);
}

} // namespace MyInstancedModel
