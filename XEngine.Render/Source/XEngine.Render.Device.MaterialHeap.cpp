#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.MaterialHeap.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.ClassLinkage.h"

#define device this->getDevice()

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Device_;

void MaterialHeap::initialize()
{
	device.d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(0x10000),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dMaterialsTable.uuid(), d3dMaterialsTable.voidInitRef());

	d3dMaterialsTable->Map(0, &D3D12Range(), to<void**>(&mappedMaterialsTable));
}

MaterialHandle MaterialHeap::createMaterial(EffectHandle effect,
	const void* initialConstants, const TextureHandle* intialTextures)
{
	MaterialHandle result = MaterialHandle(materialCount);
	materialCount++;
	return result;
}

void MaterialHeap::updateMaterialConstants(MaterialHandle handle,
	uint32 offset, const void* data, uint32 size)
{
	byte *materialData = mappedMaterialsTable +
		device.effectHeap.getMaterialConstantsSize(0) * handle;
	Memory::Copy(materialData + offset, data, size);
}

EffectHandle MaterialHeap::getEffect(MaterialHandle handle) const
{
	return 0;
}

uint64 MaterialHeap::getMaterialsTableGPUAddress(EffectHandle effectHandle)
{
	return d3dMaterialsTable->GetGPUVirtualAddress();
}