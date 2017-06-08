#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Math.Matrix3x4.h>

#include "XEngine.Color.h"

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;

namespace XEngine
{
	class XEREffect;
	class XERGeometry;
	class XERContext;
	class XERDevice;

	using XERGeometryInstanceId = uint32;
	using XERLightId = uint32;

	struct LightDesc
	{
		float32x3 position;
		XEColor color;
		float32 intensity;
	};

	namespace Internal
	{
		struct GPUGeometryInstanceDesc;
	}

	class XERScene : public XLib::NonCopyable
	{
		friend XERContext;

	private:
		struct EffectData
		{
			XEREffect *effect = nullptr;
			COMPtr<ID3D12Resource> d3dCommandsBuffer;
			uint32 commandsBufferSize = 0;
			uint32 commandCount = 0;
		};

		XERDevice *device = nullptr;

		COMPtr<ID3D12Resource> d3dGeometryInstacesBuffer;
		COMPtr<ID3D12Resource> d3dTransformsBuffer;
		XLib::Matrix3x4 *mappedTransformsBuffer = nullptr;
		Internal::GPUGeometryInstanceDesc *mappedGeometryInstacesBuffer = nullptr;

		uint32 geometryInstancesBufferSize = 0, transformsBufferSize = 0;
		uint32 geometryInstanceCount = 0;

		EffectData effectsDataList[8];
		uint32 effectCount = 0;

		LightDesc lights[8];
		uint32 lightCount = 0;

		void fillD3DCommandList_draw(ID3D12GraphicsCommandList* d3dCommandList);
		void fillD3DCommandList_drawWithoutEffects(ID3D12GraphicsCommandList* d3dCommandList);
		void fillD3DCommandList_runOcclusionCulling(ID3D12GraphicsCommandList* d3dCommandList,
			ID3D12Resource *d3dTempBuffer, uint32 tempBufferSize, uint32 tempRTVDescriptorsBase);

	public:
		void initialize(XERDevice* device);

		XERGeometryInstanceId createGeometryInstance(XERGeometry* geometry, XEREffect* effect, const XLib::Matrix3x4& transform);
		XERLightId createLight(float32x3 position, XEColor color, float32 intensity);
		void removeGeometryInstance(XERGeometryInstanceId id);
		void removeLight(XERLightId id);

		void setGeometryInstanceTransform(XERGeometryInstanceId id, const XLib::Matrix3x4& transform);

		inline uint32 getLightCount() { return lightCount; }
		inline LightDesc& getLightDesc(XERLightId id) { return lights[id]; }
	};
}