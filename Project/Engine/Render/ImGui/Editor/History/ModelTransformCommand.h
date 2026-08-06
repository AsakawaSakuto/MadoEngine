#pragma once

#include "EditorCommand.h"
#include "Math/Transform.h"
#include "Render/Object/ObjectHandle.h"

namespace MadoEngine::Editor {

struct TransformSnapshot {
	Transform3D transform;
};

/// @brief ModelのTransform変更をUndoとRedoへ適用するCommand
class ModelTransformCommand : public IEditorCommand {
public:
	/// @brief ModelのTransform変更Commandを生成する
	/// @param target 操作対象のModelHandle
	/// @param before 操作前のTransform
	/// @param after 操作後のTransform
	ModelTransformCommand(ModelHandle target, const TransformSnapshot& before, const TransformSnapshot& after);

	/// @brief 操作前のTransformへ戻す
	void Undo() override;

	/// @brief 操作後のTransformへ進める
	void Redo() override;

	/// @brief 操作対象のModelが現在も有効か確認する
	/// @return ModelHandleが有効な場合はtrue
	bool IsValid() const override;

private:
	ModelHandle target_{};
	TransformSnapshot before_;
	TransformSnapshot after_;
};

} // namespace MadoEngine::Editor
