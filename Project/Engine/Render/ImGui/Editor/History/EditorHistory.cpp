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

	undoStack_.push_back(std::move(command));
	redoStack_.clear();
}

bool EditorHistory::Undo() {
	while (!undoStack_.empty()) {
		std::unique_ptr<IEditorCommand> command = std::move(undoStack_.back());
		undoStack_.pop_back();
		if (!command->IsValid()) {
			continue;
		}
		command->Undo();
		redoStack_.push_back(std::move(command));
		return true;
	}
	return false;
}

bool EditorHistory::Redo() {
	while (!redoStack_.empty()) {
		std::unique_ptr<IEditorCommand> command = std::move(redoStack_.back());
		redoStack_.pop_back();
		if (!command->IsValid()) {
			continue;
		}
		command->Redo();
		undoStack_.push_back(std::move(command));
		return true;
	}
	return false;
}

void EditorHistory::Clear() {
	undoStack_.clear();
	redoStack_.clear();
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
