#include "DocumentSnapshotCommand.h"
#include <utility>

namespace MadoEngine::Editor {

DocumentSnapshotCommand::DocumentSnapshotCommand(
	std::size_t documentKey,
	std::uint64_t transactionId,
	std::string beforeSnapshot,
	std::string afterSnapshot,
	RestoreFunction restoreFunction)
	: documentKey_(documentKey),
	transactionId_(transactionId),
	beforeSnapshot_(std::move(beforeSnapshot)),
	afterSnapshot_(std::move(afterSnapshot)),
	restoreFunction_(std::move(restoreFunction)) {
}

void DocumentSnapshotCommand::Undo() {
	if (restoreFunction_) {
		restoreFunction_(beforeSnapshot_);
	}
}

void DocumentSnapshotCommand::Redo() {
	if (restoreFunction_) {
		restoreFunction_(afterSnapshot_);
	}
}

bool DocumentSnapshotCommand::IsValid() const {
	return static_cast<bool>(restoreFunction_);
}

bool DocumentSnapshotCommand::TryMerge(const IEditorCommand& newerCommand) {
	const auto* newerSnapshotCommand = dynamic_cast<const DocumentSnapshotCommand*>(&newerCommand);
	if (!newerSnapshotCommand ||
		newerSnapshotCommand->documentKey_ != documentKey_ ||
		newerSnapshotCommand->transactionId_ != transactionId_) {
		return false;
	}

	// 同じDragや文字入力中の中間Snapshotを破棄して開始前と最新値だけを保持
	afterSnapshot_ = newerSnapshotCommand->afterSnapshot_;
	return true;
}

std::size_t DocumentSnapshotCommand::GetHistoryDomain() const {
	return documentKey_;
}

} // namespace MadoEngine::Editor
