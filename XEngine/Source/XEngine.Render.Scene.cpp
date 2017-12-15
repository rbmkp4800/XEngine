#include <d3d12.h>

#include <XLib.Debug.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Resources.h"
#include "XEngine.Render.Internal.GPUStructs.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Internal;

static constexpr uint32 initialGeometryInstancesBufferSize = 65536;
static constexpr uint32 initialTransformsBufferSize = 65536;
static constexpr uint32 initialBVHBufferSize = 65536;
static constexpr uint32 initialCommandsBufferSize = 65536;

void XERScene::initialize(XERDevice* device)
{
	this->device = device;

	geometryInstancesBufferSize = initialGeometryInstancesBufferSize;
	transformsBufferSize = initialTransformsBufferSize;
	bvhBufferSize = initialBVHBufferSize;
	geometryInstanceCount = 0;
	bvhNodeCount = 0;
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
	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(GPUxBVHNode) * bvhBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dBVHBuffer.uuid(), d3dBVHBuffer.voidInitRef());

 	d3dGeometryInstacesBuffer->Map(0, &D3D12Range(), to<void**>(&mappedGeometryInstacesBuffer));
	d3dTransformsBuffer->Map(0, &D3D12Range(), to<void**>(&mappedTransformsBuffer));
	d3dBVHBuffer->Map(0, &D3D12Range(), to<void**>(&mappedBVHBuffer));
}

XERGeometryInstanceId XERScene::createGeometryInstance(XERGeometry* geometry,
	XEREffect* effect, const Matrix3x4& transform, XERTexture* texture)
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
	command.textureId = texture ? texture->srvDescriptor : uint32(-1);
	command.drawIndexedArguments.IndexCountPerInstance = geometry->indexCount;
	command.drawIndexedArguments.InstanceCount = 1;
	command.drawIndexedArguments.StartIndexLocation = 0;
	command.drawIndexedArguments.BaseVertexLocation = 0;
	command.drawIndexedArguments.StartInstanceLocation = id;
	command.geometryInstanceId = id;

	{
		GPUxBVHNode &bvhNode = mappedBVHBuffer[bvhNodeCount];
		bvhNodeCount++;

		float32x3 convexHullVertices[] =
		{
			{ -1.0f, -1.0f, -1.0f },
			{ -1.0f, -1.0f,  1.0f },
			{ -1.0f,  1.0f, -1.0f },
			{ -1.0f,  1.0f,  1.0f },

			{  1.0f, -1.0f, -1.0f },
			{  1.0f, -1.0f,  1.0f },
			{  1.0f,  1.0f, -1.0f },
			{  1.0f,  1.0f,  1.0f },
		};

		for each (float32x3& vertex in convexHullVertices)
			vertex = vertex * transform;

		bvhNode.bboxMin = convexHullVertices[0];
		bvhNode.bboxMax = convexHullVertices[0];

		for (uint32 i = 1; i < countof(convexHullVertices); i++)
		{
			bvhNode.bboxMin.x = { min(bvhNode.bboxMin.x, convexHullVertices[i].x) };
			bvhNode.bboxMin.y = { min(bvhNode.bboxMin.y, convexHullVertices[i].y) };
			bvhNode.bboxMin.z = { min(bvhNode.bboxMin.z, convexHullVertices[i].z) };

			bvhNode.bboxMax.x = { max(bvhNode.bboxMax.x, convexHullVertices[i].x) };
			bvhNode.bboxMax.y = { max(bvhNode.bboxMax.y, convexHullVertices[i].y) };
			bvhNode.bboxMax.z = { max(bvhNode.bboxMax.z, convexHullVertices[i].z) };
		}
	}

	device->uploadEngine.uploadBuffer(effectData->d3dCommandsBuffer,
		effectData->commandCount * sizeof(GPUDefaultDrawingIC),
		&command, sizeof(GPUDefaultDrawingIC));

	geometryInstanceCount++;
	effectData->commandCount++;

	return id;
}

XERLightId XERScene::createLight(float32x3 position, Color color, float32 intensity)
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