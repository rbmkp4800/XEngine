#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Math.Matrix3x4.h>
#include <XLib.Math.Matrix4x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"

#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;

static constexpr uint32 commandListArenaSegmentSize = 0x4000;
static constexpr uint16 shadowMapDim = 2048;

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

struct alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
	Scene::ShadowCameraTransformConstants
{
	Matrix4x4 transform;
};

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

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(ShadowCameraTransformConstants)),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dShadowCameraTransformsCB.uuid(), d3dShadowCameraTransformsCB.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_D16_UNORM, shadowMapDim, shadowMapDim,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&D3D12ClearValue_DepthStencil(DXGI_FORMAT_D16_UNORM, 1.0f),
		d3dShadowMapAtlas.uuid(), d3dShadowMapAtlas.voidInitRef());

	d3dTransformBuffer->Map(0, &D3D12Range(), to<void**>(&mappedTransformBuffer));
	d3dCommandListArena->Map(0, &D3D12Range(), to<void**>(&mappedCommandListArena));
	d3dShadowCameraTransformsCB->Map(0, &D3D12Range(), to<void**>(&mappedShadowCameraTransformsCB));

	shadowMapAtlasSRVDescriptorIndex = device.srvHeap.allocate(1);
	d3dDevice->CreateShaderResourceView(d3dShadowMapAtlas,
		&D3D12ShaderResourceViewDesc_Texture2D(DXGI_FORMAT_R16_UNORM),
		device.srvHeap.getCPUHandle(shadowMapAtlasSRVDescriptorIndex));
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

void Scene::populateCommandListForGBufferPass(ID3D12GraphicsCommandList* d3dCommandList,
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

void Scene::populateCommandListForShadowPass(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12CommandSignature* d3dICS, ID3D12PipelineState* d3dPSO)
{
	if (directionalLightCount == 0)
		return;

	DirectionalLight& light = directionalLights[0];

	mappedShadowCameraTransformsCB->transform =
		Matrix4x4::LookAtCentered(light.shadowVolumeOrigin, light.desc.direction, { 0.0f, 0.0f, 1.0f })
		* Matrix4x4::Scale(float32x3(1.0f, 1.0f, 1.0f) / light.shadowVolumeSize);
		//Matrix4x4::LookAt({ 30.0f, 30.0f, 30.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }) *
		//Matrix4x4::Scale({ 0.05f, 0.05f, 0.01f });

	D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle =
		device->dsvHeap.getCPUHandle(directionalLights[0].dsvDescriptorIndex);

	d3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);
	d3dCommandList->ClearDepthStencilView(dsvDescriptorHandle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, shadowMapDim, shadowMapDim));
	d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, shadowMapDim, shadowMapDim));

	d3dCommandList->SetGraphicsRootShaderResourceView(3, d3dTransformBuffer->GetGPUVirtualAddress());
	d3dCommandList->SetGraphicsRootConstantBufferView(4, d3dShadowCameraTransformsCB->GetGPUVirtualAddress());

	d3dCommandList->SetPipelineState(d3dPSO);

	for (CommandListDesc& commandList : commandLists)
	{
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

uint8 Scene::createDirectionalLight(const DirectionalLightDesc& desc)
{
	if (directionalLightCount)
		return 0;

	uint8 result = directionalLightCount;

	directionalLights[result].desc = desc;
	directionalLights[result].shadowVolumeOrigin = VectorMath::Normalize(desc.direction) * -20.0f;
	directionalLights[result].shadowVolumeSize = { 40.0f, 40.0f, 40.0f };

	directionalLights[result].dsvDescriptorIndex = device->dsvHeap.allocate(1);
	device->d3dDevice->CreateDepthStencilView(d3dShadowMapAtlas,
		&D3D12DepthStencilViewDesc_Texture2D(DXGI_FORMAT_D16_UNORM),
		device->dsvHeap.getCPUHandle(directionalLights[0].dsvDescriptorIndex));

	directionalLightCount++;
}

void Scene::updateDirectionalLightDirection(uint8 id, float32x3 direction)
{
	directionalLights[0].desc.direction = direction;
	directionalLights[0].shadowVolumeOrigin = VectorMath::Normalize(direction) * -16.0f;
}