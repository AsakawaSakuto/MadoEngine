#include "EnemyDamageView.h"
#include ".SceneManager/SceneType.h"
#include "GameObject/Combat/DamageEvent.h"
#include "GameObject/Enemy/EnemyStatusEffectVisualSettings.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Camera/Camera.h"
#ifdef USE_IMGUI
#include "imguiHeaders.h"
#endif
#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace {
	constexpr float kReferenceScreenWidth = 1280.0f;
	constexpr float kReferenceScreenHeight = 720.0f;

	constexpr std::array<float, 5> kOffsetRates = { 
		0.5f,
		0.0f,
		1.0f,
		0.25f,
		0.75f,
	};
	constexpr const char* kTextObjectNamePrefix = "EnemyDamageText_";

	/// @brief ダメージ量を表示用文字列へ変換
	/// @param damage 表示するダメージ量
	/// @return 小数点以下を切り捨てた文字列
	std::string FormatDamage(float damage) {
		return std::format("{:.0f}", std::floor(damage));
	}
}

namespace UI::Game {

	void EnemyDamageView::Initialize() {
		Finalize();

		// 実行中の生成破棄を避ける固定数Text Poolを事前構築
		for (std::size_t index = 0; index < slots_.size(); ++index) {
			DamageTextSlot& slot = slots_[index];
			slot.text = MyText::Create(
				kTextObjectNamePrefix + std::to_string(index),
				"0",
				SceneType::Game,
				MadoEngine::EditorManagementMode::RuntimeOnly,
				MadoEngine::Render::RenderLayer::UI);
			MadoEngine::Text* text = MyText::TryGet(slot.text);
			if (!text) {
				continue;
			}

			text->SetFontAsset("Assets/Font/dot.ttf", "dot");
			text->SetFontSize(fontSize_);
			text->SetAnchorPoint({ 0.5f, 0.5f });
			text->SetWordWrap(false);
			text->SetVisible(false);
			text->SetColor(damageTextColor_);
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
	}

	void EnemyDamageView::Spawn(const Combat::DamageEvent& event) {
		if (!std::isfinite(event.displayDamage) || std::floor(event.displayDamage) <= 0.0f) {
			return;
		}

		DamageTextSlot* slot = AcquireSlot();
		if (!slot) {
			return;
		}
		MadoEngine::Text* text = MyText::TryGet(slot->text);
		if (!text) {
			return;
		}

		// 連続表示が重ならないよう固定Sequenceから左右と高さのOffsetを分散
		slot->worldPosition = event.worldPosition;
		const std::size_t offsetIndex =
			static_cast<std::size_t>(spawnSequence_ % kOffsetRates.size());
		const std::size_t verticalOffsetIndex =
			(offsetIndex + 2) % kOffsetRates.size();
		slot->horizontalOffset = std::lerp(
			horizontalOffsetMin_,
			horizontalOffsetMax_,
			kOffsetRates[offsetIndex]);
		slot->verticalOffset = std::lerp(
			enemyHeadOffsetMin_,
			enemyHeadOffsetMax_,
			kOffsetRates[verticalOffsetIndex]);
		slot->elapsedTime = 0.0f;
		slot->type = ResolveDamageTextType(event);
		slot->isActive = true;
		++spawnSequence_;

		text->SetText(FormatDamage(event.displayDamage));
		text->SetScale({
			1.0f + initialScaleAddition_,
			1.0f + initialScaleAddition_,
		});
		text->SetColor(GetDamageTextColor(slot->type));
		text->SetVisible(isVisible_);
	}

	void EnemyDamageView::Update(float deltaTime, const Camera& camera) {
		if (!isVisible_) {
			return;
		}

		// Activeな固定Slotだけを更新して表示中の動的確保を回避
		for (DamageTextSlot& slot : slots_) {
			MadoEngine::Text* text = MyText::TryGet(slot.text);
			if (!slot.isActive || !text) {
				continue;
			}

			if (deltaTime > 0.0f) {
				slot.elapsedTime += deltaTime;
			}

			// 縮小演出の完了後にSlotを解放して次の表示へ再利用
			if (slot.elapsedTime >= displayLifeTime_ + shrinkDuration_) {
				slot.isActive = false;
				text->SetVisible(false);
				continue;
			}

			const Vector3 displayWorldPosition =
				slot.worldPosition + Vector3{ 0.0f, slot.verticalOffset, 0.0f };
			Vector2 screenPosition;

			// Camera外の座標は寿命を進めたまま描画だけを抑制
			if (!WorldToScreen(displayWorldPosition, camera, screenPosition)) {
				text->SetVisible(false);
				continue;
			}

			const float displayProgress =
				std::clamp(slot.elapsedTime / displayLifeTime_, 0.0f, 1.0f);

			// 表示時間中に初期拡大を収束させ、寿命経過時のScaleからゼロへ縮小
			const Vector4& textColor = GetDamageTextColor(slot.type);
			const float alpha = 1.0f;
			const float scaleSettleProgress =
				std::clamp(displayProgress / scaleSettleProgress_, 0.0f, 1.0f);
			const float displayScale = 1.0f +
				initialScaleAddition_ * (1.0f - scaleSettleProgress);
			const float shrinkProgress = std::clamp(
				(slot.elapsedTime - displayLifeTime_) / shrinkDuration_,
				0.0f,
				1.0f);
			const float scale = displayScale * (1.0f - shrinkProgress);

			screenPosition.x += slot.horizontalOffset;
			screenPosition.y -= riseDistance_ * displayProgress;

			text->SetPosition(screenPosition);
			text->SetScale({ scale, scale });
			text->SetColor({
				textColor.x,
				textColor.y,
				textColor.z,
				alpha,
			});
			text->SetVisible(true);
		}
	}

	void EnemyDamageView::SetVisible(bool isVisible) {
		if (isVisible_ == isVisible) {
			return;
		}

		isVisible_ = isVisible;
		if (isVisible_) {
			return;
		}

		// 非表示中もSlot寿命を保持し、再表示時に残存表示を継続
		for (DamageTextSlot& slot : slots_) {
			if (MadoEngine::Text* text = MyText::TryGet(slot.text)) {
				text->SetVisible(false);
			}
		}
	}

	void EnemyDamageView::Finalize() {
		for (std::size_t index = 0; index < slots_.size(); ++index) {
			DamageTextSlot& slot = slots_[index];
			slot = {};
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
	}

	void EnemyDamageView::DrawImGui() {
#ifdef USE_IMGUI
		ImGui::Begin("Enemy Damage View");

		ImGui::DragFloat("表示時間", &displayLifeTime_, 0.01f, 0.05f, 5.0f, "%.2f 秒");
		ImGui::DragFloat("縮小時間", &shrinkDuration_, 0.01f, 0.01f, 1.0f, "%.2f 秒");
		ImGui::DragFloat("スケール整定位置", &scaleSettleProgress_, 0.01f, 0.01f, 1.0f, "%.2f");
		ImGui::DragFloat("初期スケール加算", &initialScaleAddition_, 0.01f, 0.0f, 3.0f, "%.2f");
		ImGui::DragFloat("上昇距離", &riseDistance_, 1.0f, -500.0f, 500.0f, "%.0f px");
		ImGui::DragFloatRange2(
			"頭上オフセット範囲",
			&enemyHeadOffsetMin_,
			&enemyHeadOffsetMax_,
			0.05f,
			-10.0f,
			10.0f,
			"最小: %.2f",
			"最大: %.2f");
		ImGui::DragFloatRange2(
			"左右オフセット範囲",
			&horizontalOffsetMin_,
			&horizontalOffsetMax_,
			1.0f,
			-500.0f,
			500.0f,
			"最小: %.0f px",
			"最大: %.0f px");
		const bool fontSizeChanged =
			ImGui::DragFloat("フォントサイズ", &fontSize_, 1.0f, 1.0f, 200.0f, "%.0f px");
		ImGui::ColorEdit4("通常ダメージ文字色", &damageTextColor_.x);
		ImGui::ColorEdit4("クリティカルダメージ文字色", &criticalDamageTextColor_.x);
		Enemy::StatusEffect::VisualSettings& visualSettings =
			Enemy::StatusEffect::VisualSettings::GetInstance();
		ImGui::ColorEdit4(
			"火傷ダメージ文字色",
			&visualSettings.Edit(Enemy::StatusEffect::Type::Burn).damageTextColor.x);
		ImGui::ColorEdit4(
			"毒ダメージ文字色",
			&visualSettings.Edit(Enemy::StatusEffect::Type::Poison).damageTextColor.x);

		// 手入力を含む調整値をUpdate内の除算と補間が安全な範囲へ制限
		displayLifeTime_ = std::clamp(displayLifeTime_, 0.05f, 5.0f);
		shrinkDuration_ = std::clamp(shrinkDuration_, 0.01f, 1.0f);
		scaleSettleProgress_ = std::clamp(scaleSettleProgress_, 0.01f, 1.0f);
		initialScaleAddition_ = std::clamp(initialScaleAddition_, 0.0f, 3.0f);
		riseDistance_ = std::clamp(riseDistance_, -500.0f, 500.0f);
		enemyHeadOffsetMin_ = std::clamp(enemyHeadOffsetMin_, -10.0f, 10.0f);
		enemyHeadOffsetMax_ = std::clamp(enemyHeadOffsetMax_, -10.0f, 10.0f);
		horizontalOffsetMin_ = std::clamp(horizontalOffsetMin_, -500.0f, 500.0f);
		horizontalOffsetMax_ = std::clamp(horizontalOffsetMax_, -500.0f, 500.0f);
		if (enemyHeadOffsetMin_ > enemyHeadOffsetMax_) {
			std::swap(enemyHeadOffsetMin_, enemyHeadOffsetMax_);
		}
		if (horizontalOffsetMin_ > horizontalOffsetMax_) {
			std::swap(horizontalOffsetMin_, horizontalOffsetMax_);
		}
		fontSize_ = std::clamp(fontSize_, 1.0f, 200.0f);

		if (fontSizeChanged) {
			for (DamageTextSlot& slot : slots_) {
				if (MadoEngine::Text* text = MyText::TryGet(slot.text)) {
					text->SetFontSize(fontSize_);
				}
			}
		}

		ImGui::End();
#endif
	}

	EnemyDamageView::DamageTextSlot* EnemyDamageView::AcquireSlot() {

		// 次回位置から未使用Slotを循環探索して表示順の偏りを防止
		for (std::size_t offset = 0; offset < slots_.size(); ++offset) {
			const std::size_t index = (nextSlotIndex_ + offset) % slots_.size();
			DamageTextSlot& slot = slots_[index];
			if (!MyText::TryGet(slot.text) || slot.isActive) {
				continue;
			}

			nextSlotIndex_ = (index + 1) % slots_.size();
			return &slot;
		}

		// 全Slot使用中は古い順に上書きしてPool容量を一定に維持
		for (std::size_t offset = 0; offset < slots_.size(); ++offset) {
			const std::size_t index = (nextSlotIndex_ + offset) % slots_.size();
			DamageTextSlot& slot = slots_[index];
			if (!MyText::TryGet(slot.text)) {
				continue;
			}

			nextSlotIndex_ = (index + 1) % slots_.size();
			return &slot;
		}

		return nullptr;
	}

	EnemyDamageView::DamageTextType EnemyDamageView::ResolveDamageTextType(
		const Combat::DamageEvent& event) const {

		// 継続ダメージはクリティカル表示より発生元の状態異常色を優先
		switch (event.sourceType) {
		case Combat::DamageSourceType::Burn:
			return DamageTextType::Burn;
		case Combat::DamageSourceType::Poison:
			return DamageTextType::Poison;
		case Combat::DamageSourceType::Weapon:
		default:
			break;
		}

		return event.isCritical ? DamageTextType::Critical : DamageTextType::Normal;
	}

	const Vector4& EnemyDamageView::GetDamageTextColor(DamageTextType type) const {
		switch (type) {
		case DamageTextType::Critical:
			return criticalDamageTextColor_;
		case DamageTextType::Burn:
			return Enemy::StatusEffect::VisualSettings::GetInstance()
				.Get(Enemy::StatusEffect::Type::Burn).damageTextColor;
		case DamageTextType::Poison:
			return Enemy::StatusEffect::VisualSettings::GetInstance()
				.Get(Enemy::StatusEffect::Type::Poison).damageTextColor;
		case DamageTextType::Normal:
		default:
			return damageTextColor_;
		}
	}

	bool EnemyDamageView::WorldToScreen(
		const Vector3& worldPosition,
		const Camera& camera,
		Vector2& outScreenPosition) const {
		const Vector3 viewPosition =
			Matrix::Transform(worldPosition, camera.GetViewMatrix());

		// Camera前後範囲外と非有限値を射影前に除外
		if (!std::isfinite(viewPosition.x) ||
			!std::isfinite(viewPosition.y) ||
			!std::isfinite(viewPosition.z) ||
			viewPosition.z < camera.GetNearClip() ||
			viewPosition.z > camera.GetFarClip()) {
			return false;
		}

		const Vector3 normalizedDevicePosition =
			Matrix::Transform(worldPosition, camera.GetViewProjectionMatrix());

		// NDCの深度範囲外を除外して背面座標の画面反転を防止
		if (!std::isfinite(normalizedDevicePosition.x) ||
			!std::isfinite(normalizedDevicePosition.y) ||
			!std::isfinite(normalizedDevicePosition.z) ||
			normalizedDevicePosition.z < 0.0f ||
			normalizedDevicePosition.z > 1.0f) {
			return false;
		}

		outScreenPosition = {
			(normalizedDevicePosition.x + 1.0f) * 0.5f * kReferenceScreenWidth,
			(1.0f - normalizedDevicePosition.y) * 0.5f * kReferenceScreenHeight,
		};

		// 基準解像度のViewport内へ収まる座標だけを表示対象として返却
		return outScreenPosition.x >= 0.0f &&
			outScreenPosition.x <= kReferenceScreenWidth &&
			outScreenPosition.y >= 0.0f &&
			outScreenPosition.y <= kReferenceScreenHeight;
	}

} // namespace UI::Game
