#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.System.Threading.Event.h>

enum D3D12_COMMAND_LIST_TYPE;
enum D3D12_DESCRIPTOR_HEAP_TYPE;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

struct IDXGIFactory5;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandList;
struct ID3D12DescriptorHeap;
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12CommandSignature;
struct ID3D12Resource;
struct ID3D12QueryHeap;

namespace XEngine
{
	class XEREffect;
	class XERGeometry;
	class XERScene;
	class XERMonospacedFont;
	class XERUIGeometryRenderer;
	class XERWindowTarget;
	class XERContext;

	class XERDevice : public XLib::NonCopyable
	{
		friend XEREffect;
		friend XERGeometry;
		friend XERScene;
		friend XERMonospacedFont;
		friend XERUIGeometryRenderer;
		friend XERWindowTarget;
		friend XERContext;

	private:
		class GPUQueue
		{
			friend XERDevice;

		private:
			COMPtr<ID3D12CommandQueue> d3dCommandQueue;
			COMPtr<ID3D12Fence> d3dFence;
			XLib::Event fenceSyncEvent;
			uint64 fenceValue = 0;

			void initialize(ID3D12Device* d3dDevice, D3D12_COMMAND_LIST_TYPE type);
			void synchronize();

		public:
			void execute(ID3D12CommandList** d3dCommandLists, uint32 count);
			uint64 getTimestampFrequency();
			inline ID3D12CommandQueue* getD3DCommandQueue() { return d3dCommandQueue; }
		};

		class UploadEngine
		{
			friend XERDevice;

		private:
			static constexpr uint32 uploadBufferSize = 0x10000;

			GPUQueue copyGPUQueue;
			COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
			COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;
			COMPtr<ID3D12Resource> d3dUploadBuffer;
			void *mappedUploadBuffer;

			void initalize(ID3D12Device* d3dDevice);

		public:
			void uploadTexture(ID3D12Resource* d3dTexture, const uint8* sourceBitmap, uint32 width, uint32 height);
			void uploadBuffer(ID3D12Resource* d3dDestBuffer, uint32 destOffset, const void* sourceData, uint32 size);
		};

		class DescriptorHeap
		{
			friend XERDevice;

		private:
			COMPtr<ID3D12DescriptorHeap> d3dDescriptorHeap;
			uint32 allocatedDescriptorsCount;
			uint32 descritorSize;

			void initalize(ID3D12Device* d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 size, bool shaderVisible);

		public:
			uint32 allocateDescriptors(uint32 count);
			//void releaseDescriptor(uint32 descriptor);

			D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint32 descriptor);
			D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint32 descriptor);
			inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() { return d3dDescriptorHeap; }
		};

		static COMPtr<IDXGIFactory5> dxgiFactory;
		COMPtr<ID3D12Device> d3dDevice;

		COMPtr<ID3D12RootSignature> d3dDefaultGraphicsRS;
		COMPtr<ID3D12RootSignature> d3dDefaultComputeRS;
		COMPtr<ID3D12RootSignature> d3dLightingPassRS;

		COMPtr<ID3D12PipelineState> d3dLightingPassPSO;
		COMPtr<ID3D12PipelineState> d3dDebugWireframePSO;
		COMPtr<ID3D12PipelineState> d3dUIFontPSO;

		COMPtr<ID3D12PipelineState> d3dClearDefaultUAVxPSO;

		COMPtr<ID3D12PipelineState> d3dOCxBBoxDrawPSO;
		COMPtr<ID3D12PipelineState> d3dOCxICLUpdatePSO;

		COMPtr<ID3D12CommandSignature> d3dDefaultDrawingICS;

		COMPtr<ID3D12Resource> d3dReadbackBuffer;
		COMPtr<ID3D12QueryHeap> d3dTimestampQueryHeap;

		DescriptorHeap rtvHeap;
		DescriptorHeap dsvHeap;
		DescriptorHeap srvHeap;
		GPUQueue graphicsGPUQueue;
		GPUQueue copyGPUQueue;
		UploadEngine uploadEngine;

		char name[32];
		float32 gpuTickPeriod;

	public:
		bool initialize();

		inline const char* getName() { return name; }
	};
}