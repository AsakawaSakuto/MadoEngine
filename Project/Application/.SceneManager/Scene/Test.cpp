#include "Test.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <cmath>
#include <format>

namespace {
	constexpr float kFpsTextUpdateInterval = 0.25f;
	constexpr float kGameSceneTimeLimit = 10.0f * 60.0f;
	constexpr std::uint32_t kWeaponUpgradeRandomSeed = 0x4d41444fu;
}

Test::Test()
	
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

}

SceneType Test::Update(float dt) {
	
	return SceneType::Test;
}

void Test::Draw() {
	
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI

	tpsCamera_.DrawImGui();

#endif // USE_IMGUI
}

Vector3 Test::GetShadowFocusPosition() const {
	return Vector3{ 0.0f, 0.0f, 0.0f };
}

bool Test::TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
	
	return true;
}

void Test::Finalize() {
	
	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
