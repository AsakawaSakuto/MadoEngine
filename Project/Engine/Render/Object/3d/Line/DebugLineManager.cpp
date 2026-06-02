#include "DebugLineManager.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

DebugLineManager& DebugLineManager::GetInstance() {
	static DebugLineManager instance;
	return instance;
}

void DebugLineManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, uint32_t maxVertices) {
	Logger::Output("初期化開始、最大頂点数 : " + std::to_string(maxVertices), Logger::Level::Engine);
	line_ = std::make_unique<Line3d>();
	line_->Initialize(device, commandList, maxVertices);
	Logger::Output("初期化完了", Logger::Level::Engine);
}

void DebugLineManager::AddShape(const Shape& shape, const Vector4& color) {

	// std::visit を使って型に応じた処理を実行
	std::visit([this, &color](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr (std::is_same_v<T, AABB>) {
			line_->AddBox(arg, color);
		} else if constexpr (std::is_same_v<T, OBB>) {
			line_->AddBox(arg, color);
		} else if constexpr (std::is_same_v<T, Sphere>) {
			line_->AddSphere(arg, color);
		} else if constexpr (std::is_same_v<T, OvalSphere>) {
			line_->AddOvalSphere(arg, color);
		} else if constexpr (std::is_same_v<T, Plane>) {
			line_->AddPlane(arg, 10, color); // divisions = 10 をデフォルト
		} else if constexpr (std::is_same_v<T, Segment>) {
			line_->AddSegment(arg, color);
		} else if constexpr (std::is_same_v<T, Line>) {
			line_->AddLine(arg, color);
		} else if constexpr (std::is_same_v<T, Circle>) {
			line_->AddCircle(arg, color);
		}
		}, shape);
}

void DebugLineManager::AddGrid(float size, int divisions, const Vector4& color) {
	line_->AddGrid(size, divisions, color);
}

void DebugLineManager::Draw(Camera& camera) {

	// 描画
	if (isDrawing_) {
		line_->Draw(camera);
	} else {
		
	}

}

void DebugLineManager::SetPSORegistry(MadoEngine::Render::PSORegistry* psoRegistry) {
	psoRegistry_ = psoRegistry;
	line_->SetPSORegistry(psoRegistry);
}