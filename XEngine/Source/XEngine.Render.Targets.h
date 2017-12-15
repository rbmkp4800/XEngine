#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;
struct IDXGISwapChain3;

namespace XEngine
{
	class XERWindowTarget;
	class XERSceneRender;
	class XERUIRender;
	class XERDevice;

	class XERTargetBuffer : public XLib::NonCopyable
	{
		friend XERWindowTarget;
		friend XERSceneRender;
		friend XERUIRender;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint32 rtvDescriptor = uint32(-1);
	};

	class XERWindowTarget : public XLib::NonCopyable
	{
	private:
		static constexpr uint32 bufferCount = 2;

		XERDevice *device;

		XLib::Platform::COMPtr<IDXGISwapChain3> dxgiSwapChain;
		XERTargetBuffer buffers[bufferCount];

		void updateRTVs();

	public:
		void initialize(XERDevice* device, void* hWnd, uint32 width, uint32 height);
		void resize(uint32 width, uint32 height);
		void present(bool waitVSync);

		XERTargetBuffer* getCurrentTargetBuffer();
	};
}