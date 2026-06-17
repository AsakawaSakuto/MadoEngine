#include "Test.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

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

	fadeSprite_ = MySprite::Create("testFade", "black128x72", SceneType::Test);
	fadeSprite_->SetScale({ 10.0f, 10.0f });
	fadeSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	fadeSprite_->SetRenderLayer(MadoEngine::Render::RenderLayer::Default);

	fadeOutTimer_.Start(2.0f);
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

#endif // USE_IMGUI
}

void Test::Finalize() {
	MyCollider::RemoveColliderAll();
	MySprite::DestroyByScene(SceneType::Test);
	MyModel::DestroyByScene(SceneType::Test);

	Logger::Output("テストシーンの終了処理を実行しました", Logger::Level::Application);
}
