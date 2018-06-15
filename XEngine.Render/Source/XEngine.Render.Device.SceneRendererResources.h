#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12RootSignature;

namespace XEngine::Render::Device_
{
	class SceneRendererResources final : public XLib::NonCopyable
	{
		friend Device;

	private:
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dGBufferPassRS;

	private:
		SceneRendererResources() = default;
		~SceneRendererResources() = default;

		inline Device* getDevice();

		void inititalize();
		void destroy();

	public:
		
	};
}