#pragma once
#include "GameObject/Enemy/EnemyManager.h"
#include "Math/Vector2.h"
#include "Render/Object/ObjectHandle.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

class Camera;

namespace UI::Game {
	/// @brief Enemyの頭上へ有効な状態異常アイコンを追従表示するビュー
	class EnemyStatusEffectIconView {
	public:
		/// @brief 状態異常アイコン表示を初期化
		void Initialize();

		/// @brief Enemyの状態異常と頭上座標へアイコン表示を同期
		/// @param enemyManager 表示対象Enemyの管理クラス
		/// @param camera ワールド座標の投影に使用するカメラ
		void Update(const Enemy::Manager& enemyManager, const Camera& camera);

		/// @brief 状態異常アイコンの表示可否を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 状態異常アイコン表示を終了
		void Finalize();

	private:
		/// @brief 状態異常アイコン単体の実行時情報
		struct IconEntry {
			MadoEngine::SpriteHandle sprite{};
			bool ownerExists = false;
		};

		/// @brief Enemyと状態異常の組み合わせを一意なキーへ変換
		/// @param enemyId Enemyの識別番号
		/// @param type 状態異常の種類
		/// @return 状態異常アイコンの一意キー
		static std::uint64_t CreateIconKey(
			std::uint32_t enemyId,
			Enemy::StatusEffect::Type type);

		/// @brief 状態異常に対応するテクスチャ名を取得
		/// @param type 状態異常の種類
		/// @return 状態異常アイコンのテクスチャ名
		static const char* GetTextureName(Enemy::StatusEffect::Type type);

		/// @brief 状態異常アイコン用Spriteを生成
		/// @param enemyId Enemyの識別番号
		/// @param type 状態異常の種類
		/// @return 生成したSpriteのHandle、生成に失敗した場合は無効Handle
		MadoEngine::SpriteHandle CreateIconSprite(
			std::uint32_t enemyId,
			Enemy::StatusEffect::Type type) const;

		/// @brief ワールド座標を基準画面上の座標へ変換
		/// @param worldPosition 変換するワールド座標
		/// @param camera 投影に使用するカメラ
		/// @param outScreenPosition 変換後の画面座標
		/// @return 画面内へ投影できた場合はtrue
		bool WorldToScreen(
			const Vector3& worldPosition,
			const Camera& camera,
			Vector2& outScreenPosition) const;

		/// @brief 管理中の全アイコンを非表示
		void HideAllIcons();

		std::unordered_map<std::uint64_t, IconEntry> icons_;
		std::vector<Enemy::StatusEffectDisplayData> displayData_;
		bool isVisible_ = true;
		float iconPixelSize_ = 24.0f;
		float iconSpacing_ = 28.0f;
		float headWorldOffset_ = 0.4f;
	};
} // namespace UI::Game
