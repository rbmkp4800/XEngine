#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;

namespace XEngine
{
	class XERScene;
	class XERUIRender;
	class XERDevice;

	class XERGeometry : public XLib::NonCopyable
	{
		friend XERScene;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dBuffer;
		uint32 vertexCount = 0, indexCount = 0;
		uint16 vertexStride = 0;
		uint16 transformsCount = 0;

	public:
		void initialize(XERDevice* device, const void* vertices, uint32 vertexCount,
			uint32 vertexStride, const uint32* indices, uint32 indexCount);
	};

	class XERUIGeometryBuffer : public XLib::NonCopyable
	{
		friend XERUIRender;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dBuffer;

	public:
		void initialize(XERDevice* device, uint32 size);
		void update(XERDevice* device, uint32 destOffset, const void* data, uint32 size);
	};

	class XERTexture : public XLib::NonCopyable
	{
		friend XERScene;
		friend XERUIRender;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;

	public:
		void initialize(XERDevice* device, const void* data, uint32 width, uint32 height);
	};

	class XERMonospacedFont : public XLib::NonCopyable
	{
		friend XERUIRender;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;
		uint8 charWidth, charHeight;
		uint8 firstCharCode, charTableSize;

	public:
		void initializeA8(XERDevice* device, uint8* bitmapA8, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);
		void initializeA1(XERDevice* device, uint8* bitmapA1, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);

		inline uint8 getCharWidth() { return charWidth; }
		inline uint8 getCharHeight() { return charHeight; }
		inline uint8 getFirstCharCode() { return firstCharCode; }
		inline uint8 getCharTableSize() { return charTableSize; }
	};
}