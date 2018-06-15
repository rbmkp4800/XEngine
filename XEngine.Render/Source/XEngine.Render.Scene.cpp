#include <d3d12.h>

#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.Internal.RootSignaturesConfig.h"

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;

void Scene::initialize(Device& device, uint32 initialTransformBufferSize = 256)
{
	this->device = &device;

	transformBufferSize = initialTransformBufferSize;

	ID3D12Device *d3dDevice = ;
	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(Matrix3x4) * transformBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dTransformBuffer.uuid(), d3dTransformBuffer.voidInitRef());


}

void Scene::destroy()
{

}

void Scene::populateCommandList(ID3D12GraphicsCommandList2* d3dCommandList)
{
	uint64 transformBufferGPUAddress = d3dTransformBuffer->GetGPUVirtualAddress();

	for (EffectData& effectData : effectsData)
	{
		ID3D12PipelineState *d3dPSO = device->getEffectHeap().getPSO(effectData.effect);
		d3dCommandList->SetPipelineState(d3dPSO);

		for (GeometryInstanceRecord& record : effectData.visibleGeometryInstances)
		{
			d3dCommandList->IASetVertexBuffers(0, 1, &D3D12VertexBufferView());

			d3dCommandList->SetGraphicsRootConstantBufferView(
				RootSignaturesConfig::GBufferPass::TransformCBVIndex,
				transformBufferGPUAddress + sizeof(Matrix3x4) * record.baseTransformIndex);

			d3dCommandList->SetGraphicsRootConstantBufferView(
				RootSignaturesConfig::GBufferPass::MaterialCBVIndex,
				device->getMaterialHeap().translateHandleToGPUAddress(record.material));

			d3dCommandList->DrawIndexedInstanced();
		}
	}
}