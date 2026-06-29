#pragma once
#include "ModelManager.h"

namespace MyInstancedModel {

/// @brief インスタンス描画モデルを作成します。
/// @param name 作成するインスタンス描画モデルの名前です。
/// @param modelName 使用するモデルアセット名です。
/// @param sceneType 描画を許可するシーン種別です。
/// @param layer 設定する描画レイヤーです。
/// @return 作成したインスタンス描画モデルです。失敗時はnullptrです。
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

/// @brief インスタンス描画モデルを取得または作成します。
/// @param name 取得または作成するインスタンス描画モデルの名前です。
/// @param modelName 使用するモデルアセット名です。
/// @param sceneType 描画を許可するシーン種別です。
/// @param layer 設定する描画レイヤーです。
/// @return 取得または作成したインスタンス描画モデルです。失敗時はnullptrです。
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

/// @brief インスタンス描画モデルを取得します。
/// @param name 取得対象のインスタンス描画モデル名です。
/// @return 取得したインスタンス描画モデルです。見つからない場合はnullptrです。
inline InstancedModel* Get(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().GetInstanced(name);
}

/// @brief インスタンス描画モデルを破棄します。
/// @param name 破棄対象のインスタンス描画モデル名です。
inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().DestroyInstanced(name);
}

} // namespace MyInstancedModel
