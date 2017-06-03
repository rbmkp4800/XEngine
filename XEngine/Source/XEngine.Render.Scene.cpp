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

XERGeometryInstanceId XERScene::createGeometryInstance(XERGeometry* geometry, XEREffect* effect, const Matrix3x4& _transform)
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

		device->d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(effectData->commandsBufferSize * sizeof(GPUDefaultDrawingIndirectCommand)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			effectData->d3dCommandsBuffer.uuid(), effectData->d3dCommandsBuffer.voidInitRef());
		effectData->d3dCommandsBuffer->Map(0, &D3D12Range(), to<void**>(&effectData->mappedCommandsBuffer));
	}
	
	GPUDefaultDrawingIndirectCommand &command = effectData->mappedCommandsBuffer[effectData->commandCount];
	Matrix3x4 &transform = mappedTransformsBuffer[geometryInstanceCount];

	D3D12_GPU_VIRTUAL_ADDRESS geometryBufferAddress = geometry->d3dBuffer->GetGPUVirtualAddress();
	uint32 vertexBufferSize = geometry->vertexCount * geometry->vertexStride;

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
	command.drawIndexedArguments.StartInstanceLocation = geometryInstanceCount;
	transform = _transform;

	effectData->commandCount++;

	XERGeometryInstanceId id = geometryInstanceCount;
	geometryInstanceCount++;

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

void XERScene::fillD3DCommandList(ID3D12GraphicsCommandList* d3dCommandList, bool applyEffects)
{
	d3dCommandList->IASetVertexBuffers(1, 1,
		&D3D12VertexBufferView(d3dTransformsBuffer->GetGPUVirtualAddress(),
		geometryInstanceCount * sizeof(Matrix3x4), sizeof(Matrix3x4)));

	for (uint32 i = 0; i < effectCount; i++)
	{
		EffectData &effectData = effectsDataList[i];
		
		if (applyEffects)
			d3dCommandList->SetPipelineState(effectData.effect->d3dPipelineState);
		d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS,
			effectData.commandCount, effectData.d3dCommandsBuffer, 0, nullptr, 0);
	}
}