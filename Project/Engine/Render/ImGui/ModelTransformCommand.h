#pragma once
#include "EditorCommand.h"
#include "Math/Transform.h"

class Model;

namespace MadoEngine::Editor {

struct TransformSnapshot {
	Transform3D transform;
};

class ModelTransformCommand : public IEditorCommand {
public:
	/// @brief ModelのTransform変更コマンドを生成する
	/// @param target 操作対象のModel
	/// @param before 操作前のTransform
	/// @param after 操作後のTransform
	ModelTransformCommand(Model* target, const TransformSnapshot& before, const TransformSnapshot& after);

	/// @brief 操作前のTransformへ戻す
	void Undo() override;

	/// @brief 操作後のTransformへ進める
	void Redo() override;

private:
	Model* target_ = nullptr;
	TransformSnapshot before_;
	TransformSnapshot after_;
};

} // namespace MadoEngine::Editor
