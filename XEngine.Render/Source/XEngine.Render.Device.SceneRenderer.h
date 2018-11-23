#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>
#include <XLib.NonCopyable.h>
#include <XLib.Platform.COMPtr.h>
#include <XLib.System.Timer.h>

#include "XEngine.Render.Base.h"
#include "XEngine.Render.Internal.GPUQueueSynchronizer.h"

struct ID3D12Device;
struct ID3D12RootSignature;
struct ID3D12CommandSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;
struct ID3D12Resource;
struct ID3D12QueryHeap;

namespace XEngine::Render { struct Camera; }
namespace XEngine::Render { class Device; }
namespace XEngine::Render { class Scene; }
namespace XEngine::Render { class Target; }
namespace XEngine::Render { class GBuffer; }

namespace XEngine::Render
{
	enum class DebugOutput
	{
		Disabled = 0,
		Wireframe,
	};

	struct SceneRenderingTimings
	{
		XLib::TimerRecord cpuRenderingStart;
		XLib::TimerRecord cpuCommandListSubmit;
		XLib::TimerRecord gpuGBufferPassStart;
		union
		{
			XLib::TimerRecord gpuGBufferPassFinish;
			XLib::TimerRecord gpuShadowPassStart;
		};
		union
		{
			XLib::TimerRecord gpuShadowPassFinish;
			XLib::TimerRecord gpuLightingPassStart;
		};
		union
		{
			XLib::TimerRecord gpuLightingPassFinish;
			XLib::TimerRecord gpuPostProcessStart;
		};
		XLib::TimerRecord gpuPostProcessFinish;
	};
}

namespace XEngine::Render::Device_
{
	class SceneRenderer : public XLib::NonCopyable
	{
	private:
		struct CameraTransformConstants;
		struct LightingPassConstants;

	private:
		XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dGBufferPassCA;
		XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dGBufferPassCL;
		XLib::Platform::COMPtr<ID3D12CommandAllocator> d3dFrameFinishCA;
		XLib::Platform::COMPtr<ID3D12GraphicsCommandList> d3dFrameFinishCL;

		XLib::Platform::COMPtr<ID3D12RootSignature> d3dGBufferPassRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dLightingPassRS;
		XLib::Platform::COMPtr<ID3D12RootSignature> d3dPostProcessRS;

		XLib::Platform::COMPtr<ID3D12CommandSignature> d3dGBufferPassICS;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dDepthBufferDownscalePSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dShadowPassPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dLightingPassPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dBloomFilterAndDownscalePSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dBloomDownscalePSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dBloomBlurHorizontalPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dBloomBlurVerticalPSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dBloomBlurVerticalAndAccumulatePSO;
		XLib::Platform::COMPtr<ID3D12PipelineState> d3dToneMappingPSO;

		XLib::Platform::COMPtr<ID3D12PipelineState> d3dDebugWireframePSO;

		XLib::Platform::COMPtr<ID3D12Resource> d3dReadbackBuffer;

		XLib::Platform::COMPtr<ID3D12Resource> d3dCameraTransformCB;
		XLib::Platform::COMPtr<ID3D12Resource> d3dLightingPassCB;
		CameraTransformConstants *mappedCameraTransformCB = nullptr;
		LightingPassConstants *mappedLightingPassCB = nullptr;

		XLib::Platform::COMPtr<ID3D12QueryHeap> d3dTimestampQueryHeap;

		uint64 cpuTimerCalibrationTimespamp = 0, gpuTimerCalibrationTimespamp = 0;
		SceneRenderingTimings timings = {};

		Internal::GPUQueueSynchronizer gpuQueueSyncronizer;

	private:
		inline Device& getDevice();

	public:
		SceneRenderer() = default;
		~SceneRenderer() = default;

		void initialize();
		void destroy();

		void render(Scene& scene, const Camera& camera, GBuffer& gBuffer, Target& target,
			rectu16 viewport, bool finalizeTarget, DebugOutput debugOutput);

		void updateTimings();

		inline ID3D12RootSignature* getGBufferPassD3DRS() { return d3dGBufferPassRS; }

		inline const SceneRenderingTimings& getTimings() const { return timings; }
	};
}