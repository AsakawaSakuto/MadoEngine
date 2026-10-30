#pragma once
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "System/PlayLog/PlayLogData.h"
#include <string>
#include <vector>

namespace UI::Result {

	/// @brief 終了時の装備武器と状態異常のアイコンと戦績の表示
	class WeaponStatsUI final {
	public:
		/// @brief 終了時の装備と戦績のある状態異常の表示を初期化
		/// @param result 装備履歴と毒と火傷の戦績を含む確定済みプレイ結果
		void Initialize(const System::PlayLogData& result);

		/// @brief 見出しとアイコンと戦績と操作案内の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief Scene単位で管理する描画リソースの参照を解放
		void Finalize();

	private:
		/// @brief 攻撃元のアイコンと同じ行へ名前と撃破数と累計ダメージを配置
		/// @param prefix 描画オブジェクトの識別名の接頭辞
		/// @param textureName 攻撃元のアイコンのテクスチャ名
		/// @param displayName 攻撃元の表示名
		/// @param combat 攻撃元の撃破数と累計ダメージ
		/// @param rowIndex 表示行の番号
		void CreateCombatRow(const std::string& prefix, const std::string& textureName,
			const std::string& displayName, const System::PlayCombatStats& combat, std::size_t rowIndex);

		/// @brief 戦績表示用の文字列をUI描画レイヤーへ生成
		/// @param name 描画オブジェクトの識別名
		/// @param value 表示する文字列
		/// @param position 表示領域の左上座標
		/// @param areaSize 表示領域の大きさ
		/// @param fontSize 文字の大きさ
		/// @param align 表示領域内の水平方向の配置
		void CreateText(const std::string& name, const std::string& value,
			const Vector2& position, const Vector2& areaSize, float fontSize,
			MadoEngine::TextHorizontalAlign align = MadoEngine::TextHorizontalAlign::Left);

		std::vector<MadoEngine::SpriteHandle> icons_;
		std::vector<MadoEngine::TextHandle> texts_;
		float rowSpacing_ = 70.0f;
	};

}
