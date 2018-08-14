#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

// NOTE: Temporary implementation
// TODO: Remove redundant indirection when getting effect for material

struct ID3D12Resource;
struct ID3D12PipelineState;

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class MaterialHeap : public XLib::NonCopyable
	{
	private:
		struct Effect
		{
			XLib::Platform::COMPtr<ID3D12PipelineState> d3dPSO;
			uint16 materialConstantsTableArenaBaseSegment;
			uint16 materialConstantsSize;
			uint16 materialUserSpecifiedConstantsOffset;
			uint16 constantsUsed;
		};

		struct Material
		{
			EffectHandle effectHandle;
			uint16 constantsIndex;
		};

	private:
		Effect effects[16];
		Material materials[256];
		uint16 effectCount = 0;
		uint16 materialCount = 0;

		XLib::Platform::COMPtr<ID3D12Resource> d3dMaterialConstantsTableArena;
		byte* mappedMaterialConstantsTableArena = nullptr;
		uint16 allocatedCommandListArenaSegmentCount = 0;

	private:
		inline Device& getDevice();

	public:
		MaterialHeap() = default;
		~MaterialHeap() = default;

		void initialize();

		EffectHandle createEffect_perMaterialAlbedoRoughtnessMetalness();
		EffectHandle createEffect_albedoTexturePerMaterialRoughtnessMetalness();
		EffectHandle createEffect_albedoNormalRoughtnessMetalnessTexture();
		void releaseEffect(EffectHandle handle);

		MaterialHandle createMaterial(EffectHandle effect,
			const void* initialConstants = nullptr,
			const TextureHandle* intialTextures = nullptr);
		void releaseMaterial(MaterialHandle handle);

		void updateMaterialConstants(MaterialHandle handle, uint32 offset, const void* data, uint32 size);
		void updateMaterialTexture(MaterialHandle handle, uint32 slot, TextureHandle textureHandle);

		uint16 getMaterialConstantsTableEntryIndex(MaterialHandle handle) const;
		EffectHandle getEffect(MaterialHandle handle) const;
		ID3D12PipelineState* getEffectPSO(EffectHandle handle);
		uint64 getMaterialConstantsTableGPUAddress(EffectHandle handle);
	};
}