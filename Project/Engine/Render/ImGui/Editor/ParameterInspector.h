#pragma once
#ifdef USE_IMGUI
#include "Utility/Parameter/ParameterStore.h"
#include <filesystem>

namespace MadoEngine::Editor {

/// @brief 登録済み調整値の階層表示と編集
class ParameterInspector final {
public:
	/// @brief 型に応じた入力欄と保存操作を表示
	/// @param store 編集対象の設定値管理
	/// @param directory 保存と再読込の対象ディレクトリ
	static void Draw(ParameterStore& store, const std::filesystem::path& directory);
};

}
#endif // USE_IMGUI
