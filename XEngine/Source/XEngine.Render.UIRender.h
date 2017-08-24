#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>

// TODO: refactor. Remove this include
#include "XEngine.UI.Console.h"

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

namespace XEngine
{
	class XERMonospacedFont;
	class XERTargetBuffer;
	class XERDevice;

	/*namespace Internal
	{
		class UIGeometryBuffer : public XLib::NonCopyable
		{
		private:
			COMPtr<ID3D12Resource> d3dBuffer;

		public:

		};
	}*/

	class XERUIRender : public XLib::NonCopyable
	{
		friend XEUIConsole;

	private:
		enum class GeometryType : uint8
		{
			None = 0,
			Color,
			Font,
		};

		XERDevice *device = nullptr;

		COMPtr<ID3D12Resource> d3dVertexBuffer;
		void *mappedVertexBuffer = nullptr;

		ID3D12GraphicsCommandList *d3dCommandList = nullptr;
		ID3D12CommandAllocator *d3dCommandAllocator = nullptr;
		uint16 targetWidth = 0, targetHeight = 0;
		uint32 targetRTVDescriptor = 0;
		bool commandListInitialized = false;

		uint32 vertexBufferUsedBytes = 0;
		uint32 currentGeometryVertexBufferOffset = 0;
		uint32 currentGeometrySRVDescriptor = uint32(-1);
		GeometryType currentGeometryType = GeometryType::None;

		void flushCurrentGeometry();
		void flushCommandList();
		void initCommandList();

		void* allocateVertexBuffer(uint32 size, GeometryType geometryType);
		void setSRVDescriptor(uint32 srvDescriptor);

	public:
		void initialize(XERDevice* device);

		void beginDraw(XERTargetBuffer* target);
		void endDraw();

		void drawText(XERMonospacedFont* font, float32x2 position,
			const char* text, uint32 length = uint32(-1));
		//void drawRectangle(...);
		//void drawLine(...);

		inline void drawConsole(XEUIConsole* console) { console->draw(this); }
		//void drawGUI(...);
	};
}