#pragma once
#include "EnemyStatus.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Enemy {

	/// @brief Enemy種類ごとの初期能力値と外形設定
	struct TypeSettings {
		Data::Status status;
		float scaleMultiplier = 1.0f;
		float movementColliderRadius = 0.5f;
		Vector3 hitboxMin = { -0.5f, 0.0f, -0.5f };
		Vector3 hitboxMax = { 0.5f, 2.0f, 0.5f };
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	/// @brief Elite属性へ共通適用する設定
	struct EliteSettings {
		float healthMultiplier = 3.0f;
		float powerMultiplier = 2.0f;
		float bodyScaleMultiplier = 1.5f;
		float markerHeightOffset = 0.4f;
		Vector3 markerScale = { 0.45f, 0.45f, 0.45f };
	};

	/// @brief Wave内の一定間隔ごとに適用する能力値加算設定
	struct TimedStatIncreaseSettings {
		float interval = 60.0f;
		float amount = 0.0f;
	};

	/// @brief 通常WaveとBonus Waveで共有するEnemy生成設定
	struct WaveSpawnSettings {
		float spawnInterval = 0.4f;
		std::uint32_t spawnCount = 1;
		std::uint32_t eliteSpawnInterval = 50;
		float minSpawnRadius = 8.0f;
		float maxSpawnRadius = 14.0f;
		float normalSpawnRate = 0.6f;
		float runnerSpawnRate = 0.25f;
		float tankSpawnRate = 0.15f;
		TimedStatIncreaseSettings healthIncrease = { 60.0f, 1.0f };
		TimedStatIncreaseSettings powerIncrease = { 60.0f, 0.5f };
		TimedStatIncreaseSettings moveSpeedIncrease = { 60.0f, 0.06f };
	};

	/// @brief 指定時間帯に適用する通常Wave設定
	struct WaveSettings : public WaveSpawnSettings {
		std::string name = "Wave 1";
		float startTime = 0.0f;
		float endTime = 300.0f;
	};

	/// @brief EnemyStatusとWave設定の保持とJson永続化を管理するクラス
	class Settings final {
	public:
		/// @brief Enemy設定の共有Instanceを取得
		/// @return Enemy設定の共有Instance
		static Settings& GetInstance();

		Settings(const Settings&) = delete;
		Settings& operator=(const Settings&) = delete;
		Settings(Settings&&) = delete;
		Settings& operator=(Settings&&) = delete;

		/// @brief Jsonが存在すれば読み込み、存在しなければ既定値で作成
		/// @return 読み込みまたは作成に成功した場合はtrue
		bool LoadOrCreate();

		/// @brief Enemy設定をJsonから読み込み
		/// @return 読み込みに成功した場合はtrue
		bool Load();

		/// @brief Enemy設定をJsonへ保存
		/// @return 保存に成功した場合はtrue
		bool Save() const;

		/// @brief 全Enemy設定を既定値へ復元
		void ResetToDefaults();

		/// @brief 編集値を実行可能な範囲へ補正
		void Normalize();

		/// @brief 指定Enemy種類の設定を取得
		/// @param type 取得するEnemy種類
		/// @return 指定Enemy種類の設定
		const TypeSettings& GetTypeSettings(Data::Type type) const;

		/// @brief 指定Enemy種類の編集用設定を取得
		/// @param type 編集するEnemy種類
		/// @return 指定Enemy種類の編集用設定
		TypeSettings& EditTypeSettings(Data::Type type);

		/// @brief Elite属性設定を取得
		/// @return Elite属性設定
		const EliteSettings& GetEliteSettings() const { return eliteSettings_; }

		/// @brief Elite属性の編集用設定を取得
		/// @return Elite属性の編集用設定
		EliteSettings& EditEliteSettings() { return eliteSettings_; }

		/// @brief Wave設定一覧を取得
		/// @return Wave設定一覧
		const std::vector<WaveSettings>& GetWaves() const { return waves_; }

		/// @brief Waveの編集用設定一覧を取得
		/// @return Waveの編集用設定一覧
		std::vector<WaveSettings>& EditWaves() { return waves_; }

		/// @brief 制限時間後に適用するBonus Wave設定を取得
		/// @return Bonus Wave設定
		const WaveSpawnSettings& GetBonusWaveSettings() const { return bonusWaveSettings_; }

		/// @brief 制限時間後に適用するBonus Waveの編集用設定を取得
		/// @return Bonus Waveの編集用設定
		WaveSpawnSettings& EditBonusWaveSettings() { return bonusWaveSettings_; }

		/// @brief 全Wave共通のEnemy最大生存数を取得
		/// @return 全Wave共通のEnemy最大生存数
		std::size_t GetMaxAliveEnemies() const { return maxAliveEnemies_; }

		/// @brief 全Wave共通のEnemy最大生存数を設定
		/// @param maxAliveEnemies 全Wave共通のEnemy最大生存数
		void SetMaxAliveEnemies(std::size_t maxAliveEnemies) { maxAliveEnemies_ = maxAliveEnemies; }

		/// @brief 末尾Waveの設定を引き継いだ新規Waveを追加
		void AddWave();

		/// @brief 指定位置のWaveを削除
		/// @param index 削除するWaveの位置
		/// @return Waveを削除した場合はtrue
		bool RemoveWave(std::size_t index);

	private:
		/// @brief 既定値を保持したEnemy設定を構築
		Settings();

		/// @brief Enemy種類を設定配列の位置へ変換
		/// @param type 変換するEnemy種類
		/// @return 設定配列の位置
		static std::size_t ToTypeIndex(Data::Type type);

		static constexpr std::size_t kTypeCount = 4;
		std::array<TypeSettings, kTypeCount> typeSettings_;
		EliteSettings eliteSettings_;
		std::size_t maxAliveEnemies_ = 500;
		std::vector<WaveSettings> waves_;
		WaveSpawnSettings bonusWaveSettings_;
	};

	/// @brief Enemy種類を設定名へ変換
	/// @param type 変換するEnemy種類
	/// @return Enemy種類の設定名
	const char* EnemyTypeToString(Data::Type type);

} // namespace Enemy
