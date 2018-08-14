#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

// NOTE: Temporary implementation

struct ID3D12Resource;

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class TextureHeap : public XLib::NonCopyable
	{
	private:
		struct Texture
		{
			XLib::Platform::COMPtr<ID3D12Resource> d3dTexture;
			uint16 srvDescriptorIndex;
		};

	private:
		Texture textures[16];
		uint32 textureCount = 0;

	private:
		inline Device& getDevice();

	public:
		TextureHeap() = default;
		~TextureHeap() = default;

		void initialize();

		TextureHandle createTexture(uint16 width, uint16 height);
		void releaseTexture(TextureHandle handle);

		ID3D12Resource* getTexture(TextureHandle handle);
		uint16 getSRVDescriptorIndex(TextureHandle handle) const;
	};
}