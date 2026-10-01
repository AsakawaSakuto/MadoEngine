#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace Enemy::StatusEffect {

	/// @brief 状態異常の種類
	enum class Type : std::uint8_t {
		Burn,
		Poison,
		Frozen,

		Count,
	};

	inline constexpr std::size_t kTypeCount = static_cast<std::size_t>(Type::Count);

	/// @brief 状態異常の効果内容
	struct Definition {
		float duration = 0.0f;            // 状態異常の持続時間（秒）
		float damagePerTick = 0.0f;       // 状態異常の継続ダメージ量（1Tickあたり）
		float moveSpeedMultiplier = 1.0f; // 状態異常による移動速度倍率（0.0f～1.0f）
	};

	/// @brief 状態異常の適用要求
	struct ApplyRequest {
		Type type = Type::Burn;           // 適用する状態異常の種類
		Definition definition;            // 適用する状態異常の効果内容
		std::uint64_t sourceWeaponId = 0; // 適用元の武器識別番号（継続ダメージ発生時に通知するため）
	};

	/// @brief 状態異常による継続ダメージ情報
	struct DamageEvent {
		Type type = Type::Burn;           // 発生した状態異常の種類
		float damage = 0.0f;              // 発生した継続ダメージ量
		std::uint64_t sourceWeaponId = 0; // 発生元の武器識別番号
	};

	/// @brief 状態異常更新で発生したイベント群
	struct UpdateResult {
		std::array<DamageEvent, kTypeCount> damageEvents{}; // 発生した継続ダメージ情報の配列
		std::size_t damageEventCount = 0;                   // 発生した継続ダメージ情報の件数
	};

	/// @brief 複数種類の状態異常と経過時間を管理するクラス
	class Controller {
	public:
		/// @brief 管理中の状態異常を初期化
		void Clear();

		/// @brief 状態異常を適用
		/// @param request 状態異常の適用内容
		/// @return 状態異常を適用できた場合はtrue
		bool Apply(const ApplyRequest& request);

		/// @brief 状態異常の経過時間と周期処理を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @return 更新中に発生した継続ダメージ情報
		UpdateResult Update(float deltaTime);

		/// @brief 有効な状態異常を合成した移動速度倍率を取得
		/// @return 状態異常を反映した移動速度倍率
		float GetMoveSpeedMultiplier() const;

		/// @brief 指定した状態異常の有効状態を取得
		/// @param type 確認対象の状態異常
		/// @return 状態異常が有効な場合はtrue
		bool IsActive(Type type) const;

		/// @brief 指定した状態異常の残り時間を取得
		/// @param type 確認対象の状態異常
		/// @return 状態異常の残り時間、無効な種類の場合は0
		float GetRemainingTime(Type type) const;

	private:
		/// @brief 状態異常単体の実行時情報
		struct ActiveState {
			Definition definition;
			float remainingTime = 0.0f;
			float tickAccumulator = 0.0f;
			std::uint64_t sourceWeaponId = 0;
			bool isActive = false;
		};

		/// @brief 状態異常の列挙値を配列Indexへ変換
		/// @param type 変換対象の状態異常
		/// @return 状態異常に対応する配列Index
		static std::size_t ToIndex(Type type);

		/// @brief 状態異常の種類と効果内容を検証
		/// @param type 検証対象の状態異常
		/// @param definition 検証対象の効果内容
		/// @return 適用可能な内容の場合はtrue
		static bool IsValidDefinition(Type type, const Definition& definition);

		/// @brief 状態異常の種類に不要な値を既定値へ正規化
		/// @param type 正規化対象の状態異常
		/// @param definition 正規化対象の効果内容
		/// @return 種類別に正規化した効果内容
		static Definition NormalizeDefinition(Type type, const Definition& definition);

		/// @brief 新しい効果が現在の効果より強いか判定
		/// @param type 比較対象の状態異常
		/// @param currentDefinition 現在適用中の効果内容
		/// @param newDefinition 新しく適用する効果内容
		/// @return 新しい効果が強い場合はtrue
		static bool IsStronger(
			Type type,
			const Definition& currentDefinition,
			const Definition& newDefinition);

		/// @brief 状態異常の種類に対応する固定ダメージ周期を取得
		/// @param type 取得対象の状態異常
		/// @return 継続ダメージの周期、非ダメージ型の場合は0
		static float GetDamageTickInterval(Type type);

		std::array<ActiveState, kTypeCount> states_{};
	};
} // namespace Enemy::StatusEffect
