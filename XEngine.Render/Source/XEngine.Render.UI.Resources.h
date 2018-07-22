#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

struct ID3D12Resource;

namespace XEngine::Render { class Device; }
namespace XEngine::Render::UI { class Batch; }

namespace XEngine::Render::UI
{
	class Texture : public XLib::NonCopyable
	{
		friend Batch;

	private:
		XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
		uint16 srvDescriptorIndex = 0;

	public:
		Texture() = default;
		~Texture() = default;

		void initializeA8(Device& device, const void* data, uint16 width, uint16 height);
	};
}