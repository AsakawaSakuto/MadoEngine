#include "SpriteData.h"
#include "Render/Object/RenderObject2d.h"

class Sprite : public RenderObject2d {
public:

	Sprite(std::string ObjectName);

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) override;

	void Update() override;

	void Draw() override;
private:

	// リソースデータ（2D Sprite専用構造体を使用）
	SpriteVertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	SpriteMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	Vector2 anchorPoint_ = { 0.0f,0.0f };
	AnchorPoint anchorType_ = AnchorPoint::TopLeft; // アンカーポイント（デフォルトは左上）
};