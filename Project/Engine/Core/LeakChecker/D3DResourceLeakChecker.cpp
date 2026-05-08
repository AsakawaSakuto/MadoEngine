#include"D3DResourceLeakChecker.h"
#include<wrl.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include"dxgidebug.h"

D3DResourceLeakChecker::~D3DResourceLeakChecker() {
	// リソースリークチェック
	Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
	HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
	if (SUCCEEDED(hr)) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}
}