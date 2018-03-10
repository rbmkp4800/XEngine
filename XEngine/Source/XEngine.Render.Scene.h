#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Math.Matrix3x4.h>
#include <XLib.Color.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;

namespace XEngine::Internal { struct GPUGeometryInstanceDesc; }
namespace XEngine::Internal { struct GPUxBVHNode; }

namespace XEngine
{
	class XEREffect;
	class XERGeometry;
	class XERTexture;
	class XERSceneRender;
	class XERDevice;

	using XERGeometryInstanceId = uint32;
	using XERLightId = uint32;

	struct LightDesc
	{
		float32x3 position;
		XLib::Color color;
		float32 intensity;
	};

	class XERScene : public XLib::NonCopyable
	{
		friend XERSceneRender;

	private:
		struct EffectData
		{
			XEREffect *effect = nullptr;
			XLib::Platform::COMPtr<ID3D12Resource> d3dCommandsBuffer;
			uint32 commandsBufferSize = 0;
			uint32 commandCount = 0;
		};

		XERDevice *device = nullptr;

		XLib::Platform::COMPtr<ID3D12Resource> d3dGeometryInstacesBuffer;
		XLib::Platform::COMPtr<ID3D12Resource> d3dTransformsBuffer;
		XLib::Matrix3x4 *mappedTransformsBuffer = nullptr;
		Internal::GPUGeometryInstanceDesc *mappedGeometryInstacesBuffer = nullptr;

		uint32 geometryInstancesBufferSize = 0;
		uint32 transformsBufferSize = 0;
		uint32 geometryInstanceCount = 0;

		EffectData effectsDataList[8];
		uint32 effectCount = 0;

		LightDesc lights[8];
		uint32 lightCount = 0;

	public:
		void initialize(XERDevice* device);

		XERGeometryInstanceId createGeometryInstance(XERGeometry* geometry, XEREffect* effect,
			const XLib::Matrix3x4& transform, XERTexture* texture = nullptr);
		XERLightId createLight(float32x3 position, XLib::Color color, float32 intensity);
		void removeGeometryInstance(XERGeometryInstanceId id);
		void removeLight(XERLightId id);

		void setGeometryInstanceTransform(XERGeometryInstanceId id, const XLib::Matrix3x4& transform);

		inline uint32 getLightCount() { return lightCount; }
		inline LightDesc& getLightDesc(XERLightId id) { return lights[id]; }
	};
}