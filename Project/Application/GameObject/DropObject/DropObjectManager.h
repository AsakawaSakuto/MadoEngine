#pragma once
#include "DropObject.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Player {
	class Base;
}

namespace DropObject {
	
	class Manager {
	public:

		/// @brief DropObjectの管理Instanceを取得
		/// @return DropObjectの管理Instance
		static Manager& GetInstance();

		/// @brief Exp用512枠、Money用128枠、特殊4種類共用16枠のModelとColliderを事前確保
		void Initialize();

		/// @brief DropObjectを更新
		/// @param deltaTime 1フレームの経過時間
		/// @param player 報酬を受け取るPlayer
		void Update(float deltaTime, Player::Base& player);

		/// @brief DropObjectを生成
		/// @param type DropObjectの種類
		/// @param position 生成位置
		void Spawn(Type type, const Vector3& position);

		/// @brief 発生元の確率で特殊4種類から等確率の一個を生成
		/// @param source 出現確率を決定する発生元、Jarは5%、EliteEnemyは25%
		/// @param position 生成位置のワールド座標
		/// @return 抽選に当選し、特殊Dropの空き枠へ生成できた場合はtrue
		bool TrySpawnSpecial(SpecialDropSource source, const Vector3& position);

		/// @brief 配置中のExpすべてを指定座標へ吸着開始
		/// @param targetPosition Playerの現在ワールド座標
		void AttractAllExp(const Vector3& targetPosition);

		/// @brief 各Poolの確保枠を保持して全Dropと未回収報酬をリセット
		void Clear();

		/// @brief 各Poolの確保枠を含む全DropObjectのリソースを解放
		void Finalize();

	private:
		static constexpr std::size_t kExpPoolCapacity = 512;
		static constexpr std::size_t kMoneyPoolCapacity = 128;
		static constexpr std::size_t kSpecialPoolCapacity = 16;
		static constexpr float kExpCollectionSoundInterval = 0.05f;

		struct PoolEntry {
			std::unique_ptr<Base> object;
			std::uint64_t spawnOrder = 0;
		};

		/// @brief Dropの種類ごとに独立した固定容量と上書き報酬の管理
		template<std::size_t Capacity>
		struct Pool {
			std::array<PoolEntry, Capacity> entries;
			Type type = Type::Exp;
			std::size_t nextIndex = 0;
			std::uint64_t spawnOrder = 0;
			std::uint64_t pendingAmount = 0;
		};

		/// @brief 単独利用時に各Poolを初回のみ事前確保
		void EnsureInitialized();

		/// @brief 指定したPoolのModelとColliderを事前確保
		/// @param pool 事前確保するPool
		/// @param type Poolで管理するDropの種類
		/// @param firstIndex ModelとColliderの識別番号の開始値
		template<std::size_t Capacity>
		void InitializePool(Pool<Capacity>& pool, Type type, int firstIndex);

		/// @brief 使用中のPool枠を更新して同種の蓄積報酬を回収分へ加算し、回収成立を通知
		/// @param pool 更新するPool
		/// @param deltaTime 1フレームの経過時間
		/// @param player 報酬を受け取るPlayer
		/// @return 一個以上の報酬回収が成立した場合はtrue
		template<std::size_t Capacity>
		bool UpdatePool(Pool<Capacity>& pool, float deltaTime, Player::Base& player);

		/// @brief 経験値進行度に応じた五段階の取得SEを間隔制限付きで再生
		/// @param player 経験値加算後の状態を参照するPlayer
		void PlayExpCollectionSound(const Player::Base& player);

		/// @brief 指定したPoolの空き枠または最も古い枠へ配置
		/// @param pool 配置先のPool
		/// @param position 配置先のワールド座標
		template<std::size_t Capacity>
		void SpawnPooled(Pool<Capacity>& pool, const Vector3& position);

		/// @brief 特殊4種類の共用枠へ配置し、満杯時は追加生成を中止
		/// @param type 配置する特殊Dropの種類
		/// @param position 配置先のワールド座標
		/// @return 空き枠へ配置できた場合はtrue
		bool SpawnSpecial(Type type, const Vector3& position);

		/// @brief Poolの確保枠を保持して配置と蓄積報酬をリセット
		/// @param pool リセットするPool
		template<std::size_t Capacity>
		void ClearPool(Pool<Capacity>& pool);

		/// @brief 同種の上書き報酬を回収分へ合算してPlayerへ加算
		/// @param type 回収したDropの種類
		/// @param player 報酬の受取先Player
		/// @param collectedAmount 今回回収した報酬量
		/// @param pendingAmount 同種の上書きで蓄積した未回収報酬量
		void GrantCollectedReward(Type type, Player::Base& player, int collectedAmount, std::uint64_t& pendingAmount);

		Pool<kExpPoolCapacity> expPool_;
		Pool<kMoneyPoolCapacity> moneyPool_;
		std::array<std::unique_ptr<Base>, kSpecialPoolCapacity> specialDropObjects_;
		float expCollectionSoundCooldown_ = 0.0f;
	};
}
