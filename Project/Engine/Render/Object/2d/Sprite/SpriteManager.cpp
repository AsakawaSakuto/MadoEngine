#include "SpriteManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine {

SpriteManager* SpriteManager::GetInstance() {
	static SpriteManager instance;
	return &instance;
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

Sprite* SpriteManager::Create(const std::string& name, const std::string& textureName) {
	if (sprites_.contains(name)) {
		Logger::Output("同名のSpriteが既に存在します : " + name, Logger::Level::Warning);
		return sprites_.at(name).get();
	}

	auto sprite = std::make_unique<Sprite>(name);
	sprite->Initialize(device_, commandList_, textureName, sharedGeometry_);
	sprite->SetPSORegistry(psoRegistry_);

	Sprite* ptr = sprite.get();
	sprites_.emplace(name, std::move(sprite));

	Logger::Output("Spriteを生成しました : " + name, Logger::Level::Application);
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

void SpriteManager::UpdateAll() {
	for (auto& [name, sprite] : sprites_) {
		sprite->Update();
	}
}

void SpriteManager::DrawAll() {
	for (auto& [name, sprite] : sprites_) {
		if (sprite->IsVisible()) {
			sprite->Draw();
		}
	}
}

} // namespace MadoEngine
