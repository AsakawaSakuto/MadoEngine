#include "ChestOpeningPresentation.h"
#include "Audio/MyAudio.h"
#include "Input/MyInput.h"
#include "Math/Function/MathFunction.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
	constexpr float kMinimumDisplayDistance = 1.0f;
	constexpr float kReferenceDisplayDistance = 8.0f;
	constexpr float kReferenceScale = 0.65f;
	constexpr float kReferenceFovY = 0.45f;
	constexpr float kFlashDuration = 1.0f;
	constexpr float kOpeningPulsePitchStepSemitones = 2.0f;
	constexpr std::array<float, 4> kOpeningPulseScales = { 1.1f, 1.2f, 1.3f, 1.5f };

	// 各膨張段階までの待ち時間（秒）
	constexpr std::array<float, 4> kOpeningStepDelays = {
		0.25f, // A入力 → Uncommon
		0.50f, // Uncommon → Rare
		1.00f, // Rare → Epic
		1.50f, // Epic → Legendary
	};

	// 最終膨張からフラッシュ開始までの待ち時間（秒）
	constexpr std::array<float, 4> kFinalExpansionToFlashDelays = {
		0.50f, // Uncommon
		0.75f, // Rare
		1.00f, // Epic
		0.30f, // Legendary
	};

	/// @brief レアリティに対応する宝箱の膨張回数を取得
	/// @param rarity 抽選されたレリックのレアリティ
	/// @return 宝箱の膨張回数
	std::size_t GetOpeningPulseCount(Rarity rarity) {

		// 通常抽選対象外のUniqueも最大演出に揃え、未定義値は最小回数へ制限
		switch (rarity) {
		case Rarity::Uncommon:
			return 1;
		case Rarity::Rare:
			return 2;
		case Rarity::Epic:
			return 3;
		case Rarity::Legendary:
		case Rarity::Unique:
			return kOpeningPulseScales.size();
		default:
			return 1;
		}
	}
}

namespace UI::Game {

	void ChestOpeningPresentation::Initialize() {
		model_ = MyModel::Create("ChestOpeningModel", "Chest", SceneType::Game);
		flashSprite_ = MySprite::Create("ChestOpeningFlash", "white16x16", SceneType::Game,
			MadoEngine::Render::RenderLayer::ScreenEffect);

		if (Model* model = MyModel::TryGet(model_)) {

			// Camera基底で向きと移動量を扱い、Mapの照明と影への依存を解除
			model->SetTexture("Chest");
			model->SetLightingEnabled(false);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetUseBillboard(true);
			model->SetRotation({ 0.0f, 0.0f, 0.0f });
			const ModelSharedData* sharedData = model->GetSharedData();
			if (sharedData && sharedData->modelData.bounds.isValid) {
				const ModelBounds& bounds = sharedData->modelData.bounds;
				modelCenter_ = (bounds.min + bounds.max) * 0.5f;
			}
		}

		if (Sprite* flash = MySprite::TryGet(flashSprite_)) {
			flash->SetFitToScreen(true);
			flash->SetAnchorPoint({ 0.0f, 0.0f });
			flash->SetPosition({ 0.0f, 0.0f });
		}
		Hide();
	}

	void ChestOpeningPresentation::Finalize() {
		Hide();

		// 描画Systemが保持するGPU Resourceの参照終了後にModelとSpriteの破棄を確定
		MyModel::RequestDestroy(model_);
		MadoEngine::SpriteManager::GetInstance().RequestDestroy(flashSprite_);
		model_ = {};
		flashSprite_ = {};
		modelCenter_ = {};
	}

	void ChestOpeningPresentation::Show(Rarity rarity) {
		Model* model = MyModel::TryGet(model_);
		if (!model) {
			return;
		}

		// Model生成に成功した場合だけ演出を開始して獲得待ちの進行停止を回避
		Hide();

		// 獲得確定で保留報酬が解除された後も同じ色へ補間できるよう抽選時の色を保持
		const RarityColor& rarityColor = GetRarityDefaultColor(rarity);
		flashTargetColor_ = { rarityColor[0], rarityColor[1], rarityColor[2], 0.0f };

		// 未定義値はUncommonへ統一し、UniqueはLegendaryと同じフラッシュ待ち時間を使用
		if (FindRarityDefinition(rarity)) {
			openingRarity_ = rarity;
		}
		if (openingRarity_ == Rarity::Unique) {
			openingRarity_ = Rarity::Legendary;
		}
		openingPulseCount_ = GetOpeningPulseCount(rarity);
		state_ = State::WaitingForOpen;
		model->SetVisible(true);
	}

	void ChestOpeningPresentation::Hide() {

		// 演出の中断やScene終了時に宝箱と開封フラッシュの表示状態を同時に解除
		state_ = State::Hidden;
		openingRarity_ = Rarity::Uncommon;
		openingPulseCount_ = 1;
		appliedOpeningPulseCount_ = 0;
		openingElapsedTime_ = 0.0f;
		openingScaleMultiplier_ = 1.0f;
		flashRemainingTime_ = 0.0f;
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetVisible(false);
		}
		if (Sprite* flash = MySprite::TryGet(flashSprite_)) {
			flash->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			flash->SetVisible(false);
		}
	}

	bool ChestOpeningPresentation::Update(float deltaTime, bool acceptOpenInput) {
		UpdateFlash(deltaTime);

		// 開封待ちと膨張中はカードを閉じたまま維持し、獲得待機中だけ演出を進行
		if (!acceptOpenInput || !IsActive()) {
			return false;
		}

		// 開封開始後の追加入力による再開や演出の省略を防止
		if (state_ == State::Opening) {
			return UpdateOpening(deltaTime);
		}
		if (!MyInput::Trigger("OpenChest")) {
			return false;
		}

		// A入力を待ち時間の起点とし、遅延0の設定だけ入力Frameから膨張を反映
		state_ = State::Opening;
		openingElapsedTime_ = 0.0f;
		return UpdateOpening(0.0f);
	}

	bool ChestOpeningPresentation::UpdateOpening(float deltaTime) {
		openingElapsedTime_ += (std::max)(0.0f, deltaTime);
		float expansionTime = 0.0f;

		// 各段階までの累積時間で判定し、Frame内の超過時間を次の段階にも引き継ぎ
		for (std::size_t pulseIndex = 0; pulseIndex < openingPulseCount_; ++pulseIndex) {
			expansionTime += (std::max)(0.0f, kOpeningStepDelays[pulseIndex]);
			if (openingElapsedTime_ < expansionTime) {
				return false;
			}

			// 到達済みの膨張段階を除外し、倍率変更と効果音を各段階で一度だけ反映
			if (pulseIndex < appliedOpeningPulseCount_) {
				continue;
			}
			openingScaleMultiplier_ = kOpeningPulseScales[pulseIndex];
			++appliedOpeningPulseCount_;

			// 膨張段階から音程を算出し、次の宝箱も元の音程から開始
			MyAudio::SetPitch("ChestNextRarity", static_cast<float>(pulseIndex) * kOpeningPulsePitchStepSemitones);
			MyAudio::Play("ChestNextRarity");
		}

		// 抽選レアリティの最終倍率から専用時間だけ待機し、未到達の上位段階の待ち時間を除外
		const std::size_t rarityIndex = static_cast<std::size_t>(openingRarity_);
		const float flashStartTime = expansionTime + (std::max)(0.0f, kFinalExpansionToFlashDelays[rarityIndex]);
		if (openingElapsedTime_ >= flashStartTime) {
			StartFlash();
			return true;
		}
		return false;
	}

	void ChestOpeningPresentation::StartFlash() {
		Hide();

		// 膨張終了FrameはAlphaを1に保ち、次のFrameから透明化を開始
		state_ = State::Flashing;
		flashRemainingTime_ = kFlashDuration;
		MyAudio::Play("ChestOpen");
		if (Sprite* flash = MySprite::TryGet(flashSprite_)) {
			flash->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			flash->SetVisible(true);
		}
	}

	void ChestOpeningPresentation::UpdateFlash(float deltaTime) {
		if (flashRemainingTime_ <= 0.0f) {
			return;
		}

		// 獲得待ち解除後も継続し、負の時間入力で透明度が増加しないよう下限を制限
		flashRemainingTime_ = (std::max)(0.0f, flashRemainingTime_ - (std::max)(0.0f, deltaTime));
		if (Sprite* flash = MySprite::TryGet(flashSprite_)) {
			const float alpha = flashRemainingTime_ / kFlashDuration;

			// 色の補間と透明化を同じ進捗に同期し、レアリティ色のAlphaから独立
			Vector4 flashColor = Math::Lerp(Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }, flashTargetColor_, 1.0f - alpha);
			flashColor.w = alpha;
			flash->SetColor(flashColor);
			flash->SetVisible(flashRemainingTime_ > 0.0f);
		}
		if (flashRemainingTime_ <= 0.0f) {
			state_ = State::Hidden;
		}
	}

	void ChestOpeningPresentation::FollowCamera(const Camera& camera) {
		if (!IsActive()) {
			return;
		}

		Model* model = MyModel::TryGet(model_);
		if (!model) {
			return;
		}

		// Near Clipの手前を避け、距離と視野角に比例したScaleで画面上の大きさを維持
		const float distance = (std::max)(kMinimumDisplayDistance, camera.GetNearClip() * 4.0f);
		const float fovScale = std::tan(camera.GetFovY() * 0.5f) / std::tan(kReferenceFovY * 0.5f);
		const float baseScale = kReferenceScale * distance / kReferenceDisplayDistance * fovScale;

		// Camera補正と演出倍率を分離し、画面上の基準サイズを維持しながら膨張を反映
		const float scale = baseScale * openingScaleMultiplier_;
		model->SetScale({ scale, scale, scale });
		model->SetRotation({ 0.0f, 0.0f, 0.0f });

		// 原点が底面にあるModelも外形中心をCamera前方へ合わせるためCamera基底で補正
		const Matrix4x4 localMatrix = Matrix::MakeAffine(model->GetScale(), model->GetRotation(), {});
		const Vector3 centerOffset = Matrix::Transform(modelCenter_, localMatrix);
		model->SetCameraTranslateOffset(Vector3{ 0.0f, 0.0f, distance } - centerOffset);

		// Camera ShakeとBlendを含む実際の描画位置へ追従し、中心位置のずれを防止
		const Matrix4x4 cameraWorld = Matrix::Inverse(camera.GetViewMatrix());
		model->SetPosition({ cameraWorld.m[3][0], cameraWorld.m[3][1] - 0.03f, cameraWorld.m[3][2] });
	}

}
