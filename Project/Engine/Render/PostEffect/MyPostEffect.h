#pragma once

#include "Render/PostEffectManager.h"

namespace MyPostEffect {

/// @brief 内部キーからPostEffectPassのHandleを取得
/// @param key 検索する不変の内部キー
/// @return 見つかったPassのHandle、存在しない場合は無効Handle
[[nodiscard]] inline MadoEngine::Render::PostEffectPassHandle Find(const std::string& key) {
	return MadoEngine::Render::PostEffectManager::GetInstance().Find(key);
}

/// @brief HandleからPassを処理中だけ使用する一時参照として取得
/// @param handle 対象PassのHandle
/// @return 有効な場合はPass、無効な場合はnullptr
inline MadoEngine::Render::PostEffectPass* TryGet(MadoEngine::Render::PostEffectPassHandle handle) {
	return MadoEngine::Render::PostEffectManager::GetInstance().TryGet(handle);
}

/// @brief Handleを指定してPassの有効状態を設定
/// @param handle 対象PassのHandle
/// @param enabled 有効にする場合はtrue
/// @return 設定できた場合はtrue
inline bool SetEnabled(MadoEngine::Render::PostEffectPassHandle handle, bool enabled) {
	return MadoEngine::Render::PostEffectManager::GetInstance().SetEnabled(handle, enabled);
}

/// @brief 型付きParameterを設定
/// @tparam T 登録済みのPostEffect Parameter型
/// @param handle 対象PassのHandle
/// @param parameters 設定するParameter
/// @return Effect型とサイズが一致して設定できた場合はtrue
template<class T>
bool SetParameters(MadoEngine::Render::PostEffectPassHandle handle, const T& parameters) {
	return MadoEngine::Render::PostEffectManager::GetInstance().SetParameters(handle, parameters);
}

/// @brief 型付きParameterを取得
/// @tparam T 登録済みのPostEffect Parameter型
/// @param handle 対象PassのHandle
/// @param outParameters 取得先
/// @return Effect型とサイズが一致して取得できた場合はtrue
template<class T>
bool TryGetParameters(MadoEngine::Render::PostEffectPassHandle handle, T& outParameters) {
	return MadoEngine::Render::PostEffectManager::GetInstance().TryGetParameters(handle, outParameters);
}

/// @brief 型付きParameterを短時間だけ編集して書き復元
/// @tparam T 登録済みのPostEffect Parameter型
/// @tparam Callback T&を受け取るcallback型
/// @param handle 対象PassのHandle
/// @param callback Parameterを変更するcallback
/// @return Effect型とサイズが一致して更新できた場合はtrue
template<class T, class Callback>
bool UpdateParameters(MadoEngine::Render::PostEffectPassHandle handle, Callback&& callback) {
	return MadoEngine::Render::PostEffectManager::GetInstance().UpdateParameters<T>(
		handle,
		std::forward<Callback>(callback)
	);
}

} // namespace MyPostEffect
