#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>
#include <XLib.Color.h>

#include "XEngine.Render.Base.h"
#include "XEngine.Render.Device.DescriptorHeap.h"
#include "XEngine.Render.Device.Uploader.h"
#include "XEngine.Render.Device.SceneRenderer.h"
#include "XEngine.Render.Device.EffectHeap.h"
#include "XEngine.Render.Device.MaterialHeap.h"
#include "XEngine.Render.Device.BufferHeap.h"
#include "XEngine.Render.Device.TextureHeap.h"

struct IDXGIFactory5;

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

namespace XEngine::Render { struct Camera; }
namespace XEngine::Render { class Scene; }
namespace XEngine::Render { class Target; }
namespace XEngine::Render { class GBuffer; }
namespace XEngine::Render { class SwapChain; }

namespace XEngine::Render
{
	class Device : public XLib::NonCopyable
	{
		friend Device_::Uploader;
		friend Device_::SceneRenderer;
		friend Device_::EffectHeap;
		friend Device_::MaterialHeap;
		friend Device_::BufferHeap;

		friend Scene;
		friend GBuffer;
		friend SwapChain;

	private:
		static XLib::Platform::COMPtr<IDXGIFactory5> dxgiFactory;

		XLib::Platform::COMPtr<ID3D12Device> d3dDevice;

		XLib::Platform::COMPtr<ID3D12CommandQueue> d3dGraphicsQueue;
		XLib::Platform::COMPtr<ID3D12CommandQueue> d3dCopyQueue;

		XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dCommandList;
		XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dCommandAllocator;

		Device_::DescriptorHeap srvHeap;
		Device_::DescriptorHeap rtvHeap;
		Device_::DescriptorHeap dsvHeap;

		Device_::Uploader uploader;
		Device_::SceneRenderer sceneRenderer;

		Device_::EffectHeap		effectHeap;
		Device_::MaterialHeap	materialHeap;
		Device_::BufferHeap		bufferHeap;
		Device_::TextureHeap	textureHeap;

	public:
		Device() = default;
		~Device() = default;

		bool initialize();
		void destroy();

		void updateBuffer(BufferHandle buffer, uint32 destOffset, const void* srcData, uint32 size);
		void clearTarget(Target& target, XLib::Color color);
		void renderScene(Scene& scene, const Camera& camera,
			GBuffer& gBuffer, Target& target, rectu16 viewport);

		inline BufferHandle createBuffer(uint32 size) { return bufferHeap.createBuffer(size); }
		inline EffectHandle createEffect_plain() { return effectHeap.createEffect_plain(); }
		inline MaterialHandle createMaterial(EffectHandle effect) { return materialHeap.createMaterial(effect); }
		inline void releaseBuffer(BufferHandle handle);
	};
}