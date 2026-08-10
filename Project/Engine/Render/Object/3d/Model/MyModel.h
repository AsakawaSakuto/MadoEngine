#pragma once

#include "ModelManager.h"

namespace MyModel {

/// @brief 描画Layerを指定してModelを生成
/// @param name Model名
/// @param modelName Modelアセット名またはパス
/// @param sceneType 所属Scene
/// @param layer 描画Layer
/// @return 生成したModelのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::ModelHandle Create(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::ModelManager& manager = MadoEngine::ModelManager::GetInstance();
	const MadoEngine::ModelHandle handle = manager.Create(name, modelName, sceneType);
	if (Model* model = manager.TryGet(handle)) {
		model->SetRenderLayer(layer);
	}
	return handle;
}

/// @brief 名前からModelのHandleを取得
/// @param name Model名
/// @return 見つかったModelのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::ModelHandle Find(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().Find(name);
}

/// @brief 名前からModelのHandleを取得する互換API
/// @param name Model名
/// @return 見つかったModelのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::ModelHandle Get(const std::string& name) {
	return Find(name);
}

/// @brief HandleからModelを一時参照として取得
/// @param handle ModelのHandle
/// @return 有効な場合はModel、無効な場合はnullptr
inline Model* TryGet(MadoEngine::ModelHandle handle) {
	return MadoEngine::ModelManager::GetInstance().TryGet(handle);
}

/// @brief Handleを指定してModelを即時削除
/// @param handle 削除対象のHandle
inline void Destroy(MadoEngine::ModelHandle handle) {
	MadoEngine::ModelManager::GetInstance().Destroy(handle);
}

/// @brief Handleを指定してModelの削除を安全な時点まで延期
/// @param handle 削除対象のHandle
inline void RequestDestroy(MadoEngine::ModelHandle handle) {
	if (Model* model = MadoEngine::ModelManager::GetInstance().TryGet(handle)) {
		model->SetVisible(false);
	}
	MadoEngine::ModelManager::GetInstance().RequestDestroy(handle);
}

/// @brief 名前を指定してModelを即時削除する互換API
/// @param name 削除対象の名前
inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().Destroy(name);
}

/// @brief 指定SceneのModelを一括削除する互換API
/// @param sceneType 削除対象のScene
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::ModelManager::GetInstance().DestroyByScene(sceneType);
}

/// @brief 描画に使用するCameraを設定
/// @param camera Camera
inline void SetCamera(Camera& camera) {
	MadoEngine::ModelManager::GetInstance().SetCamera(camera);
}

} // namespace MyModel
