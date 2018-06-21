#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;
struct IDXGISwapChain3;

namespace XEngine::Render { class Device; }

namespace XEngine::Render
{
	class Target : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint16 rtvDescriptorHandle = 0;
		
	public:
		Target() = default;
		~Target() = default;
	};

	class WindowTarget : public XLib::NonCopyable
	{
	private:
		static constexpr uint32 bufferCount = 2;

	private:
		Device *device = nullptr;
		Target bufffers[bufferCount];

	public:
		WindowTarget() = default;
		~WindowTarget() = default;

		Target& getCurrentTarget();
	};
}