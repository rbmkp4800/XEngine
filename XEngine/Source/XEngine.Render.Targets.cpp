#include <d3d12.h>
#include <dxgi1_5.h>
#include "Util.D3D12.h"
#include "Util.DXGI.h"

#include <XLib.Memory.h>

#include "XEngine.Render.Targets.h"
#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine;

void XERWindowTarget::initialize(XERDevice* device, void* hWnd, uint32 width, uint32 height)
{
	COMPtr<IDXGISwapChain1> dxgiSwapChain1;
	XERDevice::dxgiFactory->CreateSwapChainForHwnd(device->graphicsGPUQueue.getD3DCommandQueue(), HWND(hWnd),
		&DXGISwapChainDesc1(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, bufferCount,
		DXGI_SWAP_EFFECT_FLIP_DISCARD), nullptr, nullptr, dxgiSwapChain1.initRef());
	dxgiSwapChain1->QueryInterface(dxgiSwapChain.uuid(), dxgiSwapChain.voidInitRef());

	for (uint32 i = 0; i < bufferCount; i++)
	{
		COMPtr<ID3D12Resource> d3dBackBufferTexture;
		dxgiSwapChain->GetBuffer(i, d3dBackBufferTexture.uuid(), d3dBackBufferTexture.voidInitRef());
		uint32 rtvDescriptor = device->rtvHeap.allocateDescriptors(1);
		device->d3dDevice->CreateRenderTargetView(d3dBackBufferTexture,
			nullptr, device->rtvHeap.getCPUHandle(rtvDescriptor));

		buffers[i].initialize(d3dBackBufferTexture.moveToPtr(), rtvDescriptor);
	}
}

void XERWindowTarget::present(bool waitVSync)
{
	dxgiSwapChain->Present(waitVSync ? 1 : 0, 0);
}

XERTargetBuffer* XERWindowTarget::getCurrentTargetBuffer()
{
	return buffers + dxgiSwapChain->GetCurrentBackBufferIndex();
}