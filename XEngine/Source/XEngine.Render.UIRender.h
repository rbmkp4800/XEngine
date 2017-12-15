#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

namespace XEngine
{
	class XERUIGeometryBuffer;
	class XERTexture;
	class XERMonospacedFont;
	class XERTargetBuffer;
	class XERDevice;

	enum class XERUIGeometryType : uint8
	{
		None = 0,
		Color,
		Font,
	};

	class XERUIRender : public XLib::NonCopyable
	{
	private:
		XERDevice *device = nullptr;

		XLib::Platform::COMPtr<ID3D12Resource> d3dVertexBuffer;
		void *mappedVertexBuffer = nullptr;

		ID3D12GraphicsCommandList *d3dCommandList = nullptr;
		ID3D12CommandAllocator *d3dCommandAllocator = nullptr;
		uint16 targetWidth = 0, targetHeight = 0;
		float32x2 ndcScale = { 0.0f, 0.0f };
		uint32 targetRTVDescriptor = 0;
		bool commandListInitialized = false;

		uint32 vertexBufferUsedBytes = 0;
		uint32 currentGeometryVertexBufferOffset = 0;
		uint32 currentGeometrySRVDescriptor = uint32(-1);
		XERUIGeometryType currentGeometryType = XERUIGeometryType::None;

		void drawVertexBuffer(ID3D12Resource *d3dVertexBufferToDraw,
			uint32 offset, uint32 size, XERUIGeometryType geometryType);
		void flushCurrentGeometry();
		void flushCommandList();
		void initCommandList();

	public:
		void initialize(XERDevice* device);

		void beginDraw(XERTargetBuffer* target);
		void endDraw();

		void* allocateVertices(uint32 size, XERUIGeometryType geometryType);
		void drawBuffer(XERUIGeometryBuffer* buffer, uint32 offset, uint32 size, XERUIGeometryType geometryType);
		void setTexture(XERTexture* texture);
		void setTexture(XERMonospacedFont* fontTexture);

		void drawText(XERMonospacedFont* font, float32x2 position,
			const char* text, uint32 color, uint32 length = uint32(-1));
		void drawStackedBarChart(float32x2 position, float32 height,
			float32 horizontalScale, float32* values, uint32* colors, uint32 valueCount);
		void fillRectangle(rectf32 rect, uint32 color);
		//void drawLine(...);

		inline XERDevice* getDevice() { return device; }

		inline uint16 getTargetWidth() { return targetWidth; }
		inline uint16 getTargetHeight() { return targetHeight; }
		inline float32 getNDCHorizontalScale() { return ndcScale.x; }
		inline float32 getNDCVerticalScale() { return ndcScale.y; }
		inline float32x2 getNDCScale() { return ndcScale; }
	};
}