#include <d3d12.h>

#include <XLib.Math.Matrix3x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"

#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;

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


}

void Scene::destroy()
{

}

GeometryInstanceHandle Scene::createGeometryInstance(
	const GeometryDesc& geometryDesc, MaterialHandle material,
	uint32 transformCount, const XLib::Matrix3x4* intialTransforms)
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

		EffectInstancesData *effectInstancesData = nullptr;
		for (EffectInstancesData& data : effectInstancesDatas)
		{
			if (data.effect == effect)
			{
				effectInstancesData = &data;
				break;
			}
		}

		if (!effectInstancesData)
			effectInstancesData = &effectInstancesDatas.allocateBack();

		CachedInstance &instance = effectInstancesData->visibleInstances.allocateBack();
		uint64 vbAddress = device->bufferHeap.getBufferGPUAddress(geometryDesc.vertexBufferHandle);
		uint64 ibAddress = device->bufferHeap.getBufferGPUAddress(geometryDesc.indexBufferHandle);
		instance.vertexDataGPUAddress = vbAddress + geometryDesc.vertexDataOffset;
		instance.indexDataGPUAddress = ibAddress + geometryDesc.indexDataOffset;
		instance.indexCount = geometryDesc.indexCount;
		instance.indexIs32Bit = geometryDesc.indexIs32Bit;
		instance.vertexStride = geometryDesc.vertexStride;
		instance.vertexDataSize = geometryDesc.vertexDataSize;
		instance.baseTransformIndex = baseTransformIndex;
		instance.material = material;
	}

	return result;
}

void Scene::populateCommandList(ID3D12GraphicsCommandList2* d3dCommandList)
{
	uint64 transformBufferGPUAddress = d3dTransformBuffer->GetGPUVirtualAddress();

	for (EffectInstancesData& effectInstancesData : effectInstancesDatas)
	{
		ID3D12PipelineState *d3dPSO = device->effectHeap.getD3DPSO(effectInstancesData.effect);
		d3dCommandList->SetPipelineState(d3dPSO);

		for (CachedInstance& instance : effectInstancesData.visibleInstances)
		{
			d3dCommandList->IASetVertexBuffers(0, 1,
				&D3D12VertexBufferView(instance.vertexDataGPUAddress,
					instance.vertexDataSize, instance.vertexStride));

			d3dCommandList->IASetIndexBuffer(
				&D3D12IndexBufferView(instance.indexDataGPUAddress,
					instance.indexCount * (instance.indexIs32Bit ? 4 : 2),
					instance.indexIs32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT));

			// TODO: check performace. Maybe replace with setting single constant and fixed
			//	transforms buffer SRV.
			//	https://gpuopen.com/performance-root-signature-descriptor-sets/
			d3dCommandList->SetGraphicsRootShaderResourceView(0,
				transformBufferGPUAddress + sizeof(Matrix3x4) * instance.baseTransformIndex);
			d3dCommandList->SetGraphicsRootConstantBufferView(1,
				device->materialHeap.translateHandleToGPUAddress(instance.material));

			d3dCommandList->DrawIndexedInstanced(instance.indexCount, 1, 0, 0, 0);
		}
	}
}