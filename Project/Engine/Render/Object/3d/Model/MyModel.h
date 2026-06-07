#pragma once
#include "ModelManager.h"

namespace MyModel {

inline Model* Create(const std::string& name, const std::string& modelName, SceneType sceneType = SceneType::None) {
	return MadoEngine::ModelManager::GetInstance().Create(name, modelName, sceneType);
}

inline Model* Get(const std::string& name) {
	return MadoEngine::ModelManager::GetInstance().Get(name);
}

inline void Destroy(const std::string& name) {
	MadoEngine::ModelManager::GetInstance().Destroy(name);
}

inline void SetCamera(Camera* camera) {
	MadoEngine::ModelManager::GetInstance().SetCamera(camera);
}

} // namespace MyModel
