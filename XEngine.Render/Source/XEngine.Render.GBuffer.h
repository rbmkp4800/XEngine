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
		static constexpr uint32 BloomLevelCount = 4;

		struct SRVDescriptorIndex
		{
			enum
			{
				Diffuse = 0,
				NormalRoughnessMetalness,
				Depth,
				HDR,
				BloomBase,
				BloomBlurTemp = BloomBase + BloomLevelCount,

				Count,
			};
		};

		struct RTVDescriptorIndex
		{
			enum
			{
				Diffuse = 0,
				NormalRoughnessMetalness,
				HDR,
				BloomBase,
				BloomBlurTemp = BloomBase + BloomLevelCount,
				DownscaledX2Depth,

				Count,
			};
		};

	private:
		Device *device = nullptr;

		XLib::Platform::COMPtr<ID3D12Resource> d3dDiffuseTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dNormalRoughnessMetalnessTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dDepthTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dHDRTexture;
		XLib::Platform::COMPtr<ID3D12Resource> d3dBloomTextures[BloomLevelCount];
		XLib::Platform::COMPtr<ID3D12Resource> d3dBloomBlurTemp;
		XLib::Platform::COMPtr<ID3D12Resource> d3dDownscaledX2DepthTexture;

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