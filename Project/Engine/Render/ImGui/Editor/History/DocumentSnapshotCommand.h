#pragma once

#include "EditorCommand.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace MadoEngine::Editor {

/// @brief Editor Document全体の変更前後Snapshotを保持するCommand
class DocumentSnapshotCommand final : public IEditorCommand {
public:
	using RestoreFunction = std::function<void(const std::string&)>;

	/// @brief Document Snapshot Commandを生成
	/// @param documentKey 対象Documentの識別値
	/// @param transactionId 同一UI操作をまとめるTransaction識別値
	/// @param beforeSnapshot 操作前Snapshot
	/// @param afterSnapshot 操作後Snapshot
	/// @param restoreFunction Snapshot復元関数
	DocumentSnapshotCommand(
		std::size_t documentKey,
		std::uint64_t transactionId,
		std::string beforeSnapshot,
		std::string afterSnapshot,
		RestoreFunction restoreFunction
	);

	/// @brief 操作前Snapshotへ復元
	void Undo() override;

	/// @brief 操作後Snapshotへ復元
	void Redo() override;

	/// @brief Snapshot復元関数が有効か確認
	/// @return 復元可能な場合はtrue
	bool IsValid() const override;

	/// @brief 同一UI操作の新しいSnapshotを現在Commandへ統合
	/// @param newerCommand 統合候補の新しいCommand
	/// @return 統合できた場合はtrue
	bool TryMerge(const IEditorCommand& newerCommand) override;

	/// @brief 変更対象Documentの識別値を取得
	/// @return 対象Documentの識別値
	std::size_t GetHistoryDomain() const override;

private:
	std::size_t documentKey_ = kInvalidHistoryDomain;
	std::uint64_t transactionId_ = 0;
	std::string beforeSnapshot_;
	std::string afterSnapshot_;
	RestoreFunction restoreFunction_;
};

} // namespace MadoEngine::Editor
