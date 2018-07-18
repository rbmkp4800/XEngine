#include <d3d12.h>

#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.GBuffer.h"

#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine::Render;

void GBuffer::initialize(Device& device, uint16x2 size)
{
	this->device = &device;
	this->size = size;

	ID3D12Device *d3dDevice = device.d3dDevice;

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R8G8B8A8_UNORM, size.x, size.y,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_RENDER_TARGET,
		&D3D12ClearValue_Color(DXGI_FORMAT_R8G8B8A8_UNORM),
		d3dDiffuseTexture.uuid(), d3dDiffuseTexture.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R16G16B16A16_SNORM, size.x, size.y,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr, d3dNormalRoughnessMetalnessTexture.uuid(), d3dNormalRoughnessMetalnessTexture.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R24G8_TYPELESS, size.x, size.y,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&D3D12ClearValue_D24S8(1.0f), d3dDepthTexture.uuid(), d3dDepthTexture.voidInitRef());

	srvDescriptorsBaseIndex = device.srvHeap.allocate(3);
	d3dDevice->CreateShaderResourceView(d3dDiffuseTexture, nullptr,
		device.srvHeap.getCPUHandle(srvDescriptorsBaseIndex + 0));
	d3dDevice->CreateShaderResourceView(d3dNormalRoughnessMetalnessTexture, nullptr,
		device.srvHeap.getCPUHandle(srvDescriptorsBaseIndex + 1));
	d3dDevice->CreateShaderResourceView(d3dDepthTexture,
		&D3D12ShaderResourceViewDesc_Texture2D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS),
		device.srvHeap.getCPUHandle(srvDescriptorsBaseIndex + 2));

	rtvDescriptorsBaseIndex = device.rtvHeap.allocate(2);
	d3dDevice->CreateRenderTargetView(d3dDiffuseTexture, nullptr,
		device.rtvHeap.getCPUHandle(rtvDescriptorsBaseIndex + 0));
	d3dDevice->CreateRenderTargetView(d3dNormalRoughnessMetalnessTexture, nullptr,
		device.rtvHeap.getCPUHandle(rtvDescriptorsBaseIndex + 1));

	dsvDescriptorIndex = device.dsvHeap.allocate(1);
	d3dDevice->CreateDepthStencilView(d3dDepthTexture,
		&D3D12DepthStencilViewDesc_Texture2D(DXGI_FORMAT_D24_UNORM_S8_UINT),
		device.dsvHeap.getCPUHandle(dsvDescriptorIndex));
}

void GBuffer::resize(uint16x2 size)
{

}