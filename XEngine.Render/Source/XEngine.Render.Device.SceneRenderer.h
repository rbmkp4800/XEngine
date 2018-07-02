#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12Device;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList2;
struct ID3D12CommandAllocator;
struct ID3D12Resource;

namespace XEngine::Render { class Device; }
namespace XEngine::Render { class Scene; }
namespace XEngine::Render { class Camera; }
namespace XEngine::Render { class Target; }
namespace XEngine::Render { class GBuffer; }

namespace XEngine::Render::Device_
{
	class SceneRenderer final : public XLib::NonCopyable
	{
	private:
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dGBufferPassRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dLightingPassRS;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dLightingPassPSO;

		XLib::Platform::COMPtr<ID3D12Resource> d3dConstantBuffer;

	private:
		inline Device& getDevice();

	public:
		SceneRenderer() = default;
		~SceneRenderer() = default;

		void initialize(ID3D12Device* d3dDevice);
		void destroy();

		void render(ID3D12GraphicsCommandList2* d3dCommandList,
			ID3D12CommandAllocator* d3dCommandAllocator, Scene& scene,
			const Camera& camera, GBuffer& gBuffer, Target& target, rectu16 viewport);
	};
}