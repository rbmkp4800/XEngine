#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.MaterialHeap.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.Internal.Shaders.h"
#include "XEngine.Render.ClassLinkage.h"

#define device this->getDevice()

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;
using namespace XEngine::Render::Device_;

static constexpr uint32 materialTableArenaSegmentSize = 0x1000;

void MaterialHeap::initialize()
{
	device.d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(0x10000),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dMaterialConstantsTableArena.uuid(), d3dMaterialConstantsTableArena.voidInitRef());

	d3dMaterialConstantsTableArena->Map(0, &D3D12Range(), to<void**>(&mappedMaterialConstantsTableArena));
}

EffectHandle MaterialHeap::createEffect_perMaterialAlbedoRoughtnessMetalness()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = device.sceneRenderer.getGBufferPassD3DRS();
	psoDesc.VS = D3D12ShaderBytecode(Shaders::Effect_NormalVS.data, Shaders::Effect_NormalVS.size);
	psoDesc.PS = D3D12ShaderBytecode(Shaders::Effect_PerMaterialAlbedoRoughtnessMetalnessPS.data, Shaders::Effect_PerMaterialAlbedoRoughtnessMetalnessPS.size);
	psoDesc.BlendState = D3D12BlendDesc_NoBlend();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
	psoDesc.DepthStencilState = D3D12DepthStencilDesc_Default();
	psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_SNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	COMPtr<ID3D12PipelineState> d3dPSO;
	device.d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dPSO.uuid(), d3dPSO.voidInitRef());

	EffectHandle result = EffectHandle(effectCount);
	effectCount++;

	Effect &effect = effects[result];
	effect.d3dPSO = move(d3dPSO);
	effect.materialTableArenaBaseSegment = allocatedCommandListArenaSegmentCount;
	effect.materialConstantsSize = 32;
	effect.constantsUsed = 0;
	allocatedCommandListArenaSegmentCount++;

	return result;
}

MaterialHandle MaterialHeap::createMaterial(EffectHandle effectHandle,
	const void* initialConstants, const TextureHandle* intialTextures)
{
	MaterialHandle result = MaterialHandle(materialCount);
	materialCount++;

	Effect &effect = effects[effectHandle];
	uint16 constantsIndex = effect.constantsUsed;
	effect.constantsUsed++;

	materials[result].effectHandle = effectHandle;
	materials[result].constantsIndex = constantsIndex;

	return result;
}

void MaterialHeap::updateMaterialConstants(MaterialHandle handle,
	uint32 offset, const void* data, uint32 size)
{
	const Material &material = materials[handle];
	const Effect &effect = effects[material.effectHandle];

	uint32 materialConstantsAbsoluteOffset =
		effect.materialTableArenaBaseSegment * materialTableArenaSegmentSize +
		effect.materialConstantsSize * material.constantsIndex;

	byte *materialConstants = mappedMaterialConstantsTableArena + materialConstantsAbsoluteOffset;
	Memory::Copy(materialConstants + offset, data, size);
}

EffectHandle MaterialHeap::getEffectForMaterial(MaterialHandle handle) const
{
	return materials[handle].effectHandle;
}

ID3D12PipelineState* MaterialHeap::getEffectPSO(EffectHandle handle)
{
	return effects[handle].d3dPSO;
}

uint32 MaterialHeap::getEffectMaterialConstantsSize(EffectHandle handle) const
{
	return effects[handle].materialConstantsSize;
}

uint64 MaterialHeap::getEffectMaterialConstantsTableGPUAddress(EffectHandle handle)
{
	return d3dMaterialConstantsTableArena->GetGPUVirtualAddress() +
		effects[handle].materialTableArenaBaseSegment * materialTableArenaSegmentSize;
}