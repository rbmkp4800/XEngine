#include <d3d12.h>
#include <dxgi1_6.h>

#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.h"

#include "XEngine.Render.UI.Batch.h"

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

	d3dDevice->CreateCommandQueue(&D3D12CommandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT),
		d3dGraphicsQueue.uuid(), d3dGraphicsQueue.voidInitRef());
	d3dDevice->CreateCommandQueue(&D3D12CommandQueueDesc(D3D12_COMMAND_LIST_TYPE_COPY),
		d3dCopyQueue.uuid(), d3dCopyQueue.voidInitRef());

	srvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	rtvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 32, false);
	dsvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32, false);

	uploader.initialize();
	sceneRenderer.initialize();

	materialHeap.initialize();
	bufferHeap.initialize();
	textureHeap.initialize();

	uiResources.initialize(d3dDevice);

	gpuQueueSyncronizer.initialize(d3dDevice);

	d3dGraphicsQueue->GetTimestampFrequency(&graphicsQueueClockFrequency);

	return true;
}

void Device::updateBuffer(BufferHandle buffer, uint32 destOffset, const void* srcData, uint32 size)
{
	uploader.uploadBuffer(bufferHeap.getD3DResource(buffer), destOffset, srcData, size);
}

void Device::updateTexture(TextureHandle texture, const void* sourceData, uint32 sourceRowPitch, void* mipsGenerationBuffer)
{
	uploader.uploadTexture2DAndGenerateMIPs(textureHeap.getTexture(texture), sourceData, sourceRowPitch, mipsGenerationBuffer);
}

void Device::updateMaterialConstants(MaterialHandle material, uint32 offset, const void* data, uint32 size)
{
	materialHeap.updateMaterialConstants(material, offset, data, size);
}

void Device::updateMaterialTexture(MaterialHandle handle, uint32 slot, TextureHandle textureHandle)
{
	materialHeap.updateMaterialTexture(handle, slot, textureHandle);
}

void Device::clearTarget(Target& target, XLib::Color color)
{

}

void Device::renderScene(Scene& scene, const Camera& camera, GBuffer& gBuffer,
	Target& target, rectu16 viewport, bool finalizeTarget, DebugOutput debugOutput)
{
	sceneRenderer.render(scene, camera, gBuffer, target, viewport, finalizeTarget, debugOutput);
}

void Device::renderUI(UI::Batch& uiBatch)
{
	ID3D12CommandList *d3dCommandListsToExecute[] = { uiBatch.d3dCommandList };
	d3dGraphicsQueue->ExecuteCommandLists(1, d3dCommandListsToExecute);
}

void Device::finishFrame()
{
	gpuQueueSyncronizer.synchronize(d3dGraphicsQueue);
	sceneRenderer.updateTimings();
}