#include <d3d12.h>
#include <dxgi1_6.h>

#include "XEngine.Render.Device.h"

using namespace XLib::Platform;
using namespace XEngine::Render;

COMPtr<IDXGIFactory5> Device::dxgiFactory;
COMPtr<ID3D12Debug> d3dDebug;

bool Device::initialize()
{
	if (!dxgiFactory.isInitialized())
		CreateDXGIFactory1(dxgiFactory.uuid(), dxgiFactory.voidInitRef());

	if (!d3dDebug.isInitialized())
	{
		D3D12GetDebugInterface(d3dDebug.uuid(), d3dDebug.voidInitRef());
		d3dDebug->EnableDebugLayer();
	}

	for (uint32 adapterIndex = 0;; adapterIndex++)
	{
		COMPtr<IDXGIAdapter1> dxgiAdapter;
		if (dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.initRef()) == DXGI_ERROR_NOT_FOUND)
		{
			dxgiFactory->EnumWarpAdapter(dxgiAdapter.uuid(), dxgiAdapter.voidInitRef());
			if (FAILED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, d3dDevice.uuid(), d3dDevice.voidInitRef())))
				return false;
			break;
		}

		DXGI_ADAPTER_DESC1 desc = {};
		dxgiAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, d3dDevice.uuid(), d3dDevice.voidInitRef())))
			break;
	}

	sceneRendererResources.inititalize();
	geometryHeap .initialize();
	textureHeap  .initialize();
	effectHeap   .initialize();
	materialHeap .initialize();
}