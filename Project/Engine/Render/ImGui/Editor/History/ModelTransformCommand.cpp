#include "ModelTransformCommand.h"
#include "Render/Object/3d/Model/ModelManager.h"

namespace MadoEngine::Editor {

ModelTransformCommand::ModelTransformCommand(ModelHandle target, const TransformSnapshot& before, const TransformSnapshot& after)
	: target_(target),
	before_(before),
	after_(after) {
}

void ModelTransformCommand::Undo() {
	if (Model* target = ModelManager::GetInstance().TryGet(target_)) {
		target->SetTransform(before_.transform);
	}
}

void ModelTransformCommand::Redo() {
	if (Model* target = ModelManager::GetInstance().TryGet(target_)) {
		target->SetTransform(after_.transform);
	}
}

bool ModelTransformCommand::IsValid() const {
	return ModelManager::GetInstance().IsValid(target_);
}

} // namespace MadoEngine::Editor
