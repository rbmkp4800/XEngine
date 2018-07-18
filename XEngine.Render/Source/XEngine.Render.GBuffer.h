#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;

namespace XEngine::Render { class Device; }
namespace XEngine::Render::Device_ { class SceneRenderer; }

namespace XEngine::Render
{
	class GBuffer : public XLib::NonCopyable
	{
		friend Device_::SceneRenderer;

	private:
		Device *device = nullptr;

		XLib::Platform::COMPtr<ID3D12Resource> d3dDepthTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dNormalRoughnessMetalnessTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dDiffuseTexture;

		uint16x2 size = { 0, 0 };

		uint16 srvDescriptorsBaseIndex = 0;
		uint16 rtvDescriptorsBaseIndex = 0;
		uint16 dsvDescriptorIndex = 0;

	public:
		GBuffer() = default;
		~GBuffer() = default;

		void initialize(Device& device, uint16x2 size);
		void destroy();

		void resize(uint16x2 size);

		inline bool isInitialized() { return device != nullptr; }
	};
}