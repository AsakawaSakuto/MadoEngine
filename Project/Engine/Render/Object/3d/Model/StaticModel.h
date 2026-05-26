#include "ModelData.h"
#include "Render/Object/RenderObject3d.h"

class StaticModel : public RenderObject3d {
public:

	StaticModel(std::string ObjectName);

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;

	void Update() override;
	
	void Draw() override;
private:

};