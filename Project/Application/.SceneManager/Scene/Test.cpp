#include "Test.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <cmath>

namespace {

const char* kTestDirectionalLightName = "Test_DirectionalLight";
const char* kTestPointLightName = "Test_PointLight";
const char* kTestPointLightName2 = "Test_PointLight2";
const char* kTestSpotLightName = "Test_SpotLight";

/// @brief LightHandleが有効な場合だけLightManagerから破棄する
/// @param lightManager ライト管理クラス
/// @param handle 破棄対象のライトハンドル
void DestroyLightHandle(LightManager& lightManager, LightHandle& handle) {
	if (!handle.IsValid()) {
		return;
	}

	if (lightManager.IsValid(handle)) {
		lightManager.Destroy(handle);
	}

	handle = LightHandle{};
}

} // namespace

Test::Test()
	
{}

Test::~Test() {}

void Test::Initialize() {
	Logger::Output("テストシーンを初期化しました", Logger::Level::Application);

	for (int i = 0; i < sprites_.size(); ++i) {
		sprites_[i] = MySprite::Create("testSprite" + std::to_string(i), "uvChecker", SceneType::Test);
		sprites_[i]->SetPosition({ 32.0f * i, 32.0f * i });
		sprites_[i]->SetVisible(false);
	}

	debugCamera_.SetPosition({ 0.0f, 10.0f, -20.0f });

	sprite_ = std::make_unique<Sprite>("a");

	player_ = std::make_unique<Player>();
	player_->Initialize();
	player_->SetCamera(&tpsCamera_);

	/*MyCollider::CollisionPairSetting playerMapCollision;
	playerMapCollision.enableResolve = true;
	playerMapCollision.enableCCD = false;*/

	//MyCollider::RegisterCollisionPair(CollisionTag::PlayerHitBox, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, true);

	map_ = std::make_unique<Map>();
	map_->Initialize();

	model_ = MyModel::Create("testModel", "AnimatedCube", SceneType::Test);
	model_->SetPosition(modelPos_);

	RegisterTestLights();

	fadeSprite_ = MySprite::Create("testFade", "black128x72", SceneType::Test);
	fadeSprite_->SetScale({ 10.0f, 10.0f });
	fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	fadeSprite_->SetRenderLayer(MadoEngine::Render::RenderLayer::Default);

	fadeOutTimer_.Start(10.0f);
}

/// @brief テストシーン用ライトを登録する
void Test::RegisterTestLights() {
	LightManager& lightManager = LightManager::GetInstance();
	const LightLayerMask worldLayerMask = ToLightLayerMask(LightLayer::World);

	testDirectionalLightHandle_ = lightManager.Find(kTestDirectionalLightName);
	if (!lightManager.IsValid(testDirectionalLightHandle_)) {
		DirectionalLight directionalLight;
		directionalLight.color = { 1.0f, 0.96f, 0.86f, 1.0f };
		directionalLight.direction = { -0.35f, -1.0f, 0.25f };
		directionalLight.intensity = 0.45f;
		directionalLight.useLight = 1;
		directionalLight.useHalfLambert = 1;

		testDirectionalLightHandle_ = lightManager.CreateDirectionalLight(
			kTestDirectionalLightName,
			directionalLight,
			SceneType::Test,
			worldLayerMask);
	}

	testPointLightHandle_ = lightManager.Find(kTestPointLightName);
	if (!lightManager.IsValid(testPointLightHandle_)) {
		PointLight pointLight;
		pointLight.color = { 1.0f, 0.35f, 0.22f, 1.0f };
		pointLight.position = { modelPos_.x + 3.0f, modelPos_.y + 2.5f, modelPos_.z };
		pointLight.intensity = 2.5f;
		pointLight.radius = 8.0f;
		pointLight.decay = 1.2f;
		pointLight.useLight = 1;

		testPointLightHandle_ = lightManager.CreatePointLight(
			kTestPointLightName,
			pointLight,
			SceneType::Test,
			worldLayerMask);
	}

	testPointLightHandle2_ = lightManager.Find(kTestPointLightName2);
	if (!lightManager.IsValid(testPointLightHandle2_)) {
		PointLight pointLight;
		pointLight.color = { 0.25f, 0.75f, 1.0f, 1.0f };
		pointLight.position = { modelPos_.x - 5.0f, modelPos_.y + 3.0f, modelPos_.z + 4.0f };
		pointLight.intensity = 2.0f;
		pointLight.radius = 7.0f;
		pointLight.decay = 1.0f;
		pointLight.useLight = 1;

		testPointLightHandle2_ = lightManager.CreatePointLight(
			kTestPointLightName2,
			pointLight,
			SceneType::Test,
			worldLayerMask);
	}

	testSpotLightHandle_ = lightManager.Find(kTestSpotLightName);
	if (!lightManager.IsValid(testSpotLightHandle_)) {
		SpotLight spotLight;
		spotLight.color = { 0.35f, 0.65f, 1.0f, 1.0f };
		spotLight.position = { modelPos_.x - 4.0f, modelPos_.y + 6.0f, modelPos_.z - 3.0f };
		spotLight.intensity = 3.0f;
		spotLight.direction = { 0.35f, -1.0f, 0.25f };
		spotLight.distance = 14.0f;
		spotLight.decay = 1.0f;
		spotLight.cosAngle = std::cos(0.72f);
		spotLight.cosFalloffStart = std::cos(0.42f);
		spotLight.useLight = 1;

		testSpotLightHandle_ = lightManager.CreateSpotLight(
			kTestSpotLightName,
			spotLight,
			SceneType::Test,
			worldLayerMask);
	}
}
/// @brief テストシーンで登録したライトを破棄する
void Test::DestroyTestLights() {
	LightManager& lightManager = LightManager::GetInstance();

	DestroyLightHandle(lightManager, testDirectionalLightHandle_);
	DestroyLightHandle(lightManager, testPointLightHandle_);
	DestroyLightHandle(lightManager, testPointLightHandle2_);
	DestroyLightHandle(lightManager, testSpotLightHandle_);
}

/// @brief テストシーン用ライトを更新する
/// @param dt デルタタイム
void Test::UpdateTestLights(float dt) {
	testLightTime_ += dt;
	if (!animateTestPointLight_) {
		return;
	}

	PointLight* pointLight = LightManager::GetInstance().MutablePointLight(testPointLightHandle_);
	if (!pointLight) {
		return;
	}

	const float moveRadius = 4.0f;
	pointLight->position = {
		modelPos_.x + static_cast<float>(std::cos(testLightTime_)) * moveRadius,
		modelPos_.y + 2.5f + static_cast<float>(std::sin(testLightTime_ * 1.7f)) * 0.75f,
		modelPos_.z + static_cast<float>(std::sin(testLightTime_)) * moveRadius
	};
}

/// @brief テストライト編集用のImGuiを描画する
void Test::DrawTestLightImGui() {
#ifdef USE_IMGUI
	LightManager& lightManager = LightManager::GetInstance();

	ImGui::Begin("テストライト");

	if (lightManager.IsValid(testDirectionalLightHandle_)) {
		if (DirectionalLight* directionalLight = lightManager.MutableDirectionalLight(testDirectionalLightHandle_)) {
			if (ImGui::TreeNode("平行光源")) {
				bool enabled = lightManager.IsEnabled(testDirectionalLightHandle_);
				if (ImGui::Checkbox("有効##Directional", &enabled)) {
					lightManager.SetEnabled(testDirectionalLightHandle_, enabled);
				}

				ImGui::ColorEdit4("色##Directional", &directionalLight->color.x);
				ImGui::DragFloat3("方向##Directional", &directionalLight->direction.x, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat("強度##Directional", &directionalLight->intensity, 0.01f, 0.0f, 10.0f);

				bool useHalfLambert = directionalLight->useHalfLambert != 0;
				if (ImGui::Checkbox("ハーフランバート##Directional", &useHalfLambert)) {
					directionalLight->useHalfLambert = useHalfLambert ? 1u : 0u;
				}

				ImGui::TreePop();
			}
		}
	}

	if (lightManager.IsValid(testPointLightHandle_)) {
		if (PointLight* pointLight = lightManager.MutablePointLight(testPointLightHandle_)) {
			if (ImGui::TreeNode("点光源 1")) {
				bool enabled = lightManager.IsEnabled(testPointLightHandle_);
				if (ImGui::Checkbox("有効##Point", &enabled)) {
					lightManager.SetEnabled(testPointLightHandle_, enabled);
				}

				ImGui::Checkbox("自動移動##Point", &animateTestPointLight_);
				ImGui::ColorEdit4("色##Point", &pointLight->color.x);
				ImGui::DragFloat3("位置##Point", &pointLight->position.x, 0.05f);
				ImGui::DragFloat("強度##Point", &pointLight->intensity, 0.01f, 0.0f, 20.0f);
				ImGui::DragFloat("半径##Point", &pointLight->radius, 0.05f, 0.0f, 100.0f);
				ImGui::DragFloat("減衰##Point", &pointLight->decay, 0.01f, 0.0f, 10.0f);

				ImGui::TreePop();
			}
		}
	}

	if (lightManager.IsValid(testPointLightHandle2_)) {
		if (PointLight* pointLight = lightManager.MutablePointLight(testPointLightHandle2_)) {
			if (ImGui::TreeNode("点光源 2")) {
				bool enabled = lightManager.IsEnabled(testPointLightHandle2_);
				if (ImGui::Checkbox("有効##Point2", &enabled)) {
					lightManager.SetEnabled(testPointLightHandle2_, enabled);
				}

				ImGui::ColorEdit4("色##Point2", &pointLight->color.x);
				ImGui::DragFloat3("位置##Point2", &pointLight->position.x, 0.05f);
				ImGui::DragFloat("強度##Point2", &pointLight->intensity, 0.01f, 0.0f, 20.0f);
				ImGui::DragFloat("半径##Point2", &pointLight->radius, 0.05f, 0.0f, 100.0f);
				ImGui::DragFloat("減衰##Point2", &pointLight->decay, 0.01f, 0.0f, 10.0f);

				ImGui::TreePop();
			}
		}
	}

	if (lightManager.IsValid(testSpotLightHandle_)) {
		if (SpotLight* spotLight = lightManager.MutableSpotLight(testSpotLightHandle_)) {
			if (ImGui::TreeNode("スポットライト")) {
				bool enabled = lightManager.IsEnabled(testSpotLightHandle_);
				if (ImGui::Checkbox("有効##Spot", &enabled)) {
					lightManager.SetEnabled(testSpotLightHandle_, enabled);
				}

				ImGui::ColorEdit4("色##Spot", &spotLight->color.x);
				ImGui::DragFloat3("位置##Spot", &spotLight->position.x, 0.05f);
				ImGui::DragFloat3("方向##Spot", &spotLight->direction.x, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat("強度##Spot", &spotLight->intensity, 0.01f, 0.0f, 20.0f);
				ImGui::DragFloat("距離##Spot", &spotLight->distance, 0.05f, 0.0f, 100.0f);
				ImGui::DragFloat("減衰##Spot", &spotLight->decay, 0.01f, 0.0f, 10.0f);
				ImGui::SliderFloat("外角Cos##Spot", &spotLight->cosAngle, -1.0f, 1.0f);
				ImGui::SliderFloat("内角Cos##Spot", &spotLight->cosFalloffStart, -1.0f, 1.0f);

				ImGui::TreePop();
			}
		}
	}

	ImGui::End();
#endif // USE_IMGUI
}

SceneType Test::Update(float dt) {
	
	fadeOutTimer_.Update(dt);
	if (fadeOutTimer_.IsActive()) {
		fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOutTimer_.GetReverseProgress() });
	}

	//debugCamera_.Update();

	tpsCamera_.SetTargetPosition(player_->GetPosition());
	tpsCamera_.Update(dt);
	//debugCamera_.Update(dt);

	player_->Update(dt);

	map_->Update();

	if (model_) {
		model_->Update();
	}

	UpdateTestLights(dt);

	//sceneCamera_ = debugCamera_;
	sceneCamera_ = tpsCamera_;

	return SceneType::Test;
}

void Test::Draw() {
	
}

void Test::DrawImGui() {
	// テストシーンの描画処理
#ifdef USE_IMGUI

	tpsCamera_.DrawImGui();

	debugCamera_.DrawImGui();

	player_->DrawImGui();

	map_->DrawImGui();

	DrawTestLightImGui();

#endif // USE_IMGUI
}

void Test::Finalize() {
	DestroyTestLights();

	MyCollider::RemoveColliderAll();
	MySprite::DestroyByScene(SceneType::Test);
	MyModel::DestroyByScene(SceneType::Test);

	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
