#pragma once
#include "EditorCommand.h"
#include <memory>
#include <vector>

namespace MadoEngine::Editor {

class EditorHistory {
public:
	/// @brief シングルトンインスタンスを取得する
	/// @return EditorHistoryの参照
	static EditorHistory& GetInstance();

	/// @brief 実行済みコマンドをUndo履歴へ追加する
	/// @param command 追加するコマンド
	void Push(std::unique_ptr<IEditorCommand> command);

	/// @brief 直前の操作を取り消す
	/// @return Undoできた場合はtrue
	bool Undo();

	/// @brief 取り消した操作をやり直す
	/// @return Redoできた場合はtrue
	bool Redo();

	/// @brief 履歴をすべて削除する
	void Clear();

	/// @brief Undo可能か確認する
	/// @return Undo可能な場合はtrue
	bool CanUndo() const;

	/// @brief Redo可能か確認する
	/// @return Redo可能な場合はtrue
	bool CanRedo() const;

private:
	EditorHistory() = default;

	std::vector<std::unique_ptr<IEditorCommand>> undoStack_;
	std::vector<std::unique_ptr<IEditorCommand>> redoStack_;
};

} // namespace MadoEngine::Editor
