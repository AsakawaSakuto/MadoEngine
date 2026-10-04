#include "BaseWeapon.h"
#include "WeaponStatusJson.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Weapon {
	namespace {
		std::uint64_t nextWeaponId = 1;
		constexpr float kProjectileSpawnHeightOffset = 0.35f;

		/// @brief Weaponインスタンスの識別番号を発行
		/// @return 新しく発行した識別番号
		std::uint64_t IssueWeaponId() {
			return nextWeaponId++;
		}

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

		/// @brief 上限付き割合として扱う状態異常ステータスか確認
		/// @param type 確認する強化ステータス
		/// @return 0から100の割合として扱う場合はtrue
		bool IsStatusEffectPercentage(UpgradeStatType type) {
			return type == UpgradeStatType::BurnApplyChance ||
				type == UpgradeStatType::PoisonApplyChance ||
				type == UpgradeStatType::FrozenApplyChance ||
				type == UpgradeStatType::FrozenSlowRate;
		}

		/// @brief 正の値だけを許可する状態異常ステータスか確認
		/// @param type 確認する強化ステータス
		/// @return ダメージまたは持続時間の場合はtrue
		bool IsPositiveStatusEffectValue(UpgradeStatType type) {
			return type == UpgradeStatType::BurnDamage ||
				type == UpgradeStatType::BurnDuration ||
				type == UpgradeStatType::PoisonDamage ||
				type == UpgradeStatType::PoisonDuration ||
				type == UpgradeStatType::FrozenDuration;
		}

		/// @brief 状態異常の継続ダメージへ外部ダメージ倍率を適用
		/// @param statusEffects 倍率を適用する状態異常設定
		/// @param damageMultiplier 適用するダメージ倍率
		void ApplyDamageMultiplier(StatusEffectStatus& statusEffects, float damageMultiplier) {
			if (statusEffects.burn) {
				statusEffects.burn->damagePerTick *= damageMultiplier;
			}
			if (statusEffects.poison) {
				statusEffects.poison->damagePerTick *= damageMultiplier;
			}
		}
	}

	BaseWeapon::~BaseWeapon() {
		if (Projectile::IsPersistentWeaponType(type_)) {
			Projectile::Manager::GetInstance().RemoveProjectilesBySourceWeaponId(weaponId_);
		}
	}

	bool BaseWeapon::Initialize(Projectile::Type type, int slotIndex) {
		if (!Projectile::IsPlayableWeaponType(type) || slotIndex < 0) {
			Logger::Output("[Application] 武器の初期化引数が不正です。", Logger::Level::Error);
			return false;
		}

		UpgradeStatus loadedStatus{};
		StatusEffectUpgradeStatus loadedStatusEffects;
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
		if (json.is_object() && json.contains("statusEffects") &&
			!StatusEffectUpgradeStatusFromJson(json.at("statusEffects"), loadedStatusEffects)) {
			Logger::Output("[Assets] 状態異常ステータスに不正な値があります: " + jsonPath, Logger::Level::Error);
			return false;
		}

		slotIndex_ = slotIndex;
		type_ = type;
		weaponId_ = IssueWeaponId();
		status_ = loadedStatus;
		statusEffectUpgrades_ = loadedStatusEffects;
		weaponName_ = weaponName;

		upgradeLevel_ = 1;
		killCount_ = 0;
		damageCount_ = 0.0f;
		projectileCount_ = 0;
		shotNowCount_ = 0;
		wasFiredThisFrame_ = false;

		// 初弾を射撃可能にしつつ連射間隔Timerだけを待機状態で開始
		intervalTimer_.Start(status_.shotIntervalTime.value, true);
		cooldownTimer_.Reset();
		return true;
	}

	void BaseWeapon::RecordDamage(float appliedDamage, bool wasKilled) {

		// 不正値による累計値の破損とダメージを伴わない撃破加算を防止
		if (!std::isfinite(appliedDamage) || appliedDamage <= 0.0f) {
			return;
		}

		damageCount_ += appliedDamage;
		if (wasKilled) {
			++killCount_;
		}
	}

	const UpgradeValue* BaseWeapon::GetUpgradeValue(UpgradeStatType statType) const {
		if (const UpgradeValue* value = FindUpgradeValue(status_, statType)) {
			return value;
		}

		return FindUpgradeValue(statusEffectUpgrades_, statType);
	}

	std::vector<UpgradeStatType> BaseWeapon::GetSelectableUpgradeStatTypes() const {
		std::vector<UpgradeStatType> selectableTypes;
		selectableTypes.reserve(kUpgradeStatTypes.size());

		for (const UpgradeStatType statType : kUpgradeStatTypes) {
			const UpgradeValue* value = GetUpgradeValue(statType);
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

		const UpgradeValue* value = GetUpgradeValue(statType);
		if (!value || !value->isSelected || !std::isfinite(value->value) ||
			!std::isfinite(value->fixedAddValue) || !std::isfinite(value->rarityAddValue)) {
			return false;
		}

		const float rarityValue = static_cast<float>(static_cast<int>(rarity));
		const float configuredAmount = value->fixedAddValue + value->rarityAddValue * rarityValue;
		float amount = configuredAmount;

		// Cooldownの設定値だけは秒数ではなく現在時間に対する短縮率として扱う
		if (statType == UpgradeStatType::ShotCooldown) {
			constexpr float kPercentageScale = 0.01f;
			if (value->value <= 0.0f || configuredAmount <= 0.0f || configuredAmount >= 100.0f) {
				return false;
			}

			amount = -value->value * configuredAmount * kPercentageScale;
		} else if (IsStatusEffectPercentage(statType)) {

			// 付与率と減速率は100%を上限として上限直前の強化量だけを切り詰め
			if (value->value < 0.0f || value->value >= 100.0f || configuredAmount <= 0.0f) {
				return false;
			}
			amount = std::min(configuredAmount, 100.0f - value->value);
		} else if (IsPositiveStatusEffectValue(statType)) {

			// ダメージと持続時間が強化によって無効値へ遷移しないよう正の加算だけを受付
			if (value->value <= 0.0f || configuredAmount <= 0.0f) {
				return false;
			}
		}

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
		UpgradeValue* value = FindUpgradeValue(status_, statType);
		if (!value) {
			value = FindUpgradeValue(statusEffectUpgrades_, statType);
		}
		if (!value) {
			return false;
		}

		value->value += amount;
		++upgradeLevel_;
		outAppliedAmount = amount;
		return true;
	}

	void BaseWeapon::Update(
		float deltaTime,
		const Vector3& ownerPosition,
		const Vector3& targetPosition,
		const RuntimeModifiers& modifiers) {
		wasFiredThisFrame_ = false;
		if (Projectile::IsPersistentWeaponType(type_)) {
			return;
		}

		CreateProjectile(deltaTime, ownerPosition, targetPosition, modifiers);
	}

	bool BaseWeapon::SynchronizePersistentProjectile(
		const Vector3& ownerPosition,
		const RuntimeModifiers& modifiers) {
		if (!Projectile::IsPersistentWeaponType(type_)) {
			return false;
		}

		Projectile::InitializeDesc context = CreateProjectileInitializeDesc(ownerPosition, ownerPosition, modifiers);
		const bool wasCreated = Projectile::Manager::GetInstance().SynchronizePersistentProjectile(type_, context);
		if (wasCreated) {
			++projectileCount_;
		}
		return wasCreated;
	}

	void BaseWeapon::CreateProjectile(
		float deltaTime,
		const Vector3& ownerPosition,
		const Vector3& targetPosition,
		const RuntimeModifiers& modifiers) {

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

			Projectile::InitializeDesc context = CreateProjectileInitializeDesc(ownerPosition, targetPosition, modifiers);

			Projectile::Manager::GetInstance().AddProjectile(type_, context);
			wasFiredThisFrame_ = true;

			// 最大射撃数へ到達したBurstを閉じてCooldownへ遷移
			const int shotMaxCount = std::max(
				1,
				static_cast<int>(status_.shotMaxCount.value) + modifiers.additionalShotMaxCount);
			if (shotNowCount_ >= shotMaxCount) {
				shotNowCount_ = 0;
				intervalTimer_.Reset();
				const float modifiedCooldown =
					(status_.shotCooldown.value + modifiers.shotCooldownFlat) *
					(1.0f + modifiers.shotCooldownRate);
				const float cooldown = std::isfinite(modifiedCooldown)
					? std::max(modifiedCooldown, 0.0f)
					: std::max(status_.shotCooldown.value, 0.0f);
				cooldownTimer_.Start(cooldown, false);
			}
		}

		// 判定後にTimerを進めて完了イベントを次フレームの射撃へ反映
		intervalTimer_.Update(deltaTime);
		cooldownTimer_.Update(deltaTime);
	}

	Projectile::InitializeDesc BaseWeapon::CreateProjectileInitializeDesc(
		const Vector3& ownerPosition,
		const Vector3& targetPosition,
		const RuntimeModifiers& modifiers) const {
		Projectile::InitializeDesc context;
		context.sourceWeaponId = weaponId_;
		context.projectileName = weaponName_;
		context.projectileCount = projectileCount_;
		context.ownerPosition = ownerPosition;
		if (type_ != Projectile::Type::Eye && type_ != Projectile::Type::ToxicBoots) {

			// 投射武器がPlayer中心より少し上から飛び始めるよう生成位置だけを補正
			context.ownerPosition.y += kProjectileSpawnHeightOffset;
		}
		context.targetPosition = targetPosition;
		const float damageMultiplier = std::isfinite(modifiers.damageMultiplier)
			? std::max(modifiers.damageMultiplier, 0.0f)
			: 1.0f;
		context.damage = status_.damage.value * damageMultiplier;
		const float modifiedCriticalChance =
			(status_.criticalChance.value + modifiers.criticalChanceFlat) *
			(1.0f + modifiers.criticalChanceRate);
		context.criticalChance = std::isfinite(modifiedCriticalChance)
			? std::clamp(modifiedCriticalChance, 0.0f, 100.0f)
			: std::clamp(status_.criticalChance.value, 0.0f, 100.0f);
		context.criticalDamage = status_.criticalDamage.value;
		context.knockbackPower = status_.knockbackPower.value;
		context.moveSpeed = status_.speed.value;
		context.sizeRate = status_.size.value;
		context.lifeTime = status_.lifeTime.value;
		context.bounceCount = ConvertToProjectileCount(status_.bounceCount.value);
		context.penetrationCount = ConvertToProjectileCount(status_.penetrationCount.value);
		context.statusEffects = ResolveStatusEffectStatus(statusEffectUpgrades_);
		ApplyDamageMultiplier(context.statusEffects, damageMultiplier);
		return context;
	}
}
