#include "BaseWeapon.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <cmath>

namespace Weapon {
	namespace {
		/// @brief 指定した強化ステータスの変更可能な設定を取得します。
		/// @param status 参照する武器ステータスです。
		/// @param type 取得する強化ステータスです。
		/// @return 設定が存在する場合はポインターを、存在しない場合はnullptrを返します。
		UpgradeValue* FindMutableUpgradeValue(UpgradeStatus& status, UpgradeStatType type) {
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

		// 武器のステータスをJsonから読み込みます。
		nlohmann::json json;
		if (!MadoEngine::Json::JsonFile::Load(jsonPath, json)) {
			Logger::Output("[Assets] 武器ステータスの読み込みに失敗しました: " + jsonPath, Logger::Level::Error);
			return false;
		}

		const nlohmann::json* statusJson = &json;
		if (json.is_object() && json.contains("upgradeStatus")) {
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

		upgradeLevel_ = 0;
		killCount_ = 0;
		projectileCount_ = 0;
		shotNowCount_ = 0;

		intervalTimer_.Start(status_.shotIntervalTime.value, true);
		cooldownTimer_.Start(status_.shotCooldown.value, false);
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
		
		CreateProjectile(deltaTime, ownerPosition, targetPosition);

	}

	void BaseWeapon::CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {

		// クールタイムが終了している場合射撃を開始する
		if (cooldownTimer_.IsFinished()) {
			if (!intervalTimer_.IsActive()) {
				intervalTimer_.Start(status_.shotIntervalTime.value, true);
				cooldownTimer_.Reset();
			}
		}

		// 最大数射撃数に達していない場合、射撃間隔が終了したら射撃を行う
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

			Projectile::Manager::GetInstance().AddProjectile(type_, context);

			// 最大射撃数に達した場合、射撃数をリセットしてクールダウンを開始する
			if (shotNowCount_ >= static_cast<int>(status_.shotMaxCount.value)) {
				shotNowCount_ = 0;
				intervalTimer_.Reset();
				cooldownTimer_.Start(status_.shotCooldown.value, false);
			}
		}

		intervalTimer_.Update(deltaTime);
		cooldownTimer_.Update(deltaTime);
	}
}
