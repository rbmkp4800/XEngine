#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.Render.Base.h"

struct ID3D12PipelineState;

namespace XEngine::Render::Device_
{
	// NOTE: Temporary solution.
	enum class EffectType
	{
		None = 0,
		Colored,
		Textured,
	};

	class EffectHeap final : public XLib::NonCopyable
	{
		friend Device;

	private:
		EffectHeap() = default;
		~EffectHeap() = default;

		inline Device* getDevice();

	public:
		EffectHandle createEffect(EffectType type);

	// internal:
		ID3D12PipelineState* getPSO(EffectHandle handle);
	};
}