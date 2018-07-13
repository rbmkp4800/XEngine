#include <d3d12.h>

#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.DescriptorHeap.h"

using namespace XEngine::Render;
using namespace XEngine::Render::Device_;

void DescriptorHeap::initalize(ID3D12Device* d3dDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE type, uint16 size, bool shaderVisible)
{
	d3dDevice->CreateDescriptorHeap(
		&D3D12DescriptorHeapDesc(type, size,
			shaderVisible ?
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		d3dDescriptorHeap.uuid(), d3dDescriptorHeap.voidInitRef());
	allocatedDescriptorsCount = 0;
	descritorSize = d3dDevice->GetDescriptorHandleIncrementSize(type);
}

uint16 DescriptorHeap::allocate(uint16 count)
{
	uint16 result = allocatedDescriptorsCount;
	allocatedDescriptorsCount += count;
	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCPUHandle(uint16 descriptor)
	{ return d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart() + descriptor * descritorSize; }
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGPUHandle(uint16 descriptor)
	{ return d3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart() + descriptor * descritorSize; }