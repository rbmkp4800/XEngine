#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.UI.Resources.h"

#include "XEngine.Render.Device.h"

using namespace XEngine::Render;
using namespace XEngine::Render::UI;

void Texture::initializeA8(Device& device, const void* data, uint16 width, uint16 height)
{
	device.d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_A8_UNORM, width, height),
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		d3dTexture.uuid(), d3dTexture.voidInitRef());

	srvDescriptorIndex = device.srvHeap.allocate(1);
	device.d3dDevice->CreateShaderResourceView(d3dTexture, nullptr,
		device.srvHeap.getCPUHandle(srvDescriptorIndex));

	device.uploader.uploadTexture2DAndGenerateMIPs(d3dTexture, data, width, to<void*>(data));
}