#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

// NOTE: Temporary implementation

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class MaterialHeap : public XLib::NonCopyable
	{
	private:

	private:
		inline Device* getDevice();

	public:
		MaterialHeap() = default;
		~MaterialHeap() = default;

		void initialize();

		MaterialHandle createMaterial(EffectHandle effect,
			const void* initialConstants = nullptr,
			const TextureHandle* intialTextures = nullptr);
		void releaseMaterial(MaterialHandle handle);

		void updateMaterialConstants(MaterialHandle handle, uint32 offset, const void* data, uint32 size);
		void updateMaterialTexture(MaterialHandle handle, uint32 slot, TextureHandle textureHandle);

		EffectHandle getEffect(MaterialHandle handle) const;
		uint64 translateHandleToGPUAddress(MaterialHandle handle) const;
	};
}