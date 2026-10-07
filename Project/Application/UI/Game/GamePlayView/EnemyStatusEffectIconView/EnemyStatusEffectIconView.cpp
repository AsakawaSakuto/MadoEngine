#include "EnemyStatusEffectIconView.h"
#include ".SceneManager/SceneType.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Utility/Camera/Camera.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
	constexpr float kReferenceScreenWidth = 1280.0f;
	constexpr float kReferenceScreenHeight = 720.0f;
	constexpr std::uint64_t kStatusTypeBitCount = 8;
	constexpr const char* kIconObjectNamePrefix = "EnemyStatusEffectIcon_";
}

namespace UI::Game {
	void EnemyStatusEffectIconView::Initialize() {
		Finalize();
		isVisible_ = true;
	}

	void EnemyStatusEffectIconView::Update(
		const Enemy::Manager& enemyManager,
		const Camera& camera) {
		for (auto& [key, entry] : icons_) {
			(void)key;
			entry.ownerExists = false;
		}

		enemyManager.CollectStatusEffectDisplayData(displayData_);
		for (const Enemy::StatusEffectDisplayData& displayData : displayData_) {
			const std::size_t activeEffectCount = static_cast<std::size_t>(std::count(
				displayData.activeEffects.begin(),
				displayData.activeEffects.end(),
				true));
			Vector2 screenPosition;
			const Vector3 iconWorldPosition =
				displayData.anchorPosition + Vector3{ 0.0f, headWorldOffset_, 0.0f };
			const bool isOnScreen = activeEffectCount > 0 &&
				WorldToScreen(iconWorldPosition, camera, screenPosition);
			std::size_t activeEffectIndex = 0;

			// 同じEnemyの有効な状態異常だけを中央基準の横並びへ配置
			for (std::size_t typeIndex = 0;
				typeIndex < Enemy::StatusEffect::kTypeCount;
				++typeIndex) {
				const Enemy::StatusEffect::Type type =
					static_cast<Enemy::StatusEffect::Type>(typeIndex);
				const std::uint64_t iconKey = CreateIconKey(displayData.enemyId, type);
				auto iconIterator = icons_.find(iconKey);
				if (iconIterator != icons_.end()) {
					iconIterator->second.ownerExists = true;
				}

				if (!displayData.activeEffects[typeIndex]) {
					if (iconIterator != icons_.end()) {
						if (Sprite* sprite = MySprite::TryGet(iconIterator->second.sprite)) {
							sprite->SetVisible(false);
						}
					}
					continue;
				}

				if (iconIterator == icons_.end()) {
					IconEntry entry;
					entry.sprite = CreateIconSprite(displayData.enemyId, type);
					entry.ownerExists = true;
					iconIterator = icons_.emplace(iconKey, entry).first;
				}

				Sprite* sprite = MySprite::TryGet(iconIterator->second.sprite);
				if (!sprite) {
					++activeEffectIndex;
					continue;
				}

				const float groupWidth =
					static_cast<float>(activeEffectCount - 1) * iconSpacing_;
				const float iconOffsetX =
					-static_cast<float>(groupWidth) * 0.5f +
					static_cast<float>(activeEffectIndex) * iconSpacing_;
				if (isOnScreen) {
					sprite->SetPosition({ screenPosition.x + iconOffsetX, screenPosition.y });
				}
				sprite->SetVisible(isVisible_ && isOnScreen);
				++activeEffectIndex;
			}
		}

		// Enemyの破棄後は描画安全地点でSpriteも解放して管理数の増加を防止
		for (auto iconIterator = icons_.begin(); iconIterator != icons_.end();) {
			if (iconIterator->second.ownerExists) {
				++iconIterator;
				continue;
			}

			MadoEngine::SpriteManager::GetInstance().RequestDestroy(
				iconIterator->second.sprite);
			iconIterator = icons_.erase(iconIterator);
		}
	}

	void EnemyStatusEffectIconView::SetVisible(bool isVisible) {
		if (isVisible_ == isVisible) {
			return;
		}

		isVisible_ = isVisible;
		if (!isVisible_) {
			HideAllIcons();
		}
	}

	void EnemyStatusEffectIconView::Finalize() {
		for (const auto& [key, entry] : icons_) {
			(void)key;
			MySprite::Destroy(entry.sprite);
		}
		icons_.clear();
		displayData_.clear();
	}

	std::uint64_t EnemyStatusEffectIconView::CreateIconKey(
		std::uint32_t enemyId,
		Enemy::StatusEffect::Type type) {
		return (static_cast<std::uint64_t>(enemyId) << kStatusTypeBitCount) |
			static_cast<std::uint64_t>(type);
	}

	const char* EnemyStatusEffectIconView::GetTextureName(
		Enemy::StatusEffect::Type type) {
		switch (type) {
		case Enemy::StatusEffect::Type::Burn:
			return "Burn";
		case Enemy::StatusEffect::Type::Poison:
			return "Poison";
		case Enemy::StatusEffect::Type::Frozen:
			return "Frozen";
		case Enemy::StatusEffect::Type::Count:
		default:
			return "None";
		}
	}

	MadoEngine::SpriteHandle EnemyStatusEffectIconView::CreateIconSprite(
		std::uint32_t enemyId,
		Enemy::StatusEffect::Type type) const {
		const std::string objectName =
			std::string(kIconObjectNamePrefix) +
			std::to_string(enemyId) + "_" +
			std::to_string(static_cast<std::uint32_t>(type));
		const MadoEngine::SpriteHandle handle = MySprite::Create(
			objectName,
			GetTextureName(type),
			SceneType::Game,
			MadoEngine::EditorManagementMode::RuntimeOnly,
			MadoEngine::Render::RenderLayer::UI);
		if (Sprite* sprite = MySprite::TryGet(handle)) {
			sprite->SetAnchorPoint({ 0.5f, 0.5f });
			const Vector2 textureSize = sprite->GetTextureSize();

			// 画像解像度に依存せず画面上のアイコン寸法を一定化
			if (textureSize.x > 0.0f && textureSize.y > 0.0f) {
				sprite->SetScale({
					iconPixelSize_ / textureSize.x,
					iconPixelSize_ / textureSize.y,
				});
			}
			sprite->SetVisible(false);
		}
		return handle;
	}

	bool EnemyStatusEffectIconView::WorldToScreen(
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

		// Icon半径を含めて画面外の場合は全体を非表示
		const float halfIconSize = iconPixelSize_ * 0.5f;
		return outScreenPosition.x >= -halfIconSize &&
			outScreenPosition.x <= kReferenceScreenWidth + halfIconSize &&
			outScreenPosition.y >= -halfIconSize &&
			outScreenPosition.y <= kReferenceScreenHeight + halfIconSize;
	}

	void EnemyStatusEffectIconView::HideAllIcons() {
		for (const auto& [key, entry] : icons_) {
			(void)key;
			if (Sprite* sprite = MySprite::TryGet(entry.sprite)) {
				sprite->SetVisible(false);
			}
		}
	}
} // namespace UI::Game
