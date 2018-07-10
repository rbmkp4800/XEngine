#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12GraphicsCommandList2;
struct ID3D12Resource;

namespace XLib { struct Matrix3x4; }
namespace XEngine::Render { class Device; }
namespace XEngine::Render::Device_ { class SceneRenderer; }

namespace XEngine::Render
{
	struct GeometryDesc // 20 bytes
	{
		BufferHandle vertexBufferHandle;
		BufferHandle indexBufferHandle;
		uint32 vertexDataOffset;
		uint32 indexDataOffset;
		uint32 indexCount;
		struct
		{
			uint indexIs32Bit : 1;
			uint vertexStride : 7;
			uint vertexDataSize : 24;
		};
	};

	class Scene : public XLib::NonCopyable
	{
		friend Device_::SceneRenderer;

	private:
		struct Instance // 28 bytes
		{
			GeometryDesc geometryDesc;
			uint32 baseTransformIndex;
			MaterialHandle material;
		};

		struct CachedInstance // 32 bytes
		{
			uint64 vertexDataGPUAddress;
			uint64 indexDataGPUAddress;
			uint32 indexCount;
			struct
			{
				uint indexIs32Bit : 1;
				uint vertexStride : 7;
				uint vertexDataSize : 24;
			};

			uint32 baseTransformIndex;
			MaterialHandle material;
		};

		struct EffectInstancesData
		{
			EffectHandle effect;
			XLib::Vector<CachedInstance> visibleInstances;
		};

	private:
		Device *device = nullptr;

		XLib::Vector<Instance> instances;
		XLib::Vector<EffectInstancesData> effectInstancesDatas;

		XLib::Platform::COMPtr<ID3D12Resource> d3dTransformBuffer;

		XLib::Matrix3x4 *mappedTransformBuffer = nullptr;
		uint32 transformBufferSize = 0;
		uint32 allocatedTansformCount = 0;

	private:
		void populateCommandList(ID3D12GraphicsCommandList2* d3dCommandList);

	public:
		Scene() = default;
		~Scene() = default;

		void initialize(Device& device, uint32 initialTransformBufferSize = 256);
		void destroy();

		TransformHandle createTransformGroup(uint32 transformCount);
		void releaseTransformGroup(TransformHandle handle);

		GeometryInstanceHandle createGeometryInstance(
			const GeometryDesc& geometryDesc, MaterialHandle material,
			uint32 transformCount = 1, const XLib::Matrix3x4* intialTransforms = nullptr);
		void removeGeometryInstance(GeometryInstanceHandle handle);

		void updateGeometryInstanceTransform(GeometryInstanceHandle handle,
			const XLib::Matrix3x4& transform, uint16 transformIndex = 0);
		void updateTransform(TransformHandle handle, const XLib::Matrix3x4& transform);
		void updateTransforms(TransformHandle handle, const XLib::Matrix3x4* transforms, uint32 transformCount);
	};
}