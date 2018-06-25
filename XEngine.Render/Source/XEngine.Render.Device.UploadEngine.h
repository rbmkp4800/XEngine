#pragma once

namespace XEngine::Render::Device_
{
	class UploadEngine
	{
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

	private:
		void initalize(ID3D12Device* d3dDevice);

		void uploadTexture2DMIPLevel(DXGI_FORMAT format, ID3D12Resource* d3dDstTexture,
			uint16 mipLevel, uint16 width, uint16 height, uint16 pixelPitch,
			uint32 sourceRowPitch, const void* sourceData);

		void flushCommandList();
		void flushLastBufferUploadToCommandList();

	public:
		UploadEngine() = default;
		~UploadEngine() = default;

		void flush();

		void uploadTexture2DAndGenerateMIPs(ID3D12Resource* d3dDstTexture, const void* sourceData,
			uint32 sourceRowPitch, void* mipsGenerationBuffer);

		void uploadTexture3DRegion(ID3D12Resource* d3dDstTexture,
			uint32 dstLeft, uint32 dstTop, uint32 dstFront,
			const void* sourceData, uint32 width, uint32 height, uint32 depth,
			uint32 sourceRowPitch, uint32 sourceSlicePitch);

		void uploadBuffer(ID3D12Resource* d3dDestBuffer, uint32 destOffset, const void* data, uint32 size);
	};
}