#pragma once

#include "ModelManager.h"

namespace MyInstancedModel {

/// @brief 描画Layerを指定してInstancedModelを生成
/// @param name InstancedModel名
/// @param modelName Modelアセット名またはパス
/// @param sceneType 所属Scene
/// @param layer 描画Layer
/// @return 生成したInstancedModelのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::InstancedModelHandle Create(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::ModelManager& manager = MadoEngine::ModelManager::GetInstance();
	const MadoEngine::InstancedModelHandle handle = manager.CreateInstanced(name, modelName, sceneType);
	if (InstancedModel* model = manager.TryGet(handle)) {
		model->SetRenderLayer(layer);
	}
	return handle;
}

/// @brief 同じ条件のInstancedModelを取得し、存在しない場合だけ生成
/// @param name InstancedModel名
/// @param modelName Modelアセット名またはパス
/// @param sceneType 所属Scene
/// @param layer 描画Layer
/// @return 取得または生成したInstancedModelのHandle、条件不一致または生成失敗時は無効Handle
[[nodiscard]] inline MadoEngine::InstancedModelHandle GetOrCreate(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::ModelManager& manager = MadoEngine::ModelManager::GetInstance();
	const MadoEngine::InstancedModelHandle handle = manager.GetOrCreateInstanced(name, modelName, sceneType);
	if (InstancedModel* model = manager.TryGet(handle)) {
		model->SetRenderLayer(layer);
	}
	return handle;
}

/// @brief 名前からInstancedModelのHandleを取得
/// @param name InstancedModel名
/// @return 見つかったInstancedModelのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::InstancedModelHandle Find(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().FindInstanced(name);
}

/// @brief 名前からInstancedModelのHandleを取得する互換API
/// @param name InstancedModel名
/// @return 見つかったInstancedModelのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::InstancedModelHandle Get(const std::string& name) {
	return Find(name);
}

/// @brief HandleからInstancedModelを一時参照として取得
/// @param handle InstancedModelのHandle
/// @return 有効な場合はInstancedModel、無効な場合はnullptr
inline InstancedModel* TryGet(MadoEngine::InstancedModelHandle handle) {
	return MadoEngine::ModelManager::GetInstance().TryGet(handle);
}

/// @brief Handleを指定してInstancedModelを即時削除
/// @param handle 削除対象のHandle
inline void Destroy(MadoEngine::InstancedModelHandle handle) {
	MadoEngine::ModelManager::GetInstance().Destroy(handle);
}

/// @brief 名前を指定してInstancedModelを即時削除する互換API
/// @param name 削除対象の名前
inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().DestroyInstanced(name);
}

} // namespace MyInstancedModel
