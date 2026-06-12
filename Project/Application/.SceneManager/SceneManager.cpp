#include "SceneManager.h"
#include "IScene.h"
#include "ImGuiHeaders.h"
#include "Utility/Logger/Logger.h"
#include "Render/ImGui/EditorUI.h"
#include <cassert>

SceneManager::SceneManager()
	: currentScene_(nullptr)
	, currentSceneType_(SceneType::Title)
	, pendingSceneType_(SceneType::None)
	, hasPendingSceneChange_(false) {}

SceneManager::~SceneManager() {}

void SceneManager::RegisterScene(SceneType type, CreatorFunc creator) {
	creators_[type] = std::move(creator);
	Logger::Output("シーンを登録しました: " + SceneTypeToString(type), Logger::Level::Debug);
}

void SceneManager::Initialize(SceneType initialScene) {
	Logger::Output("SceneManagerを初期化しました", Logger::Level::Application);
	ChangeScene(initialScene);

	selectedModel_ = nullptr;
}

void SceneManager::Update(float dt) {
	ColliderManager::GetInstance().Update();

	if (currentScene_) {
		SceneType next = currentScene_->Update(dt);
		if (next != currentSceneType_) {
			RequestSceneChange(next);
		}
	}

	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });

	MadoEngine::SpriteManager::GetInstance().UpdateAll(currentSceneType_);

	MadoEngine::ModelManager::GetInstance().SetCamera(currentScene_->GetCamera());
	MadoEngine::ModelManager::GetInstance().UpdateAll(currentSceneType_);
}

void SceneManager::Draw() {
	DrawLayerMask(MadoEngine::Render::kAllRenderLayers);

	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::DrawLayer(MadoEngine::Render::RenderLayer layer) {
	DrawLayerMask(MadoEngine::Render::ToRenderLayerMask(layer));
}

void SceneManager::DrawLayerMask(MadoEngine::Render::RenderLayerMask layerMask) {
	if (!currentScene_) {
		return;
	}

	Camera camera = currentScene_->GetCamera();
	if (MadoEngine::Render::ContainsRenderLayer(layerMask, MadoEngine::Render::RenderLayer::Debug)) {
		MadoEngine::DebugLineManager::GetInstance().Draw(camera);
	}
	MadoEngine::SpriteManager::GetInstance().DrawLayerMask(currentSceneType_, layerMask);
	MadoEngine::ModelManager::GetInstance().DrawLayerMask(currentSceneType_, camera, layerMask);
}

void SceneManager::DrawCurrentScene() {
	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::DrawImGui() {
#ifdef USE_IMGUI
	DrawSceneManagerImGui();

	MadoEngine::Editor::DrawModelGizmoOnGameView(currentScene_->GetCamera(), currentSceneType_, selectedModel_);
#endif // USE_IMGUI

	if (currentScene_) {
		currentScene_->DrawImGui();
	}
}

void SceneManager::DrawSceneManagerImGui() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowSize(ImVec2(280.0f, 220.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene Manager")) {
		ImGui::Text("現在のシーン: %s", SceneTypeToString(currentSceneType_).c_str());
		ImGui::Separator();
		ImGui::Text("登録済みシーン");

		if (creators_.empty()) {
			ImGui::TextDisabled("登録されているシーンはありません");
		}

		for (const auto& sceneCreator : creators_) {
			const SceneType type = sceneCreator.first;
			const std::string sceneName = SceneTypeToString(type);
			const bool isCurrentScene = type == currentSceneType_;

			ImGui::PushID(static_cast<int>(type));
			if (ImGui::Selectable(sceneName.c_str(), isCurrentScene) && !isCurrentScene) {
				Logger::Output("ImGuiからシーン遷移を要求しました: " + sceneName, Logger::Level::Debug);
				RequestSceneChange(type);
				ImGui::PopID();
				break;
			}

			if (isCurrentScene) {
				ImGui::SameLine();
				ImGui::TextDisabled("現在");
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
#endif // USE_IMGUI
}

void SceneManager::ApplyPendingSceneChange() {
	if (!hasPendingSceneChange_) {
		return;
	}

	const SceneType nextSceneType = pendingSceneType_;
	hasPendingSceneChange_ = false;
	pendingSceneType_ = SceneType::None;

	ChangeScene(nextSceneType);
}

void SceneManager::RequestSceneChange(SceneType type) {
	if (type == SceneType::None || type == currentSceneType_) {
		return;
	}

	if (creators_.find(type) == creators_.end()) {
		Logger::Output("未登録のシーン遷移が要求されました: " + SceneTypeToString(type), Logger::Level::Error);
		assert(false && "未登録のSceneTypeが指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	pendingSceneType_ = type;
	hasPendingSceneChange_ = true;
	Logger::Output("シーン遷移を予約しました: " + SceneTypeToString(type), Logger::Level::Debug);
}

void SceneManager::ChangeScene(SceneType type) {
	auto it = creators_.find(type);
	if (it == creators_.end()) {
		Logger::Output("指定されたシーンは登録されていません: " + SceneTypeToString(type), Logger::Level::Error);
		assert(false && "未登録のSceneTypeが指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	if (currentScene_) {
		currentScene_->Finalize();
		Logger::Output("旧シーンの終了処理を実行しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
	}

	currentScene_ = it->second();
	currentSceneType_ = type;
	currentScene_->Initialize();
	Logger::Output("シーン遷移を完了しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
}
