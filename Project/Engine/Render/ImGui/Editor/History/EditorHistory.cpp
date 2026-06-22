#include "EditorHistory.h"

namespace MadoEngine::Editor {

EditorHistory& EditorHistory::GetInstance() {
	static EditorHistory instance;
	return instance;
}

void EditorHistory::Push(std::unique_ptr<IEditorCommand> command) {
	if (!command) {
		return;
	}

	undoStack_.push_back(std::move(command));
	redoStack_.clear();
}

bool EditorHistory::Undo() {
	if (!CanUndo()) {
		return false;
	}

	std::unique_ptr<IEditorCommand> command = std::move(undoStack_.back());
	undoStack_.pop_back();

	command->Undo();
	redoStack_.push_back(std::move(command));
	return true;
}

bool EditorHistory::Redo() {
	if (!CanRedo()) {
		return false;
	}

	std::unique_ptr<IEditorCommand> command = std::move(redoStack_.back());
	redoStack_.pop_back();

	command->Redo();
	undoStack_.push_back(std::move(command));
	return true;
}

void EditorHistory::Clear() {
	undoStack_.clear();
	redoStack_.clear();
}

bool EditorHistory::CanUndo() const {
	return !undoStack_.empty();
}

bool EditorHistory::CanRedo() const {
	return !redoStack_.empty();
}

} // namespace MadoEngine::Editor
