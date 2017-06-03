#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;

namespace XEngine
{
	class XERUIGeometryRenderer;
	class XERContext;
	class XERDevice;

	class XERMonospacedFont : public XLib::NonCopyable
	{
		friend XERUIGeometryRenderer;

	private:
		COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;
		uint8 charWidth, charHeight;
		uint8 firstCharCode, charTableSize;

	public:
		void initializeA8(XERDevice* device, uint8* bitmapA8, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);
		void initializeA1(XERDevice* device, uint8* bitmapA1, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);
	};

	class XERUIGeometryRenderer : public XLib::NonCopyable
	{
		friend XERContext;

	private:
		//enum class UIGeometryType : uint8;

		XERDevice *device = nullptr;
		XERContext *context = nullptr;

		COMPtr<ID3D12Resource> d3dVertexBuffer;
		void* mappedVertexBuffer = nullptr;

		XERMonospacedFont *font;
		uint32 usedVertexCount = 0;

		void flush();

		void fillD3DCommandList(ID3D12GraphicsCommandList *d3dCommandList);

	public:
		void initialize(XERDevice* _device);

		void beginDraw(XERContext* _context);
		void endDraw();

		inline void setFont(XERMonospacedFont* _font) { font = _font; }
		void drawText(float32x2 position, const char* text, uint32 length = uint32(-1));
	};
}