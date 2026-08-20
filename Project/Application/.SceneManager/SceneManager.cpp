#include "SceneManager.h"
#include "IScene.h"
#include "ImGuiHeaders.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Light/LightManager.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/Text/TextManager.h"
#include "Render/Object/3d/Model/ModelManager.h"
#include "Render/Object/3d/Line/MyDebugLine.h"
#include "Render/Object/3d/Particle/ParticleSystem3d.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectSystem3d.h"
#include "Render/Object/3d/BeamEffect/BeamEffectSystem3d.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectSystem3d.h"
#include "Render/Object/3d/EffectSequence/EffectSequenceSystem.h"
#include "EditorUIHeaders.h"
#include "Render/ImGui/Editor/History/EditorHistory.h"
#include "../InputRegister.h"
#include "../ColliderPairRegister.h"
#include <cassert>

namespace {

	/// @brief 指定シーン所属のEditor管理オブジェクトをJSONから読み込み
	/// @param sceneType 読み込み対象のシーン
	void LoadEditorSceneObjects(SceneType sceneType) {
		MadoEngine::Editor::LoadModelEditorJson(sceneType);
		MadoEngine::Editor::LoadSpriteEditorJson(sceneType);
		MadoEngine::Editor::LoadTextEditorJson(sceneType);
	}

	/// @brief PointLightとSpotLightの位置をDebugLineで表示
	void AddLightPositionDebugSpheres() {
		LightManager& lightManager = LightManager::GetInstance();
		constexpr float kLightDebugSphereRadius = 0.5f;
		const Vector4 enabledPointColor = { 1.0f, 0.85f, 0.1f, 1.0f };
		const Vector4 disabledPointColor = { 0.35f, 0.3f, 0.08f, 1.0f };
		const Vector4 enabledSpotColor = { 0.1f, 0.8f, 1.0f, 1.0f };
		const Vector4 disabledSpotColor = { 0.05f, 0.25f, 0.35f, 1.0f };

		// Light種別と有効状態を色分けしてEditor上の配置確認を補助
		for (LightHandle handle : lightManager.GetPointLightHandles()) {
			const PointLight* light = lightManager.GetPointLightData(handle);
			if (!light) {
				continue;
			}

			Sphere sphere;
			sphere.center = light->position;
			sphere.radius = kLightDebugSphereRadius;
			MyDebugLine::AddShape(sphere, lightManager.IsEnabled(handle) ? enabledPointColor : disabledPointColor);
		}

		for (LightHandle handle : lightManager.GetSpotLightHandles()) {
			const SpotLight* light = lightManager.GetSpotLightData(handle);
			if (!light) {
				continue;
			}

			Sphere sphere;
			sphere.center = light->position;
			sphere.radius = kLightDebugSphereRadius;
			MyDebugLine::AddShape(sphere, lightManager.IsEnabled(handle) ? enabledSpotColor : disabledSpotColor);
		}
	}

} // namespace

SceneManager::SceneManager()
	: currentScene_(nullptr)
	, currentSceneType_(SceneType::Title)
	, pendingSceneType_(SceneType::None)
	, hasPendingSceneChange_(false) {}

SceneManager::~SceneManager() {
	sceneBgmController_.Finalize();

	if (!currentScene_) {
		return;
	}

	// Scene本体を終了してから所属Resourceを管理Systemごとに一括破棄
	const SceneType previousSceneType = currentSceneType_;
	MadoEngine::Editor::EditorHistory::GetInstance().Clear();
#ifdef USE_IMGUI
	MadoEngine::Editor::ResetModelGizmoOnSceneChange(selectedModel_);
#else
	selectedModel_ = {};
#endif // USE_IMGUI
	currentScene_->Finalize();
	currentScene_.reset();
	MadoEngine::SpriteManager::GetInstance().DestroyByScene(previousSceneType);
	MadoEngine::TextManager::GetInstance().DestroyByScene(previousSceneType);
	MadoEngine::ModelManager::GetInstance().DestroyByScene(previousSceneType);
	LightManager::GetInstance().DestroyByScene(previousSceneType);
	MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().ClearScene(previousSceneType);
	MadoEngine::Particle::ParticleSystem3d::GetInstance().ClearScene(previousSceneType);
	MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().ClearScene(previousSceneType);
	MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().ClearScene(previousSceneType);
	MadoEngine::Beam::BeamEffectSystem3d::GetInstance().ClearScene(previousSceneType);
}

void SceneManager::RegisterScene(SceneType type, CreatorFunc creator) {
	creators_[type] = std::move(creator);
	Logger::Output("シーンを登録しました: " + SceneTypeToString(type), Logger::Level::Debug);
}

void SceneManager::Initialize(SceneType initialScene) {
	Logger::Output("SceneManagerを初期化しました", Logger::Level::Application);

	// Scene生成前に共通入力Action、衝突Pair、永続Applicationデータを初期化
	RegisterInput();
	RegisterColliderPair();
	commonData_.Initialize();

	ChangeScene(initialScene);

	selectedModel_ = {};
}

void SceneManager::Update(float dt) {
	SceneTransitionController& transitionController = commonData_.GetSceneTransitionController();
	transitionController.Update(dt);
	sceneBgmController_.Update(transitionController.GetEffectProgress());

	// Scene処理が参照するCollider状態をFrame先頭で更新
	ColliderManager::GetInstance().Update();

	if (currentScene_) {
		SceneType next = currentScene_->Update(dt);
		if (next != currentSceneType_) {

			// Scene側は遷移先だけを通知し、Effect進行と実際の切替時期はManager側で管理
			RequestSceneChange(next);
		}
	}

	if (transitionController.IsSceneChangeReady() && !hasPendingSceneChange_) {

		// Effect最大Frameを描画してから切り替えられるようフレーム末尾へ予約
		QueueSceneChange(transitionController.GetDestinationSceneType());
	}

	MyDebugLine::AddGrid(1000.0f, 1000, { 0.5f, 0.5f, 0.5f, 1.0f });
	AddLightPositionDebugSpheres();

	// Scene固有更新後のTransformを各描画Systemへ同期
	MadoEngine::SpriteManager::GetInstance().UpdateAll(currentSceneType_);
	MadoEngine::TextManager::GetInstance().UpdateAll(currentSceneType_);

	MadoEngine::ModelManager::GetInstance().SetCamera(currentScene_->GetCamera());
	MadoEngine::ModelManager::GetInstance().UpdateAll(currentSceneType_, dt);
	MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().Update(dt);
	MadoEngine::Particle::ParticleSystem3d::GetInstance().Update(dt);
	MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Update(dt);
	MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Update(dt);
	MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Update(dt);
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

	// 不透明、透明、Overlayの順序を維持したLayerMask描画
	DrawSceneLayerMask(layerMask);
	DrawTransparentLayerMask(layerMask);
	DrawOverlayLayerMask(layerMask);
}

void SceneManager::DrawSceneLayerMask(MadoEngine::Render::RenderLayerMask layerMask) {
	if (!currentScene_) {
		return;
	}

	Camera& camera = currentScene_->GetCamera();
	if (MadoEngine::Render::ContainsRenderLayer(layerMask, MadoEngine::Render::RenderLayer::Debug)) {
		MadoEngine::DebugLineManager::GetInstance().Draw(camera);
	}
	MadoEngine::ModelManager::GetInstance().DrawOpaqueLayerMask(currentSceneType_, camera, layerMask);
}

void SceneManager::DrawTransparentLayerMask(MadoEngine::Render::RenderLayerMask layerMask) {
	if (!currentScene_) {
		return;
	}

	Camera& camera = currentScene_->GetCamera();

	// 背景が完成したColor Targetへ透明Modelを奥から手前の順でAlpha Blend
	MadoEngine::ModelManager::GetInstance().DrawTransparentLayerMask(currentSceneType_, camera, layerMask);

	// 透明系Effectを共通CameraとLayerMaskで同じ透明描画段階へ集約
	MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().DrawLayerMask(
		currentSceneType_,
		camera,
		layerMask
	);
	MadoEngine::Particle::ParticleSystem3d::GetInstance().DrawLayerMask(
		currentSceneType_,
		camera,
		layerMask
	);
	MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().DrawLayerMask(
		currentSceneType_,
		camera,
		layerMask
	);
	MadoEngine::Beam::BeamEffectSystem3d::GetInstance().DrawLayerMask(
		currentSceneType_,
		camera,
		layerMask
	);
}

void SceneManager::DrawOverlayLayerMask(MadoEngine::Render::RenderLayerMask layerMask) {
	if (!currentScene_) {
		return;
	}

	MadoEngine::SpriteManager::GetInstance().DrawLayerMask(currentSceneType_, layerMask);
	MadoEngine::TextManager::GetInstance().DrawLayerMask(currentSceneType_, layerMask);
}

void SceneManager::DrawOverlayInOrder(MadoEngine::Render::IRenderLayerBatchContext& batchContext) {
	if (!currentScene_) {
		return;
	}

	MadoEngine::SpriteManager::GetInstance().DrawInOrder(currentSceneType_, batchContext);
	MadoEngine::TextManager::GetInstance().DrawInOrder(currentSceneType_, batchContext);
}

void SceneManager::DrawCurrentScene() {
	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::DrawImGui() {
#ifdef USE_IMGUI
	DrawSceneManagerImGui();

	if (!currentScene_) {
		return;
	}

	MadoEngine::Editor::DrawCameraManagerEditorUI(
		currentScene_->GetCameraManager(),
		currentSceneType_
	);
	MadoEngine::Editor::DrawModelGizmoOnGameView(currentScene_->GetCamera(), currentSceneType_, selectedModel_);
	currentScene_->DrawImGui();
#endif // USE_IMGUI
}

const Camera& SceneManager::GetCurrentCamera() const {
	if (!currentScene_) {
		static const Camera fallbackCamera;
		return fallbackCamera;
	}

	return currentScene_->GetCamera();
}

Vector3 SceneManager::GetShadowFocusPosition() const {
	if (!currentScene_) {
		return {};
	}

	return currentScene_->GetShadowFocusPosition();
}

bool SceneManager::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	if (!currentScene_) {
		outPosition = {};
		return false;
	}

	return currentScene_->TryGetShadowDebugTargetPosition(outPosition);
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

		// 現在Sceneを除いた登録済みFactoryだけを遷移先として受付
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

	// ChangeScene中の再要求と混在しないよう予約状態を先に解除
	hasPendingSceneChange_ = false;
	pendingSceneType_ = SceneType::None;

	ChangeScene(nextSceneType);
	commonData_.GetSceneTransitionController().NotifySceneChanged(currentSceneType_);
}

float SceneManager::GetSceneTransitionEffectProgress() const {
	return commonData_.GetSceneTransitionController().GetEffectProgress();
}

bool SceneManager::IsSceneTransitioning() const {
	return commonData_.GetSceneTransitionController().IsTransitioning();
}

void SceneManager::RequestSceneChange(SceneType type) {
	if (type == SceneType::None || type == currentSceneType_) {
		return;
	}

	if (creators_.find(type) == creators_.end()) {

		// 未登録Typeによる現在Sceneの破棄を事前に阻止
		Logger::Output("未登録のシーン遷移が要求されました: " + SceneTypeToString(type), Logger::Level::Error);
		assert(false && "未登録のSceneTypeが指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	SceneTransitionController& transitionController = commonData_.GetSceneTransitionController();
	if (transitionController.Request(type)) {
		Logger::Output("シーン遷移演出を開始しました: " + SceneTypeToString(type), Logger::Level::Debug);
	}
}

void SceneManager::QueueSceneChange(SceneType type) {
	if (type == SceneType::None || type == currentSceneType_ || hasPendingSceneChange_) {
		return;
	}

	pendingSceneType_ = type;
	hasPendingSceneChange_ = true;
	Logger::Output("遷移Effect最大到達後のシーン切替を予約しました: " + SceneTypeToString(type), Logger::Level::Debug);
}

void SceneManager::ChangeScene(SceneType type) {
	auto it = creators_.find(type);
	if (it == creators_.end()) {
		Logger::Output("指定されたシーンは登録されていません: " + SceneTypeToString(type), Logger::Level::Error);
		assert(false && "未登録のSceneTypeが指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	if (currentScene_) {

		// 旧SceneのHandleが新Sceneへ残らないよう全所属Resourceを生成前に破棄
		const SceneType previousSceneType = currentSceneType_;
		MadoEngine::Editor::EditorHistory::GetInstance().Clear();
#ifdef USE_IMGUI
		MadoEngine::Editor::ResetModelGizmoOnSceneChange(selectedModel_);
#else
		selectedModel_ = {};
#endif // USE_IMGUI
		currentScene_->Finalize();
		currentScene_.reset();
		MadoEngine::SpriteManager::GetInstance().DestroyByScene(previousSceneType);
		MadoEngine::TextManager::GetInstance().DestroyByScene(previousSceneType);
		MadoEngine::ModelManager::GetInstance().DestroyByScene(previousSceneType);
		LightManager::GetInstance().DestroyByScene(previousSceneType);
		MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().ClearScene(previousSceneType);
		MadoEngine::Particle::ParticleSystem3d::GetInstance().ClearScene(previousSceneType);
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().ClearScene(previousSceneType);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().ClearScene(previousSceneType);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().ClearScene(previousSceneType);
		Logger::Output("旧シーンの終了処理を実行しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
	}

	// Editor管理Objectを復元してからScene固有初期化を実行
	currentScene_ = it->second(commonData_);
	currentSceneType_ = type;
	LoadEditorSceneObjects(currentSceneType_);
	currentScene_->Initialize();
	sceneBgmController_.ChangeScene(currentSceneType_);

	// Runtime Camera登録後にScene別Jsonを読み込み、Editor Cameraと保存済みActive状態を復元
	const std::filesystem::path cameraJsonPath =
		CameraManager::CreateDefaultJsonPath(SceneTypeToString(currentSceneType_));
	if (MadoEngine::Json::JsonFile::Exists(cameraJsonPath)) {
		currentScene_->GetCameraManager().LoadFromJson(cameraJsonPath);
	}
	Logger::Output("シーン遷移を完了しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
}
