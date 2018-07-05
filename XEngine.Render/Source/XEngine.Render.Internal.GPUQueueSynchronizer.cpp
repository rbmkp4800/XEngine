#include <d3d12.h>

#include "XEngine.Render.Internal.GPUQueueSynchronizer.h"

using namespace XEngine::Render::Internal;

void GPUQueueSynchronizer::initialize(ID3D12Device* d3dDevice)
{
	d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, d3dFence.uuid(), d3dFence.voidInitRef());
	fenceSyncEvent.initialize(false, false);
	fenceValue = 1;
}

void GPUQueueSynchronizer::destroy()
{

}

void GPUQueueSynchronizer::synchronize(ID3D12CommandQueue* d3dQueue)
{
	d3dQueue->Signal(d3dFence, fenceValue);
	if (d3dFence->GetCompletedValue() < fenceValue)
	{
		d3dFence->SetEventOnCompletion(fenceValue, fenceSyncEvent.getHandle());
		fenceSyncEvent.wait();
	}

	fenceValue++;
}