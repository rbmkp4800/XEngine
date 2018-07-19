#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;
struct IDXGISwapChain3;

namespace XEngine::Render { class Device; }
namespace XEngine::Render { class SwapChain; }
namespace XEngine::Render::Device_ { class SceneRenderer; }

namespace XEngine::Render::UI { class Batch; }

namespace XEngine::Render
{
	class Target : public XLib::NonCopyable
	{
		friend SwapChain;
		friend UI::Batch;
		friend Device_::SceneRenderer;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint16 rtvDescriptorIndex = 0;
		bool stateRenderTarget = false;
		
	public:
		Target() = default;
		~Target() = default;
	};

	class SwapChain : public XLib::NonCopyable
	{
	private:
		static constexpr uint32 bufferCount = 2;

	private:
		Device *device = nullptr;
		XLib::Platform::COMPtr<IDXGISwapChain3> dxgiSwapChain;
		Target buffers[bufferCount];
		uint16x2 size;

	private:
		void updateRTVs(bool allocateDescriptors);

	public:
		SwapChain() = default;
		~SwapChain() = default;

		void initialize(Device& device, void* hWnd, uint16x2 size);
		void destroy();

		void resize(uint16x2 size);
		void present(bool sync = false);

		Target& getCurrentTarget();

		inline uint16x2 getSize() const { return size; }
	};
}