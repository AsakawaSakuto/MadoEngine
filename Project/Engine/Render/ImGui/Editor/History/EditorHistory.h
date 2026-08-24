#pragma once
#include "EditorCommand.h"
#include <memory>
#include <vector>

namespace MadoEngine::Editor {

class EditorHistory {
public:
	/// @brief シングルトンインスタンスを取得
	/// @return EditorHistoryの参照
	static EditorHistory& GetInstance();

	/// @brief 実行済みコマンドをUndo履歴へ追加
	/// @param command 追加するコマンド
	void Push(std::unique_ptr<IEditorCommand> command);

	/// @brief 直前の操作を取り消し
	/// @return Undoできた場合はtrue
	bool Undo();

	/// @brief 取り消した操作をやり直し
	/// @return Redoできた場合はtrue
	bool Redo();

	/// @brief 履歴をすべて削除
	void Clear();

	/// @brief Undo可能か確認
	/// @return Undo可能な場合はtrue
	bool CanUndo() const;

	/// @brief Redo可能か確認
	/// @return Redo可能な場合はtrue
	bool CanRedo() const;

	/// @brief 最後にUndoまたはRedoしたEditor領域を取得
	/// @return Editor領域の識別値、操作前の場合はkInvalidHistoryDomain
	std::size_t GetLastAffectedDomain() const { return lastAffectedDomain_; }

private:
	EditorHistory() = default;
	static constexpr std::size_t kMaximumHistoryCount = 128;

	std::vector<std::unique_ptr<IEditorCommand>> undoStack_;
	std::vector<std::unique_ptr<IEditorCommand>> redoStack_;
	std::size_t lastAffectedDomain_ = kInvalidHistoryDomain;
};

} // namespace MadoEngine::Editor
