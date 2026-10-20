#include "MiniMapUI.h"
#include ".SceneManager/SceneType.h"
#include "Core/TextureManager/TextureManager.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
	constexpr const char* kBlockTextureName = "BlockMinMap";
	constexpr const char* kSlopeTextureName = "SlopeMinMap";
	constexpr std::uint32_t kMaximumCellCountPerAxis = 128;
	constexpr float kMapZoom = 2.0f;
	constexpr float kMinimumPanelSize = 64.0f;
	constexpr float kScreenMargin = 16.0f;
	constexpr float kContentPadding = 10.0f;
	constexpr float kFrameThickness = 2.0f;
	constexpr float kBossMarkerSize = 7.0f;
	constexpr float kPlayerMarkerSize = 24.0f;
	constexpr float kObjectMarkerSize = 16.0f;

	/// @brief イベントObjectのアイコン画像と表示寸法
	struct ObjectMarkerStyle {
		const char* name;
		const char* textureName;
		float pixelSize;
		Vector4 color;
	};

	constexpr std::array kObjectMarkerStyles = {
		ObjectMarkerStyle{ "BossSpawner", "white2x2", kBossMarkerSize, { 1.0f, 0.06f, 0.08f, 1.0f } },
		ObjectMarkerStyle{ "Chest", "ChestMinMap", kObjectMarkerSize, { 1.0f, 1.0f, 1.0f, 1.0f } },
		ObjectMarkerStyle{ "Jar", "JarMinMap", kObjectMarkerSize, { 1.0f, 1.0f, 1.0f, 1.0f } },
		ObjectMarkerStyle{ "Karma", "KarmaMinMap", kObjectMarkerSize, { 1.0f, 1.0f, 1.0f, 1.0f } },
		ObjectMarkerStyle{ "Magnet", "MagnetMinMap", kObjectMarkerSize, { 1.0f, 1.0f, 1.0f, 1.0f } },
		ObjectMarkerStyle{ "Anvil", "AnvilMinMap", kObjectMarkerSize, { 1.0f, 1.0f, 1.0f, 1.0f } },
	};
	static_assert(kObjectMarkerStyles.size() == static_cast<std::size_t>(MapMiniMapObjectType::Count));

	/// @brief 地形の高度範囲からセルの表示色を補間
	/// @param height 地表の高さを表す段数
	/// @param data 高度範囲を含むMap情報
	/// @param minimumHeightColor 最低高度の表示色
	/// @param maximumHeightColor 最高高度の表示色
	/// @return 高度に対応する表示色
	Vector4 HeightToColor(std::uint32_t height, const MapMiniMapData& data,
		const Vector4& minimumHeightColor, const Vector4& maximumHeightColor) {

		// 全セルが同じ高さの場合は最低高度の色へ統一し、ゼロ除算を回避
		if (data.maxHeight <= data.minHeight) {
			return minimumHeightColor;
		}
		const float range = static_cast<float>(data.maxHeight) - static_cast<float>(data.minHeight);
		const float rate = std::clamp((static_cast<float>(height) - static_cast<float>(data.minHeight)) / range, 0.0f, 1.0f);
		return minimumHeightColor + (maximumHeightColor - minimumHeightColor) * rate;
	}

	/// @brief UI Layerへ単色Spriteを生成
	/// @param name Spriteの識別名
	/// @param color Spriteの表示色
	/// @return 生成したSpriteのHandle
	MadoEngine::SpriteHandle CreateSolidSprite(const std::string& name, const Vector4& color) {
		const MadoEngine::SpriteHandle handle = MySprite::Create(
			name, "white2x2", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		if (Sprite* sprite = MySprite::TryGet(handle)) {
			sprite->SetColor(color);
			sprite->SetVisible(false);
		}
		return handle;
	}

	/// @brief Texture解像度に依存しないSpriteの画面寸法を設定
	/// @param handle 対象SpriteのHandle
	/// @param size 画面上の幅と高さ
	void SetPixelSize(MadoEngine::SpriteHandle handle, const Vector2& size) {

		// 共有の白画像と生成画像で異なるTexture寸法を画面上のピクセル寸法へ補正
		if (Sprite* sprite = MySprite::TryGet(handle)) {
			const Vector2 textureSize = sprite->GetTextureSize();
			if (textureSize.x > 0.0f && textureSize.y > 0.0f) {
				sprite->SetScale({ size.x / textureSize.x, size.y / textureSize.y });
			}
		}
	}

	/// @brief 表示範囲に合わせて地形タイルの形状と回転後のUVを切り出し
	/// @param handle 地形タイルのSprite
	/// @param origin タイル全体の左上座標
	/// @param size タイル全体の画面寸法
	/// @param clipOrigin 表示範囲の左上座標
	/// @param clipSize 表示範囲の画面寸法
	/// @param direction 上向き画像に対応する坂の上り方向
	void SetClippedTerrainTile(MadoEngine::SpriteHandle handle,
		const Vector2& origin, const Vector2& size,
		const Vector2& clipOrigin, const Vector2& clipSize, SlopeDirection direction) {
		Sprite* sprite = MySprite::TryGet(handle);
		if (!sprite) {
			return;
		}
		const Vector2 clippedMin = {
			(std::max)(origin.x, clipOrigin.x), (std::max)(origin.y, clipOrigin.y),
		};
		const Vector2 clippedMax = {
			(std::min)(origin.x + size.x, clipOrigin.x + clipSize.x),
			(std::min)(origin.y + size.y, clipOrigin.y + clipSize.y),
		};
		if (clippedMax.x <= clippedMin.x || clippedMax.y <= clippedMin.y) {
			sprite->SetVisible(false);
			return;
		}
		const Vector2 uvOrigin = {
			std::clamp((clippedMin.x - origin.x) / size.x, 0.0f, 1.0f),
			std::clamp((clippedMin.y - origin.y) / size.y, 0.0f, 1.0f),
		};
		const Vector2 uvSize = {
			(clippedMax.x - clippedMin.x) / size.x,
			(clippedMax.y - clippedMin.y) / size.y,
		};
		Vector2 uvTranslation = uvOrigin;
		float uvRotation = 0.0f;
		constexpr float kHalfPi = 1.5707963267948966f;

		// 画面上のQuadを回さずUVだけを回転し、切り取った四角形と元画像の向きを一致
		switch (direction) {
		case SlopeDirection::PulsX:
			uvRotation = -kHalfPi;
			uvTranslation = { uvOrigin.y, 1.0f - uvOrigin.x };
			break;
		case SlopeDirection::MinusX:
			uvRotation = kHalfPi;
			uvTranslation = { 1.0f - uvOrigin.y, uvOrigin.x };
			break;
		case SlopeDirection::MinusZ:
			uvRotation = kHalfPi * 2.0f;
			uvTranslation = { 1.0f - uvOrigin.x, 1.0f - uvOrigin.y };
			break;
		case SlopeDirection::PulsZ:
			break;
		}
		const Vector2 textureSize = sprite->GetTextureSize();
		const Vector2 sourceSpan = { 1.0f - 1.0f / textureSize.x, 1.0f - 1.0f / textureSize.y };
		Vector2 imageUVSize = uvSize * sourceSpan;
		if (direction == SlopeDirection::PulsX || direction == SlopeDirection::MinusX) {
			imageUVSize = { uvSize.x * sourceSpan.y, uvSize.y * sourceSpan.x };
		}

		// 元画像の端のPixel中心へUVを対応させ、通常のSprite描画でも反対側の画素が端へ混ざる現象を抑制
		uvTranslation = uvTranslation * sourceSpan + Vector2{ 0.5f / textureSize.x, 0.5f / textureSize.y };
		sprite->SetVisible(true);
		sprite->SetPosition(clippedMin);
		SetPixelSize(handle, clippedMax - clippedMin);
		sprite->SetUVScale(imageUVSize);
		sprite->SetUVRotation(uvRotation);
		sprite->SetUVTranslate(uvTranslation);
	}

	/// @brief Textureの元の色を維持した中央基準のアイコンを生成
	/// @param name Spriteの識別名
	/// @param textureName アイコンのTexture名
	/// @param pixelSize アイコン全体の一辺の長さ
	/// @return 生成したSpriteのHandle
	MadoEngine::SpriteHandle CreateIconSprite(
		const std::string& name,
		const std::string& textureName,
		float pixelSize) {
		const MadoEngine::SpriteHandle handle = MySprite::Create(
			name, textureName, SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		if (Sprite* sprite = MySprite::TryGet(handle)) {
			sprite->SetAnchorPoint({ 0.5f, 0.5f });
			sprite->SetVisible(false);
		}
		SetPixelSize(handle, { pixelSize, pixelSize });
		return handle;
	}

	/// @brief 表示領域との交差部分だけにマーカー画像を配置
	/// @param handle 対象SpriteのHandle
	/// @param center マーカー全体の中心座標
	/// @param pixelSize マーカー全体の一辺の長さ
	/// @param clipOrigin 表示領域の左上座標
	/// @param clipSize 表示領域の幅と高さ
	void SetClippedMarker(
		MadoEngine::SpriteHandle handle,
		const Vector2& center,
		float pixelSize,
		const Vector2& clipOrigin,
		const Vector2& clipSize) {
		Sprite* sprite = MySprite::TryGet(handle);
		if (!sprite) {
			return;
		}
		if (!std::isfinite(center.x) || !std::isfinite(center.y)) {
			sprite->SetVisible(false);
			return;
		}
		const float halfSize = pixelSize * 0.5f;
		const Vector2 clippedMin = {
			(std::max)(center.x - halfSize, clipOrigin.x),
			(std::max)(center.y - halfSize, clipOrigin.y),
		};
		const Vector2 clippedMax = {
			(std::min)(center.x + halfSize, clipOrigin.x + clipSize.x),
			(std::min)(center.y + halfSize, clipOrigin.y + clipSize.y),
		};

		// 外周へ押し付けず、枠外のマーカーは非表示、境界上は交差する部分だけを描画
		if (clippedMax.x <= clippedMin.x || clippedMax.y <= clippedMin.y) {
			sprite->SetVisible(false);
			return;
		}
		sprite->SetVisible(true);
		sprite->SetPosition((clippedMin + clippedMax) * 0.5f);
		SetPixelSize(handle, clippedMax - clippedMin);

		// Quadと同じ割合でUVも切り出し、枠の境界でアイコン画像が縮まないよう元の比率を維持
		sprite->SetUVTranslate({
			std::clamp((clippedMin.x - center.x + halfSize) / pixelSize, 0.0f, 1.0f),
			std::clamp((clippedMin.y - center.y + halfSize) / pixelSize, 0.0f, 1.0f),
		});
		sprite->SetUVScale((clippedMax - clippedMin) / pixelSize);
	}
}

namespace UI::Game {

	void MiniMapUI::Initialize(const MapMiniMapData& data) {
		Finalize();

		// セル参照とワールド座標変換に必要な寸法を検証し、ゼロ除算と非有限値の伝播を防止
		if (data.width == 0 || data.depth == 0 ||
			data.width > kMaximumCellCountPerAxis || data.depth > kMaximumCellCountPerAxis ||
			data.cells.size() != static_cast<std::size_t>(data.width) * data.depth ||
			!std::isfinite(data.limit.min.x) || !std::isfinite(data.limit.max.x) ||
			!std::isfinite(data.limit.min.z) || !std::isfinite(data.limit.max.z) ||
			!std::isfinite(data.limit.max.x - data.limit.min.x) || !std::isfinite(data.limit.max.z - data.limit.min.z) ||
			data.limit.max.x <= data.limit.min.x || data.limit.max.z <= data.limit.min.z) {
			return;
		}
		std::uint32_t textureIndex = 0;
		MadoEngine::TextureManager& textures = MadoEngine::TextureManager::GetInstance();
		if (!textures.TryGetTextureIndex(kBlockTextureName, textureIndex) ||
			!textures.TryGetTextureIndex(kSlopeTextureName, textureIndex)) {
			return;
		}
		hasMapData_ = true;
		mapLimit_ = data.limit;
		terrainData_ = data;

		// 地形タイルとイベントObjectの後にPlayerを登録して現在位置のアイコンを最前面へ配置
		frame_ = CreateSolidSprite("MiniMap.Frame", { 0.35f, 0.45f, 0.55f, 1.0f });
		background_ = CreateSolidSprite("MiniMap.Background", { 0.025f, 0.035f, 0.05f, 0.95f });
		UpdateLayout();
		CreateTerrainTiles();
		for (std::size_t typeIndex = 0; typeIndex < objectMarkers_.size(); ++typeIndex) {
			const ObjectMarkerStyle& style = kObjectMarkerStyles[typeIndex];
			std::vector<MadoEngine::SpriteHandle>& markers = objectMarkers_[typeIndex];
			markers.reserve(data.objectPositions[typeIndex].size());
			for (std::size_t index = 0; index < data.objectPositions[typeIndex].size(); ++index) {
				const MadoEngine::SpriteHandle marker = CreateIconSprite(
					std::string("MiniMap.") + style.name + "." + std::to_string(index), style.textureName, style.pixelSize);
				if (Sprite* sprite = MySprite::TryGet(marker)) {
					sprite->SetColor(style.color);
				}
				markers.push_back(marker);
			}
		}
		playerMarker_ = CreateIconSprite("MiniMap.Player", "PlayerMinMap", kPlayerMarkerSize);
		Update({}, 0.0f, data.objectPositions, false);
	}

	void MiniMapUI::SetVisible(bool isVisible) {
		isVisible_ = isVisible;
		UpdateView();
	}

	void MiniMapUI::Update(
		const Vector3& playerPosition,
		float playerFacingYaw,
		const MapMiniMapObjectPositions& objectPositions,
		bool isVisible) {

		// 非有限Yawの描画行列への伝播を防ぎ、最後に取得した有効な向きを維持
		playerPosition_ = playerPosition;
		if (std::isfinite(playerFacingYaw)) {
			playerFacingYaw_ = playerFacingYaw;
		}
		objectPositions_ = objectPositions;
		isVisible_ = isVisible;
		if (hasMapData_) {
			UpdateLayout();
		}
		UpdateView();
	}

	void MiniMapUI::UpdateView() {
		const bool shouldShow = hasMapData_ && isVisible_ && hasLayout_ &&
			std::isfinite(playerPosition_.x) && std::isfinite(playerPosition_.z);
		for (MadoEngine::SpriteHandle handle : { frame_, background_ }) {
			if (Sprite* sprite = MySprite::TryGet(handle)) {
				sprite->SetVisible(shouldShow);
			}
		}
		if (!shouldShow) {

			// 表示無効時は以前の切り出し結果やマーカーを残さず、HUD全体を非表示
			for (MadoEngine::SpriteHandle handle : { playerMarker_ }) {
				if (Sprite* sprite = MySprite::TryGet(handle)) {
					sprite->SetVisible(false);
				}
			}
			for (MadoEngine::SpriteHandle tile : terrainTiles_) {
				if (Sprite* sprite = MySprite::TryGet(tile)) {
					sprite->SetVisible(false);
				}
			}
			for (const std::vector<MadoEngine::SpriteHandle>& markers : objectMarkers_) {
				for (MadoEngine::SpriteHandle handle : markers) {
					if (Sprite* sprite = MySprite::TryGet(handle)) {
						sprite->SetVisible(false);
					}
				}
			}
			return;
		}
		UpdateTerrainTiles();

		// Playerとの差分で周辺Objectを移動し、四辺と交差しないマーカーを非表示
		for (std::size_t typeIndex = 0; typeIndex < objectMarkers_.size(); ++typeIndex) {
			UpdateMarkers(objectMarkers_[typeIndex], objectPositions_[typeIndex], kObjectMarkerStyles[typeIndex].pixelSize);
		}

		// 上向き画像へ水平Yawを適用し、+Zを上、+Xを右とするミニマップの向きへ対応
		if (Sprite* sprite = MySprite::TryGet(playerMarker_)) {
			const Vector2 center = contentOrigin_ + contentSize_ * 0.5f;
			const float pixelSize = (std::min)(kPlayerMarkerSize,
				(std::min)(contentSize_.x, contentSize_.y) / std::sqrt(2.0f));
			sprite->SetVisible(true);
			sprite->SetPosition(center);
			sprite->SetRotation(playerFacingYaw_);
			SetPixelSize(playerMarker_, { pixelSize, pixelSize });
		}
	}

	void MiniMapUI::CreateTerrainTiles() {
		const float width = mapLimit_.max.x - mapLimit_.min.x;
		const float depth = mapLimit_.max.z - mapLimit_.min.z;
		const float visibleSpan = (std::max)(width, depth) / kMapZoom;
		const std::size_t columns = static_cast<std::size_t>(std::clamp(
			std::ceil(visibleSpan / (width / terrainData_.width)) + 1.0f, 1.0f, static_cast<float>(terrainData_.width)));
		const std::size_t rows = static_cast<std::size_t>(std::clamp(
			std::ceil(visibleSpan / (depth / terrainData_.depth)) + 1.0f, 1.0f, static_cast<float>(terrainData_.depth)));
		const std::size_t requiredCount = columns * rows;

		// 移動時に両端へ入る部分セルの分も初期化時に確保し、以降は同じSpriteを再利用
		terrainTiles_.reserve(requiredCount);
		for (std::size_t index = 0; index < requiredCount; ++index) {
			const MadoEngine::SpriteHandle tile = MySprite::Create(
				"MiniMap.TerrainTile." + std::to_string(index), kBlockTextureName,
				SceneType::Game, MadoEngine::Render::RenderLayer::UI);
			if (Sprite* sprite = MySprite::TryGet(tile)) {
				sprite->SetVisible(false);
				sprite->SetColor(minimumHeightColor_);
			}
			terrainTiles_.push_back(tile);
		}
	}

	void MiniMapUI::UpdateTerrainTiles() {
		for (MadoEngine::SpriteHandle tile : terrainTiles_) {
			if (Sprite* sprite = MySprite::TryGet(tile)) {
				sprite->SetVisible(false);
			}
		}
		const float cellWidth = (mapLimit_.max.x - mapLimit_.min.x) / terrainData_.width;
		const float cellDepth = (mapLimit_.max.z - mapLimit_.min.z) / terrainData_.depth;
		const Vector2 halfSpan = contentSize_ / (worldToMiniMapScale_ * 2.0f);
		const float minX = (std::max)(playerPosition_.x - halfSpan.x, mapLimit_.min.x);
		const float maxX = (std::min)(playerPosition_.x + halfSpan.x, mapLimit_.max.x);
		const float minZ = (std::max)(playerPosition_.z - halfSpan.y, mapLimit_.min.z);
		const float maxZ = (std::min)(playerPosition_.z + halfSpan.y, mapLimit_.max.z);
		if (minX >= maxX || minZ >= maxZ) {

			// PlayerがMap外へ出た場合も、交差しない地形を枠の端へ残さず非表示
			return;
		}
		const std::uint32_t firstX = static_cast<std::uint32_t>(std::clamp(
			std::floor((minX - mapLimit_.min.x) / cellWidth), 0.0f, static_cast<float>(terrainData_.width)));
		const std::uint32_t endX = static_cast<std::uint32_t>(std::clamp(
			std::ceil((maxX - mapLimit_.min.x) / cellWidth), 0.0f, static_cast<float>(terrainData_.width)));
		const std::uint32_t firstZ = static_cast<std::uint32_t>(std::clamp(
			std::floor((minZ - mapLimit_.min.z) / cellDepth), 0.0f, static_cast<float>(terrainData_.depth)));
		const std::uint32_t endZ = static_cast<std::uint32_t>(std::clamp(
			std::ceil((maxZ - mapLimit_.min.z) / cellDepth), 0.0f, static_cast<float>(terrainData_.depth)));
		std::size_t slot = 0;

		// 全セルを走査せず、表示範囲と交差するXZ添字の範囲だけを再利用Spriteへ割り当て
		for (std::uint32_t z = firstZ; z < endZ; ++z) {
			for (std::uint32_t x = firstX; x < endX; ++x) {
				const MapMiniMapCell& cell = terrainData_.cells[static_cast<std::size_t>(z) * terrainData_.width + x];
				if (cell.type == MapBlockType::Air) {

					// 空セルは地形画像を割り当てず、ミニマップの下地を表示
					continue;
				}
				if (slot >= terrainTiles_.size()) {
					return;
				}
				const MadoEngine::SpriteHandle tile = terrainTiles_[slot++];
				Sprite* sprite = MySprite::TryGet(tile);
				if (!sprite) {
					continue;
				}
				const char* textureName = kBlockTextureName;
				SlopeDirection direction = SlopeDirection::PulsZ;
				if (cell.type == MapBlockType::Slope) {
					textureName = kSlopeTextureName;
					direction = cell.slopeDirection;
				}
				if (sprite->GetTextureName() != textureName) {
					sprite->SetTexture(textureName);
				}

				// 高さの異なるセルへSpriteを再利用するため、CPUで補間した高度色を毎回適用
				sprite->SetColor(HeightToColor(cell.height, terrainData_, minimumHeightColor_, maximumHeightColor_));
				const Vector2 origin = WorldToMiniMap({
					mapLimit_.min.x + static_cast<float>(x) * cellWidth, 0.0f,
					mapLimit_.min.z + static_cast<float>(z + 1) * cellDepth,
				});

				// 隣接タイルと同じ端点を座標変換し、端数倍率でも境界の丸め誤差による隙間を抑制
				const Vector2 end = WorldToMiniMap({
					mapLimit_.min.x + static_cast<float>(x + 1) * cellWidth, 0.0f,
					mapLimit_.min.z + static_cast<float>(z) * cellDepth,
				});
				SetClippedTerrainTile(tile, origin, end - origin, contentOrigin_, contentSize_, direction);
			}
		}
	}

	void MiniMapUI::UpdateMarkers(
		const std::vector<MadoEngine::SpriteHandle>& markers,
		const std::vector<Vector3>& positions,
		float pixelSize) {

		// 初期化時のSprite枠を再利用し、開封や使用で減ったObjectに対応する余剰枠を非表示
		for (std::size_t index = 0; index < markers.size(); ++index) {
			if (index >= positions.size()) {
				if (Sprite* sprite = MySprite::TryGet(markers[index])) {
					sprite->SetVisible(false);
				}
				continue;
			}
			SetClippedMarker(markers[index], WorldToMiniMap(positions[index]),
				pixelSize, contentOrigin_, contentSize_);
		}
	}

	bool MiniMapUI::UpdateLayout() {
		const Vector2 screenSize = MadoEngine::SpriteManager::GetInstance().GetScreenSize();
		if (screenSize == screenSize_) {
			return hasLayout_;
		}
		screenSize_ = screenSize;
		hasLayout_ = false;
		const float panelSize = (std::min)((std::max)(kMinimumPanelSize, panelSize_),
			(std::min)(screenSize.x, screenSize.y) - kScreenMargin * 2.0f);

		// 最小化や極端に小さい画面では配置を保留して負の寸法を回避
		if (panelSize <= kContentPadding * 2.0f) {
			return false;
		}
		const Vector2 panelPosition = Vector2{ screenSize.x - panelSize - kScreenMargin, kScreenMargin } + positionOffset_;
		if (Sprite* sprite = MySprite::TryGet(frame_)) {
			sprite->SetPosition(panelPosition);
		}
		SetPixelSize(frame_, { panelSize, panelSize });
		if (Sprite* sprite = MySprite::TryGet(background_)) {
			sprite->SetPosition(panelPosition + Vector2{ kFrameThickness, kFrameThickness });
		}
		SetPixelSize(background_, { panelSize - kFrameThickness * 2.0f, panelSize - kFrameThickness * 2.0f });

		// 正方形の表示枠に共通倍率を適用し、長方形Mapでも地形の縦横比を維持
		const float extent = panelSize - kContentPadding * 2.0f;
		const float worldWidth = mapLimit_.max.x - mapLimit_.min.x;
		const float worldDepth = mapLimit_.max.z - mapLimit_.min.z;
		worldToMiniMapScale_ = extent / (std::max)(worldWidth, worldDepth) * kMapZoom;
		contentSize_ = { extent, extent };
		contentOrigin_ = panelPosition + Vector2{ kContentPadding, kContentPadding };
		hasLayout_ = true;
		return true;
	}

	Vector2 MiniMapUI::WorldToMiniMap(const Vector3& worldPosition) const {

		// Playerとの相対XZを等倍率で変換し、画面Y方向の反転で+Zを上へ統一
		return contentOrigin_ + contentSize_ * 0.5f + Vector2{
			(worldPosition.x - playerPosition_.x) * worldToMiniMapScale_,
			(playerPosition_.z - worldPosition.z) * worldToMiniMapScale_,
		};
	}

	void MiniMapUI::Finalize() {

		// GPU待機後に所有Spriteだけを解放し、共有するPNG Textureの寿命をTextureManagerへ委譲
		for (MadoEngine::SpriteHandle handle : { frame_, background_, playerMarker_ }) {
			MySprite::Destroy(handle);
		}
		for (MadoEngine::SpriteHandle tile : terrainTiles_) {
			MySprite::Destroy(tile);
		}
		terrainTiles_.clear();
		for (std::vector<MadoEngine::SpriteHandle>& markers : objectMarkers_) {
			for (MadoEngine::SpriteHandle handle : markers) {
				MySprite::Destroy(handle);
			}
			markers.clear();
		}
		for (std::vector<Vector3>& positions : objectPositions_) {
			positions.clear();
		}
		frame_ = {};
		background_ = {};
		playerMarker_ = {};
		terrainData_ = {};
		playerPosition_ = {};
		playerFacingYaw_ = 0.0f;
		worldToMiniMapScale_ = 0.0f;
		screenSize_ = {};
		contentOrigin_ = {};
		contentSize_ = {};
		hasMapData_ = false;
		hasLayout_ = false;
		isVisible_ = false;
	}

}
