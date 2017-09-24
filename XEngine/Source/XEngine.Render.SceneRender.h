#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12Fence;
struct ID3D12Resource;

namespace XEngine
{
	struct XERCamera;

	class XERScene;
	class XERUIRender;
	class XERTargetBuffer;
	class XERWindowTarget;
	class XERDevice;

	enum class XERDebugWireframeMode : uint8
	{
		Disabled = 0,
		Enabled = 1,
	};

	struct XERSceneDrawTimings
	{
		float32 objectsPassFinished;
		float32 occlusionCullingDownscaleFinished;
		float32 occlusionCullingBBoxDrawFinished;
		float32 occlusionCullingFinished;
		float32 lightingPassFinished;
	};

	namespace Internal
	{
		struct CameraTransformCB;
		struct LightingPassCB;
	}

	class XERSceneRender : public XLib::NonCopyable
	{
	private:
		XERDevice *device = nullptr;

		COMPtr<ID3D12Resource> d3dNormalTexture, d3dDiffuseTexture, d3dDepthTexture;
		COMPtr<ID3D12Resource> d3dDownscaledDepthTexture;
		uint32 rtvDescriptors = 0, srvDescriptors = 0;	// +0 diffuse, +1 normal
		uint32 dsvDescriptor = 0;

		COMPtr<ID3D12Resource> d3dTempBuffer;
		uint32 tempRTVDescriptorsBase;

		COMPtr<ID3D12Resource> d3dCameraTransformCB;
		COMPtr<ID3D12Resource> d3dLightingPassCB;
		Internal::CameraTransformCB *mappedCameraTransformCB = nullptr;
		Internal::LightingPassCB *mappedLightingPassCB = nullptr;

	public:
		bool initialize(XERDevice* device);

		void draw(XERTargetBuffer* target, XERScene* scene, const XERCamera& camera,
			XERDebugWireframeMode debugWireframeMode, bool updateOcclusionCulling,
			XERSceneDrawTimings* timings = nullptr);

		inline XERDevice* getDevice() { return device; }
	};
}