#pragma once

namespace MadoEngine {
	class Execution;
}

namespace RenderPassSetup {

	/// @brief アプリケーションで使用するLayerEffectPassを初期化して登録する
	/// @param execution 登録先のExecution
	void RegisterLayerEffectPasses(MadoEngine::Execution& execution);

} // namespace RenderPassSetup
