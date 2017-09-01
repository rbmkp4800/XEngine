#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

namespace XEngine
{
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

	class UIGeometryBuffer : public XLib::NonCopyable
	{
	private:
		COMPtr<ID3D12Resource> d3dBuffer;

	public:
		void initialize(XERDevice* device, uint32 size);
	};

	class XERUIRender : public XLib::NonCopyable
	{
	private:
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
		XERUIGeometryType currentGeometryType = XERUIGeometryType::None;

		void flushCurrentGeometry();
		void flushCommandList();
		void initCommandList();

	public:
		void initialize(XERDevice* device);

		void beginDraw(XERTargetBuffer* target);
		void endDraw();

		void* allocateVertices(uint32 size, XERUIGeometryType geometryType);
		void setTexture(XERTexture* texture);
		void setTexture(XERMonospacedFont* fontTexture);

		void drawText(XERMonospacedFont* font, float32x2 position,
			const char* text, uint32 color, uint32 length = uint32(-1));
		void drawStackedBarChart(float32x2 position, float32 height,
			float32 horizontalScale, float32* values, uint32 valueCount);
		//void drawRectangle(...);
		//void drawLine(...);

		inline float32x2 getNDCScaleCoef() { return float32x2(1.0f / float32(targetWidth / 2), -1.0f / float32(targetHeight / 2)); }
	};
}