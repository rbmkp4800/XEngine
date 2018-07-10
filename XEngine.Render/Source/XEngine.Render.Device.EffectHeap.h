#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

// NOTE: Temporary implementation

struct ID3D12PipelineState;

namespace XEngine::Render { class Device; }

namespace XEngine::Render::Device_
{
	class EffectHeap : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dPSOs[16];
		uint32 effectCount = 0;

	private:
		inline Device& getDevice();

	public:
		EffectHeap() = default;
		~EffectHeap() = default;

		void initialize();

		EffectHandle createEffect_plain();
		EffectHandle createEffect_textured();
		void releaseEffect(EffectHandle handle);

		ID3D12PipelineState* getD3DPSO(EffectHandle handle);
	};
}