#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Debug.h>

#include "XEngine.Render.Scene.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Geometry.h"
#include "XEngine.Render.Effect.h"
#include "XEngine.Render.Internal.GPUStructs.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Internal;

static constexpr uint32 initialGeometryInstancesBufferSize = 65536;
static constexpr uint32 initialTransformsBufferSize = 65536;
static constexpr uint32 initialCommandsBufferSize = 65536;

void XERScene::initialize(XERDevice* device)
{
	this->device = device;

	geometryInstancesBufferSize = initialGeometryInstancesBufferSize;
	transformsBufferSize = initialTransformsBufferSize;
	geometryInstanceCount = 0;
	effectCount = 0;
	lightCount = 0;

	ID3D12Device *d3dDevice = device->d3dDevice;
	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(GPUGeometryInstanceDesc) * geometryInstancesBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dGeometryInstacesBuffer.uuid(), d3dGeometryInstacesBuffer.voidInitRef());
	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(Matrix3x4) * transformsBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dTransformsBuffer.uuid(), d3dTransformsBuffer.voidInitRef());
 	d3dGeometryInstacesBuffer->Map(0, &D3D12Range(), to<void**>(&mappedGeometryInstacesBuffer));
	d3dTransformsBuffer->Map(0, &D3D12Range(), to<void**>(&mappedTransformsBuffer));
}

XERGeometryInstanceId XERScene::createGeometryInstance(XERGeometry* geometry, XEREffect* effect, const Matrix3x4& transform)
{
	EffectData *effectData = nullptr;

	for (uint32 i = 0; i < effectCount; i++)
	{
		if (effectsDataList[i].effect == effect)
		{
			effectData = &effectsDataList[i];
			break;
		}
	}

	if (!effectData)
	{
		Debug::CrashCondition(effectCount >= countof(effectsDataList), DbgMsgFmt("effects limit exceeded"));

		effectData = &effectsDataList[effectCount];
		effectCount++;

		effectData->effect = effect;
		effectData->commandsBufferSize = initialCommandsBufferSize;
		effectData->commandCount = 0;

		device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(effectData->commandsBufferSize * sizeof(GPUDefaultDrawingIC),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			effectData->d3dCommandsBuffer.uuid(), effectData->d3dCommandsBuffer.voidInitRef());
	}
	
	mappedTransformsBuffer[geometryInstanceCount] = transform;

	XERGeometryInstanceId id = geometryInstanceCount;
	D3D12_GPU_VIRTUAL_ADDRESS geometryBufferAddress = geometry->d3dBuffer->GetGPUVirtualAddress();
	uint32 vertexBufferSize = geometry->vertexCount * geometry->vertexStride;

	GPUDefaultDrawingIC command;
	command.vertexBufferView.BufferLocation = geometryBufferAddress;
	command.vertexBufferView.SizeInBytes = vertexBufferSize;
	command.vertexBufferView.StrideInBytes = geometry->vertexStride;
	command.indexBufferView.BufferLocation = geometryBufferAddress + vertexBufferSize;
	command.indexBufferView.SizeInBytes = geometry->indexCount * sizeof(uint32);
	command.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	command.drawIndexedArguments.IndexCountPerInstance = geometry->indexCount;
	command.drawIndexedArguments.InstanceCount = 1;
	command.drawIndexedArguments.StartIndexLocation = 0;
	command.drawIndexedArguments.BaseVertexLocation = 0;
	command.drawIndexedArguments.StartInstanceLocation = id;
	command.geometryInstanceId = id;

	device->uploadEngine.uploadBuffer(effectData->d3dCommandsBuffer,
		effectData->commandCount * sizeof(GPUDefaultDrawingIC),
		&command, sizeof(GPUDefaultDrawingIC));

	geometryInstanceCount++;
	effectData->commandCount++;

	return id;
}

XERLightId XERScene::createLight(float32x3 position, XEColor color, float32 intensity)
{
	LightDesc &desc = lights[lightCount];
	desc.position = position;
	desc.color = color;
	desc.intensity = intensity;

	XERLightId id = lightCount;
	lightCount++;
	return id;
}

void XERScene::removeGeometryInstance(XERGeometryInstanceId id)
{

}

void XERScene::setGeometryInstanceTransform(XERGeometryInstanceId id, const Matrix3x4& transform)
{
	mappedTransformsBuffer[id] = transform;
}

void XERScene::fillD3DCommandList_draw(ID3D12GraphicsCommandList* d3dCommandList)
{
	d3dCommandList->IASetVertexBuffers(1, 1,
		&D3D12VertexBufferView(d3dTransformsBuffer->GetGPUVirtualAddress(),
		geometryInstanceCount * sizeof(Matrix3x4), sizeof(Matrix3x4)));

	for (uint32 i = 0; i < effectCount; i++)
	{
		EffectData &effectData = effectsDataList[i];

		d3dCommandList->SetPipelineState(effectData.effect->d3dPipelineState);
		d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS,
			effectData.commandCount, effectData.d3dCommandsBuffer, 0, nullptr, 0);
	}
}

void XERScene::fillD3DCommandList_runOcclusionCulling(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12Resource *d3dTempBuffer, uint32 tempBufferSize, uint32 tempRTVDescriptorsBase)
{
	// temp buffer layout
	//   4 bytes - counter (must be aligned to D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT (4096))
	//   4 * geometryInstanceCount bytes - visibility flags
	//   remaining bytes - appeared geometry instances draw command list (aligned to size of command)

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	d3dCommandList->SetPipelineState(device->d3dClearDefaultUAVxPSO);
	d3dCommandList->SetComputeRootSignature(device->d3dDefaultComputeRS);
	d3dCommandList->SetComputeRootUnorderedAccessView(2, d3dTempBuffer->GetGPUVirtualAddress());
	d3dCommandList->Dispatch(tempBufferSize / (sizeof(uint32) * 1024), 1, 1);	// TODO: refactor that 1024

	d3dCommandList->SetPipelineState(device->d3dOCxBBoxDrawPSO);
	d3dCommandList->SetGraphicsRootShaderResourceView(1, d3dTransformsBuffer->GetGPUVirtualAddress());
	d3dCommandList->SetGraphicsRootUnorderedAccessView(3, d3dTempBuffer->GetGPUVirtualAddress() + sizeof(uint32));
	d3dCommandList->DrawInstanced(geometryInstanceCount * 36, 1, 0, 0);

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

	uint32 appearedGeometryInstancesDrawCommandsByteOffset =
		alignup<uint32>(sizeof(uint32) * geometryInstanceCount + sizeof(uint32), sizeof(GPUDefaultDrawingIC));
	uint32 appearedGeometryInstancesDrawCommandsStructOffset =
		appearedGeometryInstancesDrawCommandsByteOffset / sizeof(GPUDefaultDrawingIC);
	uint32 appearedGeometryInstancesDrawCommandsLimit =
		(tempBufferSize - appearedGeometryInstancesDrawCommandsByteOffset) / sizeof(GPUDefaultDrawingIC);

	device->d3dDevice->CreateUnorderedAccessView(d3dTempBuffer, d3dTempBuffer,
		&D3D12UnorderedAccessViewDesc_Buffer(
			appearedGeometryInstancesDrawCommandsStructOffset,
			appearedGeometryInstancesDrawCommandsLimit,
			sizeof(GPUDefaultDrawingIC), 0),
		device->srvHeap.getCPUHandle(tempRTVDescriptorsBase));

	for (uint32 i = 0; i < effectCount; i++)
	{
		EffectData &effectData = effectsDataList[i];

		d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		d3dCommandList->SetPipelineState(device->d3dOCxICLUpdatePSO);
		d3dCommandList->SetComputeRoot32BitConstant(0, effectData.commandCount, 0);
		d3dCommandList->SetComputeRootShaderResourceView(1, d3dTempBuffer->GetGPUVirtualAddress() + sizeof(uint32));
		d3dCommandList->SetComputeRootUnorderedAccessView(2, effectData.d3dCommandsBuffer->GetGPUVirtualAddress());
		d3dCommandList->SetComputeRootDescriptorTable(3, device->srvHeap.getGPUHandle(tempRTVDescriptorsBase));
		d3dCommandList->Dispatch((effectData.commandCount - 1) / 1024 + 1, 1, 1);

		d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		d3dCommandList->SetPipelineState(effectData.effect->d3dPipelineState);
		d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS, appearedGeometryInstancesDrawCommandsLimit,
			d3dTempBuffer, appearedGeometryInstancesDrawCommandsByteOffset, d3dTempBuffer, 0);
	}
}

void XERScene::fillD3DCommandList_drawWithoutEffects(ID3D12GraphicsCommandList* d3dCommandList)
{
	d3dCommandList->IASetVertexBuffers(1, 1,
		&D3D12VertexBufferView(d3dTransformsBuffer->GetGPUVirtualAddress(),
		geometryInstanceCount * sizeof(Matrix3x4), sizeof(Matrix3x4)));

	for (uint32 i = 0; i < effectCount; i++)
	{
		EffectData &effectData = effectsDataList[i];
		d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS,
			effectData.commandCount, effectData.d3dCommandsBuffer, 0, nullptr, 0);
	}
}