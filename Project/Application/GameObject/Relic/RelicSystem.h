#pragma once
#include "RelicCatalog.h"
#include "RelicInventory.h"
#include "Utility/Random.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace Relic {

	/// @brief レリック抽選と獲得による効果集約を管理するクラス
	class System {
	public:
		/// @brief レリックシステムを初期化
		/// @param gameSeed ゲーム共通シード値
		/// @param catalogPath レリック定義JSONのパス
		/// @return 初期化に成功した場合はtrue
		bool Initialize(std::uint32_t gameSeed, const std::string& catalogPath = "Assets/Json/Relic/Relics.json");

		/// @brief Editorのレリック定義を保存して実行時へ適用
		/// @param definitions 保存するレリック定義一覧
		/// @return 保存と適用に成功した場合はtrue
		bool SaveCatalog(const std::vector<Definition>& definitions);

		/// @brief JSONからレリック定義を再読み込み
		/// @return 再読み込みと適用に成功した場合はtrue
		bool ReloadCatalog();

		/// @brief 宝箱一個分のレリック報酬を抽選
		/// @return 獲得可能な報酬を抽選できた場合はtrue
		bool RequestReward();

		/// @brief 抽選可能なレリックが残っているか確認
		/// @return 未所持上限のレリックが存在し報酬待ちでない場合はtrue
		bool CanRequestReward() const;

		/// @brief 保留中のレリック報酬を所持品へ追加
		/// @return 報酬を追加できた場合はtrue
		bool ConfirmPendingReward();

		/// @brief Editor操作で指定レリックを一個獲得
		/// @param id 獲得対象のレリックID
		/// @return 所持上限未満で獲得できた場合はtrue
		bool AcquireForEditor(const Id& id);

		/// @brief Editor操作で指定レリックを一個破棄
		/// @param id 破棄対象のレリックID
		/// @return 一個以上所持していて破棄できた場合はtrue
		bool DiscardForEditor(const Id& id);

		/// @brief IDに対応する現在適用中のレリック定義を取得
		/// @param id 検索対象のレリックID
		/// @return 定義が存在する場合はconstポインター、存在しない場合はnullptr
		const Definition* FindDefinition(const Id& id) const { return catalog_.Find(id); }

		/// @brief 獲得待ちのレリックが存在するか確認
		/// @return 獲得待ちの場合はtrue
		bool HasPendingReward() const { return pendingRelicId_.has_value(); }

		/// @brief 獲得待ちのレリック定義を取得
		/// @return 定義が存在する場合はconstポインター、存在しない場合はnullptr
		const Definition* GetPendingReward() const;

		/// @brief 現在のレリック所持品を取得
		/// @return レリック所持品へのconst参照
		const Inventory& GetInventory() const { return inventory_; }

		/// @brief 現在適用中のレリック定義一覧を取得
		/// @return レリック定義一覧へのconst参照
		const std::vector<Definition>& GetDefinitions() const { return catalog_.GetAll(); }

		/// @brief 所持レリックから集約した補正値を取得
		/// @return 集約済み補正値へのconst参照
		const ModifierSnapshot& GetModifiers() const { return modifiers_; }

		/// @brief レリック効果の変更番号を取得
		/// @return レリック獲得ごとに増える変更番号
		std::uint64_t GetRevision() const { return revision_; }

	private:
		/// @brief 所持中レリックから全補正値を再構築
		void RebuildModifiers();

		/// @brief 定義一覧が現在の所持状態と両立するか確認
		/// @param catalog 確認対象のレリックカタログ
		/// @return 所持レリックが全て定義内かつ上限以内の場合はtrue
		bool IsCompatibleWithInventory(const Catalog& catalog) const;

		Catalog catalog_;
		Inventory inventory_;
		Random random_;
		std::optional<Id> pendingRelicId_;
		ModifierSnapshot modifiers_;
		std::uint64_t revision_ = 0;
		std::string catalogPath_;
	};

}
