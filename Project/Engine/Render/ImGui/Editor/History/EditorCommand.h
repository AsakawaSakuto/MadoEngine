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
};

} // namespace MadoEngine::Editor
