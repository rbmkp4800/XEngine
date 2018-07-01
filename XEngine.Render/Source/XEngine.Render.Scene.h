#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Math.Matrix3x4.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12GraphicsCommandList2;
struct ID3D12Resource;

namespace XEngine::Render { class Device; }
namespace XEngine::Render::Device_ { class SceneRenderer; }

namespace XEngine::Render
{
    class Scene : public XLib::NonCopyable
    {
		friend Device_::SceneRenderer;

	private:
		struct InstancesTableEntry
		{
			uint32 baseTransformIndex;

			BufferHandle vertexBufferHandle;
			uint32 vertexDataOffset;
			BufferHandle indexBufferHandle;
			uint32 indexDataOffset;

			uint8 vertexStride;
			bool indexIs32Bit;
			MaterialHandle material;

			uint32 indexCount;
		};

		struct GeometryInstanceRecord
		{
			uint64 vertexBufferGPUAddress;
			uint64 indexBufferGPUAddress;
			uint32 baseTransformIndex;
			MaterialHandle material;
			uint8 vertexStide;
			bool indexIs32Bit;
		};

		struct EffectData
		{
			EffectHandle effect;
			XLib::Vector<GeometryInstanceRecord> visibleGeometryInstances;
		};

	private:
		Device *device = nullptr;

		XLib::Vector<InstancesTableEntry> instancesTable;
		XLib::Vector<EffectData> effectsData;

		XLib::Platform::COMPtr<ID3D12Resource> d3dTransformBuffer;

		uint32 transformBufferSize = 0;

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
			BufferHandle vertexBufferHandle, uint32 vertexDataOffset,
			BufferHandle indexBufferHandle, uint32 indexDataOffset,
			uint8 vertexStride, bool indexIs32Bit, uint32 indexCount, MaterialHandle material,
			uint32 transformCount = 1, const XLib::Matrix3x4* intialTransforms = nullptr);
		void removeGeometryInstance(GeometryInstanceHandle handle);

		void updateTransform(TransformHandle handle, const XLib::Matrix3x4& transform);
		void updateTransform(GeometryInstanceHandle handle,
			const XLib::Matrix3x4& transform, uint16 transformIndex = 0);
		void updateTransforms(TransformHandle handle, const XLib::Matrix3x4* transforms, uint32 transformCount);
    };
}