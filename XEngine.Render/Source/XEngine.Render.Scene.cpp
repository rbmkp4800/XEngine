#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Math.Matrix3x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"

#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;

static constexpr uint32 commandListArenaSegmentSize = 0x4000;

#pragma pack(push, 1)

namespace
{
	struct IndirectCommand
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		uint32 baseTransformIndex;
		uint32 materialIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};
}

#pragma pack(pop)

void Scene::initialize(Device& device, uint32 initialTransformBufferSize)
{
	this->device = &device;
	transformBufferSize = initialTransformBufferSize;

	ID3D12Device *d3dDevice = device.d3dDevice;

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(Matrix3x4) * transformBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dTransformBuffer.uuid(), d3dTransformBuffer.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(commandListArenaSegmentSize * 4),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dCommandListArena.uuid(), d3dCommandListArena.voidInitRef());

	d3dTransformBuffer->Map(0, &D3D12Range(), to<void**>(&mappedTransformBuffer));
	d3dCommandListArena->Map(0, &D3D12Range(), to<void**>(&mappedCommandListArena));
}

void Scene::destroy()
{

}

GeometryInstanceHandle Scene::createGeometryInstance(
	const GeometryDesc& geometryDesc, MaterialHandle material,
	uint32 transformCount, const Matrix3x4* intialTransforms)
{
	uint32 baseTransformIndex = allocatedTansformCount;
	allocatedTansformCount++;

	GeometryInstanceHandle result = GeometryInstanceHandle(instances.getSize());

	{
		Instance &instance = instances.allocateBack();
		instance.geometryDesc = geometryDesc;
		instance.baseTransformIndex = baseTransformIndex;
		instance.material = material;
	}

	{
		EffectHandle effect = device->materialHeap.getEffect(material);

		CommandListDesc *commandListDesc = nullptr;
		for (CommandListDesc& i : commandLists)
		{
			if (i.effect == effect)
			{
				commandListDesc = &i;
				break;
			}
		}

		if (!commandListDesc)
		{
			commandListDesc = &commandLists.allocateBack();
			commandListDesc->effect = effect;
			commandListDesc->arenaBaseSegment = allocatedCommandListArenaSegmentCount;
			commandListDesc->length = 0;
			allocatedCommandListArenaSegmentCount++;
		}

		uint32 commandIndex = commandListDesc->length;
		commandListDesc->length++;

		IndirectCommand *mappedCommandList = to<IndirectCommand*>(mappedCommandListArena +
			commandListDesc->arenaBaseSegment * commandListArenaSegmentSize);

		int a = sizeof(IndirectCommand);

		IndirectCommand &command = mappedCommandList[commandIndex];
			// VBV
		command.vertexBufferView.BufferLocation =
			device->bufferHeap.getBufferGPUAddress(geometryDesc.vertexBufferHandle) + geometryDesc.vertexDataOffset;
		command.vertexBufferView.SizeInBytes = geometryDesc.vertexDataSize;
		command.vertexBufferView.StrideInBytes = geometryDesc.vertexStride;
			// IBV
		command.indexBufferView.BufferLocation =
			device->bufferHeap.getBufferGPUAddress(geometryDesc.indexBufferHandle) + geometryDesc.indexDataOffset;
		command.indexBufferView.SizeInBytes = geometryDesc.indexCount * (geometryDesc.indexIs32Bit ? 4 : 2);
		command.indexBufferView.Format = geometryDesc.indexIs32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			// Constats
		command.baseTransformIndex = baseTransformIndex;
		command.materialIndex = uint32(material);
			// Draw arguments
		command.drawIndexedArguments.IndexCountPerInstance = geometryDesc.indexCount;
		command.drawIndexedArguments.InstanceCount = 1;
		command.drawIndexedArguments.StartIndexLocation = 0;
		command.drawIndexedArguments.BaseVertexLocation = 0;
		command.drawIndexedArguments.StartInstanceLocation = 0;
	}

	return result;
}

void Scene::populateCommandList(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12CommandSignature* d3dICS)
{
	d3dCommandList->SetGraphicsRootShaderResourceView(3,
		d3dTransformBuffer->GetGPUVirtualAddress());

	for (CommandListDesc& commandList : commandLists)
	{
		ID3D12PipelineState *d3dPSO = device->effectHeap.getPSO(commandList.effect);
		d3dCommandList->SetPipelineState(d3dPSO);

		d3dCommandList->SetGraphicsRootConstantBufferView(2,
			device->materialHeap.getMaterialsTableGPUAddress(commandList.effect));

		d3dCommandList->ExecuteIndirect(d3dICS, commandList.length, d3dCommandListArena,
			commandList.arenaBaseSegment * commandListArenaSegmentSize, nullptr, 0);
	}
}

void Scene::updateGeometryInstanceTransform(GeometryInstanceHandle handle,
	const Matrix3x4& transform, uint16 transformIndex)
{
	uint32 globalTransformIndex = instances[handle].baseTransformIndex + transformIndex;
	mappedTransformBuffer[globalTransformIndex] = transform;
}