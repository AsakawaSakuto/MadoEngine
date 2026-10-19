#pragma once
#include "GameObject/Map/MapMiniMapData.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/ObjectHandle.h"
#include <vector>

namespace UI::Game {

	/// @brief Playerを中心に周辺地形を拡大表示するHUD
	class MiniMapUI final {
	public:
		/// @brief GPU待機済みの時点で地形タイルとマーカーを初期化
		/// @param data 生成済みMapから取得した表示情報
		void Initialize(const MapMiniMapData& data);

		/// @brief 現在位置とスクリーンサイズを表示へ反映
		/// @param playerPosition Playerのワールド座標
		/// @param playerFacingYaw Playerが向いている水平Yaw角度（ラジアン）
		/// @param objectPositions 現在残っているイベントObjectの種別ごとのワールド座標
		/// @param isVisible 表示する場合はtrue
		void Update(
			const Vector3& playerPosition,
			float playerFacingYaw,
			const MapMiniMapObjectPositions& objectPositions,
			bool isVisible);

		/// @brief GPU待機済みの時点で所有するSpriteを解放
		void Finalize();

	private:
		/// @brief スクリーンと表示設定に応じた配置と倍率を更新
		/// @return 描画可能な配置がある場合はtrue
		bool UpdateLayout();

		/// @brief 現在位置を基準に地形とマーカーの表示範囲を更新
		void UpdateView();

		/// @brief 表示範囲へ入るセルに再利用可能な地形タイルを配置
		void UpdateTerrainTiles();

		/// @brief 初期化時に固定の拡大率へ必要な地形タイルを一括生成
		void CreateTerrainTiles();

		/// @brief ワールド座標一覧に対応するマーカーを表示範囲へ配置
		/// @param markers 初期化済みマーカーのHandle一覧
		/// @param positions 現在残っているObjectのワールド座標一覧
		/// @param pixelSize マーカー全体の一辺の長さ
		void UpdateMarkers(
			const std::vector<MadoEngine::SpriteHandle>& markers,
			const std::vector<Vector3>& positions,
			float pixelSize);

		/// @brief Playerを中心にワールドのXZ座標をスクリーン座標へ変換
		/// @param worldPosition 変換対象のワールド座標
		/// @return 表示範囲外も含むスクリーン座標
		Vector2 WorldToMiniMap(const Vector3& worldPosition) const;

		MapLimit mapLimit_;
		MapMiniMapData terrainData_;
		Vector4 minimumHeightColor_ = { 32.0f / 255.0f, 32.0f / 255.0f, 32.0f / 255.0f, 1.0f };
		Vector4 maximumHeightColor_ = { 160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f };
		Vector2 positionOffset_ = { -4.0f, 36.0f };
		float panelSize_ = 240.0f;
		float worldToMiniMapScale_ = 0.0f;
		Vector3 playerPosition_{};
		float playerFacingYaw_ = 0.0f;
		MapMiniMapObjectPositions objectPositions_;
		Vector2 screenSize_{};
		Vector2 contentOrigin_{};
		Vector2 contentSize_{};
		MadoEngine::SpriteHandle frame_{};
		MadoEngine::SpriteHandle background_{};
		std::vector<MadoEngine::SpriteHandle> terrainTiles_;
		MadoEngine::SpriteHandle playerMarker_{};
		std::array<std::vector<MadoEngine::SpriteHandle>,
			static_cast<std::size_t>(MapMiniMapObjectType::Count)> objectMarkers_;
		bool hasMapData_ = false;
		bool hasLayout_ = false;
		bool isVisible_ = false;
	};
}
