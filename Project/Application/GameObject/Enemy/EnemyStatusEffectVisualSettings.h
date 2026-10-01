#pragma once
#include "EnemyStatusEffect.h"
#include "Math/Vector4.h"
#include <array>

namespace Enemy::StatusEffect {

	/// @brief 状態異常の表示設定
	struct VisualStyle {
		Vector4 enemyColorMultiplier = { 1.0f, 1.0f, 1.0f, 1.0f }; // Enemy表示色へ乗算する係数
		Vector4 damageTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };       // 継続ダメージ文字色
	};

	/// @brief 状態異常ごとの表示設定を一元管理するクラス
	class VisualSettings final {
	public:
		/// @brief 状態異常表示設定の共有インスタンスを取得
		/// @return 状態異常表示設定の共有インスタンス
		static VisualSettings& GetInstance();

		/// @brief 指定した状態異常の表示設定を取得
		/// @param type 取得対象の状態異常
		/// @return 指定した状態異常の表示設定
		const VisualStyle& Get(Type type) const;

		/// @brief 指定した状態異常の表示設定を編集用に取得
		/// @param type 編集対象の状態異常
		/// @return 指定した状態異常の表示設定
		VisualStyle& Edit(Type type);

	private:
		/// @brief 状態異常表示設定を既定値で初期化
		VisualSettings();

		std::array<VisualStyle, kTypeCount> styles_;
		VisualStyle fallbackStyle_;
	};

} // namespace Enemy::StatusEffect
