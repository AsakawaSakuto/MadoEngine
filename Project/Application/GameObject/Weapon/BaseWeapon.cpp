#include "BaseWeapon.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Weapon {
	namespace {
		/// @brief 浮動小数点の強化値をProjectile用の回数へ変換
		/// @param value 変換する強化値
		/// @return 0以上の整数回数
		int ConvertToProjectileCount(float value) {
			if (!std::isfinite(value) || value <= 0.0f) {
				return 0;
			}

			const double clampedValue = std::min(
				static_cast<double>(value),
				static_cast<double>(std::numeric_limits<int>::max()));
			return static_cast<int>(clampedValue);
		}

		/// @brief 指定した強化ステータスの変更可能な設定を取得
		/// @param status 参照する武器ステータス
		/// @param type 取得する強化ステータス
		/// @return 設定が存在する場合はポインターを、存在しない場合はnullptr
		UpgradeValue* FindMutableUpgradeValue(UpgradeStatus& status, UpgradeStatType type) {

			// 選択TypeをUpgradeStatus内の唯一の更新先へ対応付け
			switch (type) {
			case UpgradeStatType::Damage:           return &status.damage;
			case UpgradeStatType::ShotMaxCount:     return &status.shotMaxCount;
			case UpgradeStatType::ShotIntervalTime: return &status.shotIntervalTime;
			case UpgradeStatType::ShotCooldown:     return &status.shotCooldown;
			case UpgradeStatType::CriticalChance:   return &status.criticalChance;
			case UpgradeStatType::CriticalDamage:   return &status.criticalDamage;
			case UpgradeStatType::Size:             return &status.size;
			case UpgradeStatType::BounceCount:      return &status.bounceCount;
			case UpgradeStatType::PenetrationCount: return &status.penetrationCount;
			case UpgradeStatType::KnockbackPower:   return &status.knockbackPower;
			case UpgradeStatType::LifeTime:         return &status.lifeTime;
			case UpgradeStatType::Speed:            return &status.speed;
			default:                                return nullptr;
			}
		}
	}

	bool BaseWeapon::Initialize(Projectile::Type type, int slotIndex) {
		if (!Projectile::IsPlayableWeaponType(type) || slotIndex < 0) {
			Logger::Output("[Application] 武器の初期化引数が不正です。", Logger::Level::Error);
			return false;
		}

		UpgradeStatus loadedStatus{};
		const std::string weaponName = ProjectileTypeToString(type);
		const std::string jsonPath = "Assets/Json/Weapon/" + Projectile::ProjectileTypeToJsonFileName(type) + ".json";

		// 実行時の武器性能を外部調整可能なJsonから復元
		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(jsonPath, json)) {
			Logger::Output("[Assets] 武器ステータスの読み込みに失敗しました: " + jsonPath, Logger::Level::Error);
			return false;
		}

		const nlohmann::json* statusJson = &json;
		if (json.is_object() && json.contains("upgradeStatus")) {

			// Editorの保存形式とステータス単体形式の両方を受け入れ
			statusJson = &json.at("upgradeStatus");
		}

		if (!UpgradeStatusFromJson(*statusJson, loadedStatus)) {
			Logger::Output("[Assets] 武器ステータスに不正な値があります: " + jsonPath, Logger::Level::Error);
			return false;
		}

		slotIndex_ = slotIndex;
		type_ = type;
		status_ = loadedStatus;
		weaponName_ = weaponName;

		upgradeLevel_ = 1;
		killCount_ = 0;
		projectileCount_ = 0;
		shotNowCount_ = 0;
		wasFiredThisFrame_ = false;

		// 初弾を射撃可能にしつつ連射間隔Timerだけを待機状態で開始
		intervalTimer_.Start(status_.shotIntervalTime.value, true);
		cooldownTimer_.Reset();
		return true;
	}

	std::vector<UpgradeStatType> BaseWeapon::GetSelectableUpgradeStatTypes() const {
		std::vector<UpgradeStatType> selectableTypes;
		selectableTypes.reserve(kUpgradeStatTypes.size());

		for (const UpgradeStatType statType : kUpgradeStatTypes) {
			const UpgradeValue* value = FindUpgradeValue(status_, statType);
			if (!value || !value->isSelected || !std::isfinite(value->value) ||
				!std::isfinite(value->fixedAddValue) || !std::isfinite(value->rarityAddValue)) {
				continue;
			}

			// 候補表示後の計算失敗を防ぐため全レアリティの加算値を事前検証
			bool canUseForAllRarities = true;
			for (int rarityValue = static_cast<int>(Rarity::Uncommon);
				rarityValue <= static_cast<int>(Rarity::Legendary); ++rarityValue) {
				float amount = 0.0f;
				if (!CalculateUpgradeAmount(statType, static_cast<Rarity>(rarityValue), amount)) {
					canUseForAllRarities = false;
					break;
				}
			}

			if (canUseForAllRarities) {
				selectableTypes.push_back(statType);
			}
		}

		return selectableTypes;
	}

	bool BaseWeapon::CalculateUpgradeAmount(UpgradeStatType statType, Rarity rarity, float& outAmount) const {
		outAmount = 0.0f;

		// 武器強化用Rarityと有限な設定値だけを計算対象として受付
		if (!IsWeaponUpgradeRarity(rarity)) {
			return false;
		}

		const UpgradeValue* value = FindUpgradeValue(status_, statType);
		if (!value || !value->isSelected || !std::isfinite(value->value) ||
			!std::isfinite(value->fixedAddValue) || !std::isfinite(value->rarityAddValue)) {
			return false;
		}

		const float rarityValue = static_cast<float>(static_cast<int>(rarity));
		const float amount = value->fixedAddValue + value->rarityAddValue * rarityValue;
		if (!std::isfinite(amount) || !std::isfinite(value->value + amount)) {
			return false;
		}

		outAmount = amount;
		return true;
	}

	bool BaseWeapon::ApplyUpgrade(UpgradeStatType statType, Rarity rarity, float expectedAmount, float& outAppliedAmount) {
		outAppliedAmount = 0.0f;
		float amount = 0.0f;
		if (!CalculateUpgradeAmount(statType, rarity, amount) ||
			!std::isfinite(expectedAmount) || amount != expectedAmount) {
			return false;
		}

		// 候補生成時と適用時の値が一致する場合だけステータスを更新
		UpgradeValue* value = FindMutableUpgradeValue(status_, statType);
		if (!value) {
			return false;
		}

		value->value += amount;
		++upgradeLevel_;
		outAppliedAmount = amount;
		return true;
	}

	void BaseWeapon::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		wasFiredThisFrame_ = false;
		CreateProjectile(deltaTime, ownerPosition, targetPosition);
	}

	void BaseWeapon::CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {

		// Burst間のCooldown終了後に次の射撃間隔Timerを再開
		if (cooldownTimer_.IsFinished()) {
			if (!intervalTimer_.IsActive()) {
				intervalTimer_.Start(status_.shotIntervalTime.value, true);
				cooldownTimer_.Reset();
			}
		}

		// 射撃間隔ごとに現在の強化値を反映したProjectileを生成
		if (intervalTimer_.IsFinished()) {
			shotNowCount_++;
			projectileCount_++;

			Projectile::InitializeDesc context;
			context.projectileName = weaponName_;
			context.projectileCount = projectileCount_;
			context.ownerPosition = ownerPosition;
			context.targetPosition = targetPosition;
			context.damage = status_.damage.value;
			context.moveSpeed = status_.speed.value;
			context.sizeRate = status_.size.value;
			context.lifeTime = status_.lifeTime.value;
			context.bounceCount = ConvertToProjectileCount(status_.bounceCount.value);
			context.penetrationCount = ConvertToProjectileCount(status_.penetrationCount.value);

			Projectile::Manager::GetInstance().AddProjectile(type_, context);
			wasFiredThisFrame_ = true;

			// 最大射撃数へ到達したBurstを閉じてCooldownへ遷移
			if (shotNowCount_ >= static_cast<int>(status_.shotMaxCount.value)) {
				shotNowCount_ = 0;
				intervalTimer_.Reset();
				cooldownTimer_.Start(status_.shotCooldown.value, false);
			}
		}

		// 判定後にTimerを進めて完了イベントを次フレームの射撃へ反映
		intervalTimer_.Update(deltaTime);
		cooldownTimer_.Update(deltaTime);
	}
}
