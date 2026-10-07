#pragma once
#include "GameObject/Rarity.h"
#include <RenderHeaders.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Relic {
	class System;
	struct Definition;
}

namespace UI::Game {

	/// @brief 宝箱から獲得する一個のレリックを表示するUI
	class RelicAcquisitionUI {
	public:
		/// @brief 描画オブジェクトを初期化
		void Initialize();

		/// @brief 描画オブジェクトの参照を解放
		void Finalize();

		/// @brief 獲得待ちレリックの表示と決定入力を更新
		/// @param relicSystem 更新対象のレリックシステム
		void Update(Relic::System& relicSystem);

		/// @brief レリック獲得UIの見た目を調整するImGuiを描画
		/// @param relicSystem プレビューに使用するレリックシステム
		void DrawImGui(Relic::System& relicSystem);

	private:
		struct AppearanceSettings {
			/// @brief 共通レアリティ色を使用して見た目設定を初期化
			AppearanceSettings();

			std::array<Vector2, 3> panelPositions = {
				Vector2{ 640.0f, 360.0f },
				Vector2{ 640.0f, 360.0f },
				Vector2{ 640.0f, 360.0f },
			};
			std::array<Vector2, 3> panelScales = {
				Vector2{ 80.0f, 45.0f },
				Vector2{ 16.0f, 24.0f },
				Vector2{ 15.0f, 23.0f },
			};
			Vector4 overlayColor = { 0.0f, 0.0f, 0.0f, 184.0f / 255.0f };
			Vector4 panelColor = { 11.0f / 255.0f, 15.0f / 255.0f, 26.0f / 255.0f, 250.0f / 255.0f };
			Vector2 iconPosition = { 640.0f, 360.0f };
			Vector2 iconScale = { 2.0f, 2.0f };
			std::array<Vector2, 3> textPositions = {
				Vector2{ 640.0f, 264.0f },
				Vector2{ 640.0f, 216.0f },
				Vector2{ 640.0f, 460.0f },
			};
			std::array<Vector2, 3> textAreaSizes = {
				Vector2{ 480.0f, 48.0f },
				Vector2{ 480.0f, 48.0f },
				Vector2{ 480.0f, 48.0f },
			};
			std::array<float, 3> fontSizes = { 24.0f, 32.0f, 24.0f };
			std::array<Vector4, 3> textColors = {
				Vector4{ 242.0f / 255.0f, 247.0f / 255.0f, 1.0f, 1.0f },
				Vector4{ 242.0f / 255.0f, 247.0f / 255.0f, 1.0f, 1.0f },
				Vector4{ 242.0f / 255.0f, 247.0f / 255.0f, 1.0f, 1.0f },
			};
			std::array<Vector4, kRarityCount> rarityColors{};
		};

		/// @brief 現在の見た目設定を全描画要素へ反映
		void ApplyAppearanceSettings();

		/// @brief レアリティに対応する表示色を取得
		/// @param rarity 色を取得するレアリティ
		/// @return レアリティ表示色
		const Vector4& GetRarityColor(Rarity rarity) const;

		/// @brief 表示内容をレリック定義へ同期
		/// @param definition 表示対象のレリック定義
		void SetDefinition(const Relic::Definition& definition);

		/// @brief 全構成要素の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		std::array<MadoEngine::SpriteHandle, 3> panelSprites_{};
		MadoEngine::SpriteHandle iconSprite_{};
		std::array<MadoEngine::TextHandle, 3> textHandles_{};
		AppearanceSettings appearanceSettings_;
		std::string displayedRelicId_;
		std::size_t previewRelicIndex_ = 0;
		std::uint64_t displayedRevision_ = 0;
		bool isPreviewEnabled_ = false;
		bool isVisible_ = false;
		bool isInitialized_ = false;
	};

}
