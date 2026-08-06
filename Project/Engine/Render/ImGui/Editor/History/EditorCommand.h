#pragma once

namespace MadoEngine::Editor {

class IEditorCommand {
public:
	/// @brief デストラクタ
	virtual ~IEditorCommand() = default;

	/// @brief 操作を取り消す
	virtual void Undo() = 0;

	/// @brief 操作をやり直す
	virtual void Redo() = 0;

	/// @brief コマンドの操作対象が現在も有効か確認する
	/// @return UndoまたはRedoを実行できる場合はtrue
	virtual bool IsValid() const = 0;
};

} // namespace MadoEngine::Editor
