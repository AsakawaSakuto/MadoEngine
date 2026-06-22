#include "ModelTransformCommand.h"
#include "Render/Object/3d/Model/Model.h"

namespace MadoEngine::Editor {

ModelTransformCommand::ModelTransformCommand(Model* target, const TransformSnapshot& before, const TransformSnapshot& after)
	: target_(target),
	before_(before),
	after_(after) {
}

void ModelTransformCommand::Undo() {
	if (!target_) {
		return;
	}

	target_->SetTransform(before_.transform);
}

void ModelTransformCommand::Redo() {
	if (!target_) {
		return;
	}

	target_->SetTransform(after_.transform);
}

} // namespace MadoEngine::Editor
