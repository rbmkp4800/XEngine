#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;
struct ID3D12Fence;
struct ID3D12Resource;

namespace XEngine
{
	struct XERCamera;

	class XERScene;
	class XERUIGeometryRenderer;
	class XERTargetBuffer;
	class XERWindowTarget;
	class XERDevice;

	enum class XERDebugWireframeMode : uint8
	{
		Disabled = 0,
		Enabled = 1,
	};

	namespace Internal
	{
		struct CameraTransformCB;
		struct LightingPassCB;
	}

	class XERContext : public XLib::NonCopyable
	{
	private:
		XERDevice *device = nullptr;

		COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
		COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;

		COMPtr<ID3D12Resource> d3dNormalTexture, d3dDiffuseTexture, d3dDepthTexture;
		uint32 rtvDescriptors = 0, srvDescriptors = 0;	// +0 diffuse, +1 normal
		uint32 dsvDescriptor = 0;

		COMPtr<ID3D12Resource> d3dCameraTransformCB;
		COMPtr<ID3D12Resource> d3dLightingPassCB;
		Internal::CameraTransformCB *mappedCameraTransformCB = nullptr;
		Internal::LightingPassCB *mappedLightingPassCB = nullptr;

	public:
		bool initialize(XERDevice* device);

		void draw(XERTargetBuffer* target, XERScene* scene, const XERCamera& camera, XERDebugWireframeMode debugWireframeMode);
		void draw(XERTargetBuffer* target, XERUIGeometryRenderer* renderer);

		inline XERDevice* getDevice() { return device; }
	};
}