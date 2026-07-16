#include "SpriteManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Shader/RootSignatureManager.h"
#include <algorithm>
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
	drawOrder_.clear();
	sharedGeometry_.Finalize();
	Logger::Output("全リソースを解放しました", Logger::Level::Engine);
}

void SpriteManager::SetScreenSize(float width, float height) {
	screenWidth_ = width;
	screenHeight_ = height;
	for (auto& [name, entry] : sprites_) {
		(void)name;
		entry.sprite->SetScreenSize(screenWidth_, screenHeight_);
	}
}

Sprite* SpriteManager::Create(
	const std::string& name,
	const std::string& textureName,
	SceneType sceneType,
	EditorManagementMode managementMode) {
	auto existingIt = sprites_.find(name);
	if (existingIt != sprites_.end()) {
		if (existingIt->second.managementMode != managementMode) {
			Logger::Output("同名のSpriteが異なる管理方法で既に存在します : " + name, Logger::Level::Warning);
			return nullptr;
		}

		Logger::Output("同名のSpriteが既に存在します : " + name, Logger::Level::Warning);
		return existingIt->second.sprite.get();
	}
	if (name.empty()) {
		Logger::Output("名前が空のSpriteは生成できません", Logger::Level::Warning);
		return nullptr;
	}
	if (TextureManager::GetInstance().GetTextureIndex(textureName) == UINT32_MAX) {
		Logger::Output("存在しないテクスチャのSpriteは生成できません : " + textureName, Logger::Level::Warning);
		return nullptr;
	}

	auto sprite = std::make_unique<Sprite>(name);
	sprite->Initialize(device_, commandList_, textureName, sharedGeometry_);
	sprite->SetPSORegistry(psoRegistry_);
	sprite->SetSceneType(sceneType);
	sprite->SetScreenSize(screenWidth_, screenHeight_);

	Sprite* ptr = sprite.get();
	sprites_.emplace(name, SpriteEntry{ std::move(sprite), managementMode });
	drawOrder_.push_back(name);

	Logger::Output("Spriteを生成しました : " + name + " Scene : " + (sceneType == SceneType::None ? "全シーン" : SceneTypeToString(sceneType)), Logger::Level::Application);
	return ptr;
}

Sprite* SpriteManager::CreateFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		Logger::Output("Sprite Jsonの要素がオブジェクトではありません", Logger::Level::Warning);
		return nullptr;
	}

	const std::string name = json.value("name", "Sprite");
	const std::string textureName = json.value("texture", "");
	Sprite* sprite = nullptr;
	auto existingIt = sprites_.find(name);
	if (existingIt != sprites_.end()) {
		if (existingIt->second.managementMode == EditorManagementMode::RuntimeOnly) {
			Logger::Output(
				"実行時専用Spriteと同名のためJsonの読み込みをスキップしました : " + name,
				Logger::Level::Warning);
			return nullptr;
		}
		sprite = existingIt->second.sprite.get();
	} else {
		sprite = Create(name, textureName, SceneType::None, EditorManagementMode::EditorManaged);
	}

	if (!sprite) {
		return nullptr;
	}

	sprite->FromJson(json);
	return sprite;
}

Sprite* SpriteManager::Get(const std::string& name) const {
	auto it = sprites_.find(name);
	if (it == sprites_.end()) {
		Logger::Output("Spriteが見つかりません : " + name, Logger::Level::Warning);
		return nullptr;
	}
	return it->second.sprite.get();
}

void SpriteManager::Destroy(const std::string& name) {
	if (sprites_.erase(name) > 0) {
		drawOrder_.erase(std::remove(drawOrder_.begin(), drawOrder_.end(), name), drawOrder_.end());
		Logger::Output("Spriteを破棄しました : " + name, Logger::Level::Application);
	}
}

void SpriteManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::Noneは全シーン共通のため、Spriteのシーン単位破棄をスキップしました", Logger::Level::Warning);
		return;
	}

	size_t destroyCount = 0;
	for (auto it = drawOrder_.begin(); it != drawOrder_.end();) {
		auto spriteIt = sprites_.find(*it);
		if (spriteIt == sprites_.end()) {
			it = drawOrder_.erase(it);
			continue;
		}

		if (spriteIt->second.sprite->GetSceneType() == sceneType) {
			sprites_.erase(spriteIt);
			it = drawOrder_.erase(it);
			++destroyCount;
		} else {
			++it;
		}
	}

	Logger::Output("シーン内のSpriteインスタンスを破棄しました : " + SceneTypeToString(sceneType) + " 件数 : " + std::to_string(destroyCount), Logger::Level::Application);
}

void SpriteManager::UpdateAll(SceneType currentSceneType) {

	if (sprites_.empty()) { return; }

	for (const std::string& name : drawOrder_) {
		auto it = sprites_.find(name);
		if (it == sprites_.end()) { continue; }
		Sprite* sprite = it->second.sprite.get();

		SceneType spriteScene = sprite->GetSceneType();
		// SceneType::None（全シーン共通）または現在のシーンと一致する場合のみ描画
		if (!sprite->IsVisible()) { continue; }
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) { continue; }

		sprite->Update();
	}
}

void SpriteManager::DrawAll(SceneType currentSceneType) {
	DrawLayerMask(currentSceneType, MadoEngine::Render::kAllRenderLayers);
}

void SpriteManager::DrawLayer(SceneType currentSceneType, MadoEngine::Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, MadoEngine::Render::ToRenderLayerMask(layer));
}

void SpriteManager::DrawLayerMask(SceneType currentSceneType, MadoEngine::Render::RenderLayerMask layerMask) {
	if (sprites_.empty()) { return; }

	// 全スプライト共通のステートをループ外で1回だけ設定する
	bool isStateSet = false;

	for (const std::string& name : drawOrder_) {
		auto it = sprites_.find(name);
		if (it == sprites_.end()) { continue; }
		Sprite* sprite = it->second.sprite.get();

		SceneType spriteScene = sprite->GetSceneType();
		// SceneType::None（全シーン共通）または現在のシーンと一致する場合のみ描画
		if (!sprite->IsVisible()) { continue; }
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) { continue; }
		if (!sprite->IsRenderLayerIncluded(layerMask)) { continue; }

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

nlohmann::json SpriteManager::ToJson() const {
	nlohmann::json sprites = nlohmann::json::array();
	for (const std::string& name : drawOrder_) {
		auto it = sprites_.find(name);
		if (it == sprites_.end()) {
			continue;
		}
		if (it->second.managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		sprites.push_back(it->second.sprite->ToJson());
	}

	return {
		{ "sprites", sprites },
	};
}

void SpriteManager::FromJson(const nlohmann::json& json) {
	const nlohmann::json* spriteArray = nullptr;
	if (json.is_array()) {
		spriteArray = &json;
	} else if (json.contains("sprites") && json.at("sprites").is_array()) {
		spriteArray = &json.at("sprites");
	}

	if (!spriteArray) {
		Logger::Output("Sprite Jsonにsprites配列がありません", Logger::Level::Warning);
		return;
	}

	for (const nlohmann::json& spriteJson : *spriteArray) {
		try {
			CreateFromJson(spriteJson);
		}
		catch (const nlohmann::json::exception& exception) {
			Logger::Output(
				"Sprite Jsonの要素を読み込めませんでした : " + std::string(exception.what()),
				Logger::Level::Error);
		}
	}
}

bool SpriteManager::SaveToFile(const std::filesystem::path& filePath) const {
	return Json::JsonFile::Save(filePath, ToJson(), 4, true);
}

bool SpriteManager::LoadFromFile(const std::filesystem::path& filePath) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}

	FromJson(json);
	return true;
}

std::vector<std::string> SpriteManager::GetNames() const {
	return drawOrder_;
}

std::vector<std::string> SpriteManager::GetEditorManagedNames() const {
	std::vector<std::string> names;
	names.reserve(drawOrder_.size());
	for (const std::string& name : drawOrder_) {
		auto it = sprites_.find(name);
		if (it == sprites_.end()) {
			continue;
		}
		if (it->second.managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		names.push_back(name);
	}
	return names;
}

} // namespace MadoEngine
