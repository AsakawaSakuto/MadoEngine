#include "SpriteManager.h"
#include "Utility/Logger/Logger.h"
#include "Shader/RootSignatureManager.h"
#include <cassert>

namespace MadoEngine {

SpriteManager& SpriteManager::GetInstance() {
	static SpriteManager instance;
	return instance;
}

void SpriteManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MadoEngine::Render::PSORegistry* psoRegistry) {
	assert(device);
	assert(commandList);
	assert(psoRegistry);

	device_      = device;
	commandList_ = commandList;
	psoRegistry_ = psoRegistry;

	sharedGeometry_.Initialize(device_);

	Logger::Output("初期化が完了しました（共有ジオメトリバッファ生成済み）", Logger::Level::Engine);
}

void SpriteManager::Finalize() {
	sprites_.clear();
	sharedGeometry_.Finalize();
	Logger::Output("全リソースを解放しました", Logger::Level::Engine);
}

Sprite* SpriteManager::Create(const std::string& name, const std::string& textureName, SceneType sceneType) {
	if (sprites_.contains(name)) {
		Logger::Output("同名のSpriteが既に存在します : " + name, Logger::Level::Warning);
		return sprites_.at(name).get();
	}

	auto sprite = std::make_unique<Sprite>(name);
	sprite->Initialize(device_, commandList_, textureName, sharedGeometry_);
	sprite->SetPSORegistry(psoRegistry_);
	sprite->SetSceneType(sceneType);

	Sprite* ptr = sprite.get();
	sprites_.emplace(name, std::move(sprite));

	Logger::Output("Spriteを生成しました : " + name + " Scene : " + (sceneType == SceneType::None ? "全シーン" : SceneTypeToString(sceneType)), Logger::Level::Application);
	return ptr;
}

Sprite* SpriteManager::Get(const std::string& name) const {
	auto it = sprites_.find(name);
	if (it == sprites_.end()) {
		Logger::Output("Spriteが見つかりません : " + name, Logger::Level::Warning);
		return nullptr;
	}
	return it->second.get();
}

void SpriteManager::Destroy(const std::string& name) {
	if (sprites_.erase(name) > 0) {
		Logger::Output("Spriteを破棄しました : " + name, Logger::Level::Application);
	}
}

void SpriteManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::Noneは全シーン共通のため、Spriteのシーン単位破棄をスキップしました", Logger::Level::Warning);
		return;
	}

	size_t destroyCount = 0;
	for (auto it = sprites_.begin(); it != sprites_.end();) {
		if (it->second->GetSceneType() == sceneType) {
			it = sprites_.erase(it);
			++destroyCount;
		} else {
			++it;
		}
	}

	Logger::Output("シーン内のSpriteインスタンスを破棄しました : " + SceneTypeToString(sceneType) + " 件数 : " + std::to_string(destroyCount), Logger::Level::Application);
}

void SpriteManager::UpdateAll(SceneType currentSceneType) {

	if (sprites_.empty()) { return; }

	for (auto& [name, sprite] : sprites_) {

		SceneType spriteScene = sprite->GetSceneType();
		// SceneType::None（全シーン共通）または現在のシーンと一致する場合のみ描画
		if (!sprite->IsVisible()) { continue; }
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) { continue; }

		sprite->Update();
	}
}

void SpriteManager::DrawAll(SceneType currentSceneType) {
	if (sprites_.empty()) { return; }

	// 全スプライト共通のステートをループ外で1回だけ設定する
	bool isStateSet = false;

	for (auto& [name, sprite] : sprites_) {
		SceneType spriteScene = sprite->GetSceneType();
		// SceneType::None（全シーン共通）または現在のシーンと一致する場合のみ描画
		if (!sprite->IsVisible()) { continue; }
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) { continue; }

		// 最初の有効なスプライトのタイミングで共通ステートを1回だけ設定
		if (!isStateSet) {
			commandList_->SetGraphicsRootSignature(
				MadoEngine::RootSignatureManager::GetInstance().Get(sprite->GetRootSigKey()));
			commandList_->SetPipelineState(psoRegistry_->Get(sprite->GetPSODesc()));
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->IASetVertexBuffers(0, 1, &sharedGeometry_.vbv);
			commandList_->IASetIndexBuffer(&sharedGeometry_.ibv);
			isStateSet = true;
		}

		// 各スプライト固有の描画（CBV/SRVバインドとドローコール）
		sprite->Draw();
	}
}

} // namespace MadoEngine
