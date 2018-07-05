#pragma once

#include <XLib.NonCopyable.h>
#include <XLib.System.Threading.Event.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Device;
struct ID3D12Fence;
struct ID3D12CommandQueue;

namespace XEngine::Render::Internal
{
	class GPUQueueSynchronizer : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12Fence> d3dFence;
		XLib::Event fenceSyncEvent;
		uint64 fenceValue;

	public:
		GPUQueueSynchronizer() = default;
		~GPUQueueSynchronizer() = default;

		void initialize(ID3D12Device* d3dDevice);
		void destroy();

		void synchronize(ID3D12CommandQueue* d3dQueue);
	};
}