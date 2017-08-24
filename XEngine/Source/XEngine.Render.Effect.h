#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12PipelineState;

namespace XEngine
{
	class XERSceneRender;
	class XERDevice;

	class XEREffect : public XLib::NonCopyable
	{
		friend XERSceneRender;

	private:
		COMPtr<ID3D12PipelineState> d3dPipelineState;

	public:
		void initializePlain(XERDevice* device);
		void initializeTexture(XERDevice* device);
		void initializePlainSkinned(XERDevice* device);
	};
}