#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12Resource;

namespace XEngine::Render
{
	class SceneRenderer : public XLib::NonCopyable
	{
	private:
		Device *device = nullptr;

		XLib::Platform::COMPtr<ID3D12Resource> d3dNormalTexture, d3dDiffuseTexture, d3dDepthTexture;

	public:
		SceneRenderer() = default;
		~SceneRenderer() = default;

		void initialize(Device& device, uint16x2 internalFrameBuffersSize);
		void destroy();

		void resizeInternalFrameBuffers(uint16x2 size);

		void render(Scene& scene);
	};
}