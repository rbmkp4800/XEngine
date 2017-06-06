#include <d3d12.h>
#include <dxgi1_5.h>
#include "Util.D3D12.h"
#include "Util.DXGI.h"

#include <XLib.Memory.h>

#include "XEngine.Render.Targets.h"
#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine;

void XERWindowTarget::updateRTVs()
{
	for (uint32 i = 0; i < bufferCount; i++)
	{
		XERTargetBuffer &buffer = buffers[i];

		dxgiSwapChain->GetBuffer(i, buffer.d3dTexture.uuid(), buffer.d3dTexture.voidInitRef());

		if (buffer.rtvDescriptor == uint32(-1))
			buffer.rtvDescriptor = device->rtvHeap.allocateDescriptors(1);
		device->d3dDevice->CreateRenderTargetView(buffer.d3dTexture, nullptr,
			device->rtvHeap.getCPUHandle(buffer.rtvDescriptor));
	}
}

void XERWindowTarget::initialize(XERDevice* device, void* hWnd, uint32 width, uint32 height)
{
	this->device = device;

	COMPtr<IDXGISwapChain1> dxgiSwapChain1;
	XERDevice::dxgiFactory->CreateSwapChainForHwnd(device->graphicsGPUQueue.getD3DCommandQueue(), HWND(hWnd),
		&DXGISwapChainDesc1(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, bufferCount,
		DXGI_SWAP_EFFECT_FLIP_DISCARD), nullptr, nullptr, dxgiSwapChain1.initRef());
	dxgiSwapChain1->QueryInterface(dxgiSwapChain.uuid(), dxgiSwapChain.voidInitRef());
	updateRTVs();
}	

void XERWindowTarget::resize(uint32 width, uint32 height)
{
	for each (XERTargetBuffer& buffer in buffers)
		buffer.d3dTexture.destroy();

	dxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	updateRTVs();
}

void XERWindowTarget::present(bool waitVSync)
{
	dxgiSwapChain->Present(waitVSync ? 1 : 0, 0);
}

XERTargetBuffer* XERWindowTarget::getCurrentTargetBuffer()
{
	return buffers + dxgiSwapChain->GetCurrentBackBufferIndex();
}