#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12Resource;
struct IDXGISwapChain3;

namespace XEngine
{
	class XERContext;
	class XERDevice;

	class XERTargetBuffer : public XLib::NonCopyable
	{
		friend XERContext;

	private:
		COMPtr<ID3D12Resource> d3dTexture;
		uint32 rtvDescritptor = 0;

	public:
		void initialize(ID3D12Resource* _d3dTexture, uint32 _rtvDescriptor) { d3dTexture = _d3dTexture; rtvDescritptor = _rtvDescriptor; }
	};

	class XERWindowTarget : public XLib::NonCopyable
	{
	private:
		static constexpr uint32 bufferCount = 2;

		COMPtr<IDXGISwapChain3> dxgiSwapChain;
		XERTargetBuffer buffers[bufferCount];

	public:
		void initialize(XERDevice* contex, void* hWnd, uint32 width, uint32 height);
		void present(bool waitVSync);
		XERTargetBuffer* getCurrentTargetBuffer();
	};
}