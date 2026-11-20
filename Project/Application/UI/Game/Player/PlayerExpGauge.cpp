#include "PlayerExpGauge.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include <algorithm>

namespace UI::Game {

	void PlayerExpGauge::Initialize() {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		auto handle = manager.Find("PlayerExpGauge", SceneType::Game);

		// Editorで復元したGroupを再利用し、ゲーム側は現在値だけを供給
		if (!manager.TryGet(handle)) {
			handle = manager.CreateGauge("PlayerExpGauge", SceneType::Game);
		}
		gaugeId_ = manager.GetId(handle);
		isVisible_ = true;
		isUpgrading_ = false;

		dopaGauge_ = MySprite::Create("PlayerExpDopaGauge", "DopaGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		if (Sprite* sprite = MySprite::TryGet(dopaGauge_)) {
			sprite->SetRuntimeVisible(false);

			// UVスクロールのサブピクセル移動を滑らかに補間
			sprite->SetTextureFilter(Sprite::TextureFilter::Linear);
		}
	}

	void PlayerExpGauge::SetVisible(bool isVisible) {
		isVisible_ = isVisible;

		// ゲージと強化待機演出で同じ実行時の表示許可を使用
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(isVisible_);
		}
		if (Sprite* sprite = MySprite::TryGet(dopaGauge_)) {
			sprite->SetRuntimeVisible(isVisible_ && isUpgrading_);
		}
	}

	void PlayerExpGauge::Update(float currentExp, float maxExp) {

		// 最大値を先に減らして現在値を失わないよう、二つの値を一括更新
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(isVisible_);
			gauge->Update(currentExp, maxExp);
		}
	}

	void PlayerExpGauge::IsUpgrade(bool isUpgrade, float deltaTime) {
		isUpgrading_ = isUpgrade;
		if (Sprite* dopaGauge = MySprite::TryGet(dopaGauge_)) {
			auto* gauge = TryGetGauge();
			if (!gauge) {
				dopaGauge->SetRuntimeVisible(false);
				return;
			}

			// 再読込後も同じ親へ接続し、待機演出をゲージの移動・回転・フェードへ追従
			if (!dopaGauge->IsParent(gauge->GetGroup())) {
				dopaGauge->ClearParent();
				dopaGauge->SetParent(gauge->GetGroup());
			}
			const auto size = gauge->GetSize();
			const auto textureSize = dopaGauge->GetTextureSize();
			dopaGauge->SetScale({ size.x / (std::max)(1.0f, textureSize.x), size.y / (std::max)(1.0f, textureSize.y) });
			if (isUpgrade) {

				// 強化選択中はUVを横へ流して待機状態を視覚化
				dopaGauge->SetRuntimeVisible(isVisible_);

				uvOffsetX_ += uvSpeed_ * deltaTime;
				dopaGauge->SetUVTranslate(Vector2{ uvOffsetX_, 0.0f });
			} else {

				// 次回表示を同じ位相から始めるためUV進捗を初期化
				dopaGauge->SetRuntimeVisible(false);
				uvOffsetX_ = 0.0f;
			}
		}
	}

	Gauge2d* PlayerExpGauge::TryGetGauge() const {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		return manager.TryGetGauge(manager.FindById(gaugeId_));
	}
}
