#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.System.Threading.Event.h>
#include <XLib.Platform.COMPtr.h>

enum D3D12_COMMAND_LIST_TYPE;
enum D3D12_DESCRIPTOR_HEAP_TYPE;
enum DXGI_FORMAT;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

struct IDXGIFactory5;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12CommandList;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12CommandSignature;
struct ID3D12Resource;
struct ID3D12QueryHeap;

/*
	effects PSO - default graphics RS
		VS:
			CBV: camera constants
			Constants: instance constants
			SRV: transforms buffer
		PS:
			Table: diffuse textures
			Sampler: default sampler

	lighting pass PSO - lighting pass RS
		PS:
			...

	depth buffer downscale PSO - lighting pass RS

	OC bbox draw PSO - default graphics RS
		VS:
			CBV: camera constants
			SRV: transforms buffer
		PS:
			UAV: geometry instances visibility flags
*/

namespace XEngine
{
	class XEREffect;
	class XERScene;
	class XERGeometry;
	class XERUIGeometryBuffer;
	class XERTexture;
	class XERMonospacedFont;
	class XERWindowTarget;
	class XERSceneRender;
	class XERUIRender;

	class XERDevice : public XLib::NonCopyable
	{
		friend XEREffect;
		friend XERScene;
		friend XERGeometry;
		friend XERUIGeometryBuffer;
		friend XERTexture;
		friend XERMonospacedFont;
		friend XERWindowTarget;
		friend XERSceneRender;
		friend XERUIRender;

	private:
		class GPUQueue
		{
			friend XERDevice;

		private:
			XLib::Platform::COMPtr<ID3D12CommandQueue> d3dCommandQueue;
			XLib::Platform::COMPtr<ID3D12Fence> d3dFence;
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
			GPUQueue copyGPUQueue;
			XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
			XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;
			XLib::Platform::COMPtr<ID3D12Resource> d3dUploadBuffer;
			byte *mappedUploadBuffer = nullptr;

			uint32 uploadBufferBytesUsed = 0;

			ID3D12Resource *d3dLastBufferUploadResource = nullptr;
			uint64 lastBufferUploadDestOffset = 0;
			uint32 lastBufferUploadSize = 0;
			bool commandListDirty = false;

			void initalize(ID3D12Device* d3dDevice);

			void uploadTexture2DMIPLevel(DXGI_FORMAT format, ID3D12Resource* d3dDstTexture,
				uint16 mipLevel, uint16 width, uint16 height, uint16 pixelPitch,
				uint32 sourceRowPitch, const void* sourceData);

			void flushCommandList();
			void flushLastBufferUploadToCommandList();

		public:
			void flush();

			//void uploadTexture2D();
			//void uploadTexture2DRegion();
			void uploadTexture2DAndGenerateMIPs(ID3D12Resource* d3dDstTexture, const void* sourceData,
				uint32 sourceRowPitch, void* mipsGenerationBuffer);

			//void uploadTexture3D();
			void uploadTexture3DRegion(ID3D12Resource* d3dDstTexture,
				uint32 dstLeft, uint32 dstTop, uint32 dstFront,
				const void* sourceData, uint32 width, uint32 height, uint32 depth,
				uint32 sourceRowPitch, uint32 sourceSlicePitch);
			//void uploadTexture3DAndGenerateMIPs();

			void uploadBuffer(ID3D12Resource* d3dDestBuffer, uint32 destOffset, const void* data, uint32 size);
		};

		class DescriptorHeap
		{
			friend XERDevice;

		private:
			XLib::Platform::COMPtr<ID3D12DescriptorHeap> d3dDescriptorHeap;
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

		class GraphicsCommandListPool // temporary implementation
		{
			friend XERDevice;

		private:
			XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
			XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;
			bool acquired = false;

			void initialize(ID3D12Device* d3dDevice);

		public:
			bool acquireDefault(ID3D12GraphicsCommandList*& d3dCommandList,
				ID3D12CommandAllocator*& d3dCommandAllocator);
			void releaseDefault();
		};

		//===================================================================================//

		static XLib::Platform::COMPtr<IDXGIFactory5> dxgiFactory;
		XLib::Platform::COMPtr<ID3D12Device> d3dDevice;

		XLib::Platform::COMPtr<ID3D12RootSignature> d3dDefaultGraphicsRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dDefaultComputeRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dLightingPassRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dUIPassRS;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dLightingPassPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dDebugWireframePSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dUIColorPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dUIFontPSO;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dClearDefaultUAVxPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dDepthBufferDownscalePSO;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dOCxBBoxDrawPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dOCxICLUpdatePSO;

		XLib::Platform::COMPtr<ID3D12CommandSignature> d3dDefaultDrawingICS;

		XLib::Platform::COMPtr<ID3D12Resource> d3dReadbackBuffer;
		XLib::Platform::COMPtr<ID3D12QueryHeap> d3dTimestampQueryHeap;

		DescriptorHeap rtvHeap;
		DescriptorHeap dsvHeap;
		DescriptorHeap srvHeap;
		GPUQueue gpuGraphicsQueue;
		UploadEngine uploadEngine;
		GraphicsCommandListPool graphicsCommandListPool;

		char name[32];
		float32 gpuTickPeriod;

	public:
		bool initialize();

		inline const char* getName() { return name; }
	};
}