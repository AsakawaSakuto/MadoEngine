#include "ProjectileDamageView.h"
#include ".SceneManager/SceneType.h"
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
	constexpr const char* kTextObjectNamePrefix = "ProjectileDamageText_";

	/// @brief ダメージ量を表示用文字列へ変換
	/// @param damage 表示するダメージ量
	/// @return 小数点以下を切り捨てた文字列
	std::string FormatDamage(float damage) {
		return std::format("{:.0f}", std::floor(damage));
	}
}

namespace UI::Game {

	void ProjectileDamageView::Initialize() {
		Finalize();

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

			text->SetFontFamily("Segoe UI");
			text->SetFontSize(fontSize_);
			text->SetAnchorPoint({ 0.5f, 0.5f });
			text->SetWordWrap(false);
			text->SetColor(damageTextColor_);
			text->SetVisible(false);
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
	}

	void ProjectileDamageView::Spawn(float damage, const Vector3& worldPosition) {
		if (!std::isfinite(damage) || damage <= 0.0f) {
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

		slot->worldPosition = worldPosition;
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
		slot->isActive = true;
		++spawnSequence_;

		text->SetText(FormatDamage(damage));
		text->SetScale({
			1.0f + initialScaleAddition_,
			1.0f + initialScaleAddition_,
		});
		text->SetColor(damageTextColor_);
		text->SetVisible(isVisible_);
	}

	void ProjectileDamageView::Update(float deltaTime, const Camera& camera) {
		if (!isVisible_) {
			return;
		}

		for (DamageTextSlot& slot : slots_) {
			MadoEngine::Text* text = MyText::TryGet(slot.text);
			if (!slot.isActive || !text) {
				continue;
			}

			if (deltaTime > 0.0f) {
				slot.elapsedTime += deltaTime;
			}

			if (slot.elapsedTime >= displayLifeTime_) {
				slot.isActive = false;
				text->SetVisible(false);
				continue;
			}

			const Vector3 displayWorldPosition =
				slot.worldPosition + Vector3{ 0.0f, slot.verticalOffset, 0.0f };
			Vector2 screenPosition;
			if (!WorldToScreen(displayWorldPosition, camera, screenPosition)) {
				text->SetVisible(false);
				continue;
			}

			const float progress =
				std::clamp(slot.elapsedTime / displayLifeTime_, 0.0f, 1.0f);
			const float fadeProgress = std::clamp(
				(progress - fadeStartProgress_) / (1.0f - fadeStartProgress_),
				0.0f,
				1.0f);
			const float alpha = damageTextColor_.w * (1.0f - fadeProgress);
			const float scaleSettleProgress =
				std::clamp(progress / scaleSettleProgress_, 0.0f, 1.0f);
			const float scale = 1.0f +
				initialScaleAddition_ * (1.0f - scaleSettleProgress);

			screenPosition.x += slot.horizontalOffset;
			screenPosition.y -= riseDistance_ * progress;

			text->SetPosition(screenPosition);
			text->SetScale({ scale, scale });
			text->SetColor({
				damageTextColor_.x,
				damageTextColor_.y,
				damageTextColor_.z,
				alpha,
			});
			text->SetVisible(true);
		}
	}

	void ProjectileDamageView::SetVisible(bool isVisible) {
		if (isVisible_ == isVisible) {
			return;
		}

		isVisible_ = isVisible;
		if (isVisible_) {
			return;
		}

		for (DamageTextSlot& slot : slots_) {
			if (MadoEngine::Text* text = MyText::TryGet(slot.text)) {
				text->SetVisible(false);
			}
		}
	}

	void ProjectileDamageView::Finalize() {
		for (std::size_t index = 0; index < slots_.size(); ++index) {
			DamageTextSlot& slot = slots_[index];
			slot = {};
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
	}

	void ProjectileDamageView::DrawImGui() {
#ifdef USE_IMGUI
		ImGui::Begin("Projectile Damage View");

		ImGui::DragFloat("表示時間", &displayLifeTime_, 0.01f, 0.05f, 5.0f, "%.2f 秒");
		ImGui::DragFloat("フェード開始位置", &fadeStartProgress_, 0.01f, 0.0f, 0.99f, "%.2f");
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
		ImGui::ColorEdit4("文字色", &damageTextColor_.x);

		displayLifeTime_ = std::clamp(displayLifeTime_, 0.05f, 5.0f);
		fadeStartProgress_ = std::clamp(fadeStartProgress_, 0.0f, 0.99f);
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

	ProjectileDamageView::DamageTextSlot* ProjectileDamageView::AcquireSlot() {
		for (std::size_t offset = 0; offset < slots_.size(); ++offset) {
			const std::size_t index = (nextSlotIndex_ + offset) % slots_.size();
			DamageTextSlot& slot = slots_[index];
			if (!MyText::TryGet(slot.text) || slot.isActive) {
				continue;
			}

			nextSlotIndex_ = (index + 1) % slots_.size();
			return &slot;
		}

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

	bool ProjectileDamageView::WorldToScreen(
		const Vector3& worldPosition,
		const Camera& camera,
		Vector2& outScreenPosition) const {
		const Vector3 viewPosition =
			Matrix::Transform(worldPosition, camera.GetViewMatrix());
		if (!std::isfinite(viewPosition.x) ||
			!std::isfinite(viewPosition.y) ||
			!std::isfinite(viewPosition.z) ||
			viewPosition.z < camera.GetNearClip() ||
			viewPosition.z > camera.GetFarClip()) {
			return false;
		}

		const Vector3 normalizedDevicePosition =
			Matrix::Transform(worldPosition, camera.GetViewProjectionMatrix());
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

		return outScreenPosition.x >= 0.0f &&
			outScreenPosition.x <= kReferenceScreenWidth &&
			outScreenPosition.y >= 0.0f &&
			outScreenPosition.y <= kReferenceScreenHeight;
	}

} // namespace UI::Game
