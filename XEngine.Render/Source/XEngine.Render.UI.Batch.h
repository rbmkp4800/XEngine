#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Vertices.h"

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

namespace XEngine::Render { class Device; }
namespace XEngine::Render { class Target; }
namespace XEngine::Render::UI { class Texture; }

namespace XEngine::Render::UI
{
	enum class GeometryType : uint8
	{
		None = 0,
		Color,
		ColorTexture,
	};

	class Batch : public XLib::NonCopyable
	{
		friend Device;

	private:
		Device *device = nullptr;

		XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
		XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;
		XLib::Platform::COMPtr<ID3D12Resource> d3dVertexBuffer;

		byte *mappedVertexBuffer = nullptr;
		uint32 vertexBufferSize = 0;

		Target *target = nullptr;
		uint32 vertexBufferUsedBytes = 0;
		uint32 currentGeometryVertexBufferOffset = 0;
		GeometryType currentGeometryType = GeometryType::None;
		uint8 currentGeometryVertexStride = 0;

	private:
		void flushCurrentGeometry();

	public:
		Batch() = default;
		~Batch() = default;

		void initialize(Device& device);
		void destroy();

		void beginDraw(Target& target, rectu16 viewport);
		void endDraw(bool finalizeTarget);

		void setTexture(Texture& texture);

		void* allocateVertices(GeometryType type, uint32 count);
	};
}