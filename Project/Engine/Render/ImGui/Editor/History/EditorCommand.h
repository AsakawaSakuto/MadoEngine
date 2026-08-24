#pragma once

#include <cstddef>
#include <limits>

namespace MadoEngine::Editor {

inline constexpr std::size_t kInvalidHistoryDomain = (std::numeric_limits<std::size_t>::max)();

class IEditorCommand {
public:
	/// @brief デストラクタ
	virtual ~IEditorCommand() = default;

	/// @brief 操作を取り消し
	virtual void Undo() = 0;

	/// @brief 操作をやり直し
	virtual void Redo() = 0;

	/// @brief コマンドの操作対象が現在も有効か確認
	/// @return UndoまたはRedoを実行できる場合はtrue
	virtual bool IsValid() const = 0;

	/// @brief 新しいCommandを現在Commandへ統合
	/// @param newerCommand 統合候補の新しいCommand
	/// @return 統合できた場合はtrue
	virtual bool TryMerge(const IEditorCommand& newerCommand) {
		(void)newerCommand;
		return false;
	}

	/// @brief 変更対象Editor領域の識別値を取得
	/// @return 対象領域の識別値、領域を持たない場合はkInvalidHistoryDomain
	virtual std::size_t GetHistoryDomain() const {
		return kInvalidHistoryDomain;
	}
};

} // namespace MadoEngine::Editor
