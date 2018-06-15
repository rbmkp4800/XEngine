#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

namespace XEngine::Render::Device_
{
	class MaterialHeap final : public XLib::NonCopyable
	{
		friend Device;

	private:
		MaterialHeap() = default;
		~MaterialHeap() = default;

		inline Device* getDevice();

	public:
		MaterialHandle createMaterial(EffectHandle effect,
			const void* initialConstants = nullptr,
			const TextureHandle* intialTextures = nullptr);
		void releaseMaterial(MaterialHandle handle);

		void updateMaterialConstants(MaterialHandle handle, uint32 offset, const void* data, uint32 size);
		void updateMaterialTexture(MaterialHandle handle, uint32 slot, TextureHandle textureHandle);

	// internal:
		inline uint64 translateHandleToGPUAddress(MaterialHandle handle) const;
	};
}