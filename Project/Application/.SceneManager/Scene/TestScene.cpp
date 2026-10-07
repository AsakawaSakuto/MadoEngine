#include "TestScene.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <cmath>
#include <format>

TestScene::TestScene()
	
{}

TestScene::~TestScene() {}

void TestScene::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	debugCameraHandle_ = cameraManager_.CreateCamera<DebugCamera>("TestDebugCamera");
	tpsCameraHandle_ = cameraManager_.CreateCamera<TPS_Camera>("TestTpsCamera");
	if (DebugCamera* debugCamera = cameraManager_.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
		debugCamera->SetPosition({ 0.0f, 10.0f, -20.0f });
	}
	cameraManager_.CutTo(debugCameraHandle_);
}

SceneType TestScene::Update(float dt) {
	cameraManager_.Update(dt);
	return SceneType::Test;
}

void TestScene::Draw() {
	
}

void TestScene::DrawImGui() {
#ifdef USE_IMGUI

	if (TPS_Camera* tpsCamera = cameraManager_.TryGetCamera<TPS_Camera>(tpsCameraHandle_)) {
		tpsCamera->DrawImGui();
	}

#endif // USE_IMGUI
}

Vector3 TestScene::GetShadowFocusPosition() const {
	return Vector3{ 0.0f, 0.0f, 0.0f };
}

bool TestScene::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	
	return true;
}

void TestScene::Finalize() {
	cameraManager_.Clear();
	debugCameraHandle_ = {};
	tpsCameraHandle_ = {};
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
