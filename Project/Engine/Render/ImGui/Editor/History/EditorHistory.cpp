#include "EditorHistory.h"
#include <algorithm>

namespace MadoEngine::Editor {

EditorHistory& EditorHistory::GetInstance() {
	static EditorHistory instance;
	return instance;
}

void EditorHistory::Push(std::unique_ptr<IEditorCommand> command) {
	if (!command || !command->IsValid()) {
		return;
	}
	if (!undoStack_.empty() && undoStack_.back()->TryMerge(*command)) {

		// 統合後も新規編集扱いとして分岐前のRedo履歴を破棄
		redoStack_.clear();
		return;
	}

	// Undo後に別操作を確定した場合は分岐前のRedo履歴を破棄
	undoStack_.push_back(std::move(command));
	if (undoStack_.size() > kMaximumHistoryCount) {

		// Document SnapshotのMemory使用量を制限するため古い履歴から破棄
		undoStack_.erase(undoStack_.begin());
	}
	redoStack_.clear();
}

bool EditorHistory::Undo() {

	// 編集対象が破棄済みのCommandを読み飛ばして操作可能な履歴を探索
	while (!undoStack_.empty()) {
		std::unique_ptr<IEditorCommand> command = std::move(undoStack_.back());
		undoStack_.pop_back();
		if (!command->IsValid()) {
			continue;
		}
		lastAffectedDomain_ = command->GetHistoryDomain();
		command->Undo();
		redoStack_.push_back(std::move(command));
		return true;
	}
	return false;
}

bool EditorHistory::Redo() {

	// 編集対象が破棄済みのCommandを読み飛ばして操作可能な履歴を探索
	while (!redoStack_.empty()) {
		std::unique_ptr<IEditorCommand> command = std::move(redoStack_.back());
		redoStack_.pop_back();
		if (!command->IsValid()) {
			continue;
		}
		lastAffectedDomain_ = command->GetHistoryDomain();
		command->Redo();
		undoStack_.push_back(std::move(command));
		return true;
	}
	return false;
}

void EditorHistory::Clear() {
	undoStack_.clear();
	redoStack_.clear();
	lastAffectedDomain_ = kInvalidHistoryDomain;
}

bool EditorHistory::CanUndo() const {
	return std::any_of(undoStack_.begin(), undoStack_.end(), [](const std::unique_ptr<IEditorCommand>& command) {
		return command->IsValid();
	});
}

bool EditorHistory::CanRedo() const {
	return std::any_of(redoStack_.begin(), redoStack_.end(), [](const std::unique_ptr<IEditorCommand>& command) {
		return command->IsValid();
	});
}

} // namespace MadoEngine::Editor
