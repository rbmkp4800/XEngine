#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Math.Matrix4x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.SceneRenderer.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.Camera.h"
#include "XEngine.Render.Scene.h"
#include "XEngine.Render.Target.h"
#include "XEngine.Render.GBuffer.h"
#include "XEngine.Render.Internal.Shaders.h"
#include "XEngine.Render.ClassLinkage.h"

#define device this->getDevice()

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;
using namespace XEngine::Render::Device_;

namespace
{
	constexpr uint32 frameConstantsBufferSize = 16384;

	class GPUTimestampId abstract final
	{
	public:
		enum : uint32
		{
			Start = 0,
			GBufferPassFinish,
			DepthBufferDownscaleFinish,
			ShadowPassFinish,
			LightingPassFinished,
			PostProcessFinished,

			Count,
		};
	};

	struct CameraTransformConstants
	{
		Matrix4x4 viewProjection;
		Matrix4x4 view;
	};

	struct ShadowTransformConstants
	{
		Matrix4x4 transform;
	};

	struct LightingPassConstants
	{
		Matrix4x4 inverseView;
		float32 ndcToViewDepthConversionA;
		float32 ndcToViewDepthConversionB;
		float32 aspect;
		float32 halfFOVTan;

		uint32 directionalLightCount;
		uint32 pointLightCount;
		uint32 _padding0;
		uint32 _padding1;

		struct DirectionalLight
		{
			Matrix4x4 shadowTextureTransform;
			float32x3 direction;
			uint32 _padding0;
			float32x3 color;
			uint32 _padding1;
		} directionalLights[2];

		struct PointLight
		{
			Matrix4x4 shadowTextureTransforms[6];
			float32x3 viewSpacePosition;
			uint32 _padding0;
			float32x3 color;
			uint32 _padding1;
		} pointLights[4];
	};

	inline Matrix4x4 getPointLightShadowDirectionTransform(float32x3 position, uint8 direction)
	{
		float32x3 target = { 0.0f, 0.0f, 0.0f };
		if (direction == 0)
			target.x = +1.0f;
		else if (direction == 1)
			target.x = -1.0f;
		else if (direction == 2)
			target.y = +1.0f;
		else if (direction == 3)
			target.y = -1.0f;
		else if (direction == 4)
			target.z = +1.0f;
		else if (direction == 5)
			target.z = -1.0f;

		const float32x3 up = direction == 4 || direction == 5 ?
			float32x3(1.0f, 0.0f, 0.0f) : float32x3(0.0f, 0.0f, 1.0f);

		return
			Matrix4x4::LookAtCentered(position, target, up) *
			Matrix4x4::Perspective(Math::HalfPi<float32>, 1.0f, 0.5f, 20.0f);
	}
}

void SceneRenderer::initialize()
{
	ID3D12Device *d3dDevice = device.d3dDevice;

	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		d3dGBufferPassCA.uuid(), d3dGBufferPassCA.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		d3dGBufferPassCA, nullptr, d3dGBufferPassCL.uuid(), d3dGBufferPassCL.voidInitRef());
	d3dGBufferPassCL->Close();

	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		d3dFrameFinishCA.uuid(), d3dFrameFinishCA.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		d3dFrameFinishCA, nullptr, d3dFrameFinishCL.uuid(), d3dFrameFinishCL.voidInitRef());
	d3dFrameFinishCL->Close();

	// G-buffer pass RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 16, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
				// VS b0 base transform index
			D3D12RootParameter_Constants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX),
				// PS b0 base material index
			D3D12RootParameter_Constants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				// PS b1 materials table
			D3D12RootParameter_CBV(1, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				// VS t0 transforms buffer
			D3D12RootParameter_SRV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX),
				// PS b2 camera transform constant buffer
			D3D12RootParameter_CBV(2, 0, D3D12_SHADER_VISIBILITY_VERTEX),
				// PS t0 space1 textures
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		D3D12_STATIC_SAMPLER_DESC staticSamplers[] =
		{
			D3D12StaticSamplerDesc_Default(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(
			&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
				countof(staticSamplers), staticSamplers,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());

		d3dDevice->CreateRootSignature(0,
			d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dGBufferPassRS.uuid(), d3dGBufferPassRS.voidInitRef());
	}

	// Depth buffer downscale RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		D3D12_STATIC_SAMPLER_DESC staticSamplers[] =
		{
			D3D12StaticSamplerDesc_Default(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(
			&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
				countof(staticSamplers), staticSamplers),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());

		d3dDevice->CreateRootSignature(0,
			d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dDepthBufferDownscaleRS.uuid(), d3dDepthBufferDownscaleRS.voidInitRef());
	}

	// Lighting pass RS
	{
		D3D12_DESCRIPTOR_RANGE gBufferRanges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};
		D3D12_DESCRIPTOR_RANGE shadowMapRanges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 3, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
				// b0 lighting pass constants
			D3D12RootParameter_CBV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				// t0-t3 G-buffer textures
			D3D12RootParameter_Table(countof(gBufferRanges), gBufferRanges, D3D12_SHADER_VISIBILITY_PIXEL),
				// t4 shadow maps
			D3D12RootParameter_Table(countof(shadowMapRanges), shadowMapRanges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		D3D12_STATIC_SAMPLER_DESC staticSamplers[] =
		{
				// s0 shadow sampler
			D3D12StaticSamplerDesc(0, 0, D3D12_SHADER_VISIBILITY_PIXEL,
				D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_COMPARISON_FUNC_LESS),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(
			&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
				countof(staticSamplers), staticSamplers),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());

		d3dDevice->CreateRootSignature(0,
			d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dLightingPassRS.uuid(), d3dLightingPassRS.voidInitRef());
	}

	// Post-process RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_Constants(3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.MipLODBias = 0.0f;
		staticSampler.MaxAnisotropy = 1;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		staticSampler.MinLOD = 0.0f;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(
			&D3D12RootSignatureDesc(countof(rootParameters), rootParameters, 1, &staticSampler),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());

		d3dDevice->CreateRootSignature(0,
			d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dPostProcessRS.uuid(), d3dPostProcessRS.voidInitRef());
	}

	// G-buffer pass ICS
	{
		D3D12_INDIRECT_ARGUMENT_DESC indirectArgumentDescs[] =
		{
			D3D12IndirectArgumentDesc_VBV(0),
			D3D12IndirectArgumentDesc_IBV(),
			D3D12IndirectArgumentDesc_Constants(0, 0, 1),
			D3D12IndirectArgumentDesc_Constants(1, 0, 1),
			D3D12IndirectArgumentDesc_DrawIndexed(),
		};

#pragma pack(push, 1)
		struct IndirectCommand
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			D3D12_INDEX_BUFFER_VIEW ibv;
			uint32 constants0;
			uint32 constants1;
			D3D12_DRAW_INDEXED_ARGUMENTS args;
		};
#pragma pack(pop)

		d3dDevice->CreateCommandSignature(
			&D3D12CommandSignatureDesc(sizeof(IndirectCommand),
				countof(indirectArgumentDescs), indirectArgumentDescs),
			d3dGBufferPassRS, d3dGBufferPassICS.uuid(), d3dGBufferPassICS.voidInitRef());
	}

	// Depth buffer downscale PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDepthBufferDownscaleRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::DepthBufferDownscalePS.data, Shaders::DepthBufferDownscalePS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dDepthBufferDownscalePSO.uuid(), d3dDepthBufferDownscalePSO.voidInitRef());
	}

	// Shadow pass PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dGBufferPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::SceneGeometryPositionOnlyVS.data, Shaders::SceneGeometryPositionOnlyVS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Default();
		psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		device.d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dShadowPassPSO.uuid(), d3dShadowPassPSO.voidInitRef());
	}

	// Lighting pass PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dLightingPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::LightingPassPS.data, Shaders::LightingPassPS.size);
		psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R11G11B10_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dLightingPassPSO.uuid(), d3dLightingPassPSO.voidInitRef());
	}

	// Bloom PSOs
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dPostProcessRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		//     .PS customized
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R11G11B10_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomFilterAndDownscalePS.data, Shaders::BloomFilterAndDownscalePS.size);
		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dBloomFilterAndDownscalePSO.uuid(), d3dBloomFilterAndDownscalePSO.voidInitRef());

		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomDownscalePS.data, Shaders::BloomDownscalePS.size);
		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dBloomDownscalePSO.uuid(), d3dBloomDownscalePSO.voidInitRef());

		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomBlurHorizontalPS.data, Shaders::BloomBlurHorizontalPS.size);
		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dBloomBlurHorizontalPSO.uuid(), d3dBloomBlurHorizontalPSO.voidInitRef());

		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomBlurVerticalPS.data, Shaders::BloomBlurVerticalPS.size);
		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dBloomBlurVerticalPSO.uuid(), d3dBloomBlurVerticalPSO.voidInitRef());

		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomBlurVerticalAndAccumulatePS.data, Shaders::BloomBlurVerticalAndAccumulatePS.size);
		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dBloomBlurVerticalAndAccumulatePSO.uuid(), d3dBloomBlurVerticalAndAccumulatePSO.voidInitRef());
	}

	// Tone mapping PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dPostProcessRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::ToneMappingPS.data, Shaders::ToneMappingPS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dToneMappingPSO.uuid(), d3dToneMappingPSO.voidInitRef());
	}

	// Debug wireframe PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dGBufferPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::SceneGeometryPositionOnlyVS.data, Shaders::SceneGeometryPositionOnlyVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::DebugWhitePS.data, Shaders::DebugWhitePS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc(D3D12_CULL_MODE_BACK, D3D12_FILL_MODE_WIREFRAME, true);
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dDebugWireframePSO.uuid(), d3dDebugWireframePSO.voidInitRef());
	}

	// Readback buffer
	{
		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(1024 * 1024 * 2), D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			d3dReadbackBuffer.uuid(), d3dReadbackBuffer.voidInitRef());
	}

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(frameConstantsBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dFrameConstantsBuffer.uuid(), d3dFrameConstantsBuffer.voidInitRef());
	d3dFrameConstantsBuffer->Map(0, &D3D12Range(), to<void**>(&mappedFrameConstantsBuffer));

	d3dDevice->CreateQueryHeap(&D3D12QueryHeapDesc(D3D12_QUERY_HEAP_TYPE_TIMESTAMP, 16),
		d3dTimestampQueryHeap.uuid(), d3dTimestampQueryHeap.voidInitRef());

	gpuQueueSyncronizer.initialize(d3dDevice);
}

void SceneRenderer::destroy()
{

}

void SceneRenderer::render(Scene& scene, const Camera& camera, GBuffer& gBuffer,
	Target& target, rectu16 viewport, bool finalizeTarget, DebugOutput debugOutput)
{
	timings.cpuRenderingStart = Timer::GetRecord();

	device.d3dGraphicsQueue->GetClockCalibration(&gpuTimerCalibrationTimespamp, &cpuTimerCalibrationTimespamp);

	const uint16x2 viewportSize = viewport.getSize();
	const uint16x2 viewportHalfSize = viewportSize / 2;
	const float32x2 viewportSizeF = float32x2(viewportSize);
	const float32x2 viewportHalfSizeF = float32x2(viewportHalfSize);

	// TODO: refactor

	const D3D12_GPU_VIRTUAL_ADDRESS frameConstantsBufferAddress = d3dFrameConstantsBuffer->GetGPUVirtualAddress();

	uint32 frameConstantsBufferBytesUsed = 0;

	CameraTransformConstants *mappedCameraTransformConstants =
		to<CameraTransformConstants*>(mappedFrameConstantsBuffer + frameConstantsBufferBytesUsed);
	const D3D12_GPU_VIRTUAL_ADDRESS cameraTransformConstantsAddress =
		frameConstantsBufferAddress + frameConstantsBufferBytesUsed;
	frameConstantsBufferBytesUsed += alignup<uint32>(
		sizeof(CameraTransformConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	LightingPassConstants *mappedLightingPassConstants =
		to<LightingPassConstants*>(mappedFrameConstantsBuffer + frameConstantsBufferBytesUsed);
	const D3D12_GPU_VIRTUAL_ADDRESS lightingPassConstantsAddress =
		frameConstantsBufferAddress + frameConstantsBufferBytesUsed;
	frameConstantsBufferBytesUsed += alignup<uint32>(
		sizeof(LightingPassConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	// Updating frame constants =============================================================//
	{
		float32 aspect = viewportSizeF.x / viewportSizeF.y;
		Matrix4x4 view = camera.getViewMatrix();
		Matrix4x4 viewProjection = view * camera.getProjectionMatrix(aspect);
		mappedCameraTransformConstants->viewProjection = viewProjection;
		mappedCameraTransformConstants->view = view;

		float32 invCameraClipPlanesDelta = 1.0f / (camera.zFar - camera.zNear);
		mappedLightingPassConstants->inverseView.setInverse(view);
		mappedLightingPassConstants->ndcToViewDepthConversionA = (camera.zFar * camera.zNear) * invCameraClipPlanesDelta;
		mappedLightingPassConstants->ndcToViewDepthConversionB = camera.zFar * invCameraClipPlanesDelta;
		mappedLightingPassConstants->aspect = aspect;
		mappedLightingPassConstants->halfFOVTan = Math::Tan(camera.fov * 0.5f);

		// TODO: handle limits
		mappedLightingPassConstants->pointLightCount = scene.pointLightCount;
		for (uint16 i = 0; i < scene.pointLightCount; i++)
		{
			const Scene::PointLight &srcLight = scene.pointLights[i];
			LightingPassConstants::PointLight &dstLight = mappedLightingPassConstants->pointLights[i];

			for (uint8 direction = 0; direction < 6; direction++)
			{
				dstLight.shadowTextureTransforms[direction] =
					getPointLightShadowDirectionTransform(srcLight.desc.position, direction) *
					Matrix4x4::Scale(float32x3(0.5f, -0.5f, 1.0f)) *
					Matrix4x4::Translation(0.5f, 0.5f, 0.0f);
			}
			
			dstLight.viewSpacePosition = (srcLight.desc.position * view).xyz;
			dstLight.color = srcLight.desc.color;
		}

		mappedLightingPassConstants->directionalLightCount = scene.directionalLightCount;
		for (uint8 i = 0; i < scene.directionalLightCount; i++)
		{
			const Scene::DirectionalLight &srcLight = scene.directionalLights[i];
			LightingPassConstants::DirectionalLight &dstLight = mappedLightingPassConstants->directionalLights[i];

			Matrix4x4 transform =
				Matrix4x4::LookAtCentered(srcLight.shadowVolumeOrigin, srcLight.desc.direction, { 0.0f, 0.0f, 1.0f }) *
				Matrix4x4::Scale(float32x3(1.0f, 1.0f, 1.0f) / srcLight.shadowVolumeSize);

			dstLight.shadowTextureTransform = transform *
				Matrix4x4::Scale(float32x3(0.5f, -0.5f, 1.0f)) *
				Matrix4x4::Translation(0.5f, 0.5f, 0.0f);

			dstLight.direction = VectorMath::Normalize(MultiplyBySubmatrix3x3(-srcLight.desc.direction, view));
			dstLight.color = srcLight.desc.color;
		}
	}

	// Initialize G-Buffer pass command list ================================================//
	{
		d3dGBufferPassCA->Reset();
		d3dGBufferPassCL->Reset(d3dGBufferPassCA, nullptr);

		d3dGBufferPassCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device.srvHeap.getD3D12DescriptorHeap() };
		d3dGBufferPassCL->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);
	}

	d3dGBufferPassCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::Start);

	// G-buffer pass ========================================================================//
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device.rtvHeap.getCPUHandle(
			gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::Diffuse);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle = device.dsvHeap.getCPUHandle(gBuffer.dsvDescriptorIndex);
		d3dGBufferPassCL->OMSetRenderTargets(3, &rtvDescriptorsHandle, TRUE, &dsvDescriptorHandle);

		D3D12_CPU_DESCRIPTOR_HANDLE hdrRTVDescriptorsHandle = device.rtvHeap.getCPUHandle(
			gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::HDR);

		const float32 clearColor[4] = {};
		d3dGBufferPassCL->ClearRenderTargetView(rtvDescriptorsHandle, clearColor, 0, nullptr);
		d3dGBufferPassCL->ClearRenderTargetView(hdrRTVDescriptorsHandle, clearColor, 0, nullptr);
		d3dGBufferPassCL->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		d3dGBufferPassCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dGBufferPassCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dGBufferPassCL->SetGraphicsRootSignature(d3dGBufferPassRS);
		d3dGBufferPassCL->SetGraphicsRootConstantBufferView(4, cameraTransformConstantsAddress);
		d3dGBufferPassCL->SetGraphicsRootDescriptorTable(5, device.srvHeap.getGPUHandle(0));

		// TODO: check if separate emissive pass is faster then mixed emissive + default (extra RTV is bound)
		scene.populateCommandListForGBufferPass(d3dGBufferPassCL, d3dGBufferPassICS, true);
	}

	d3dGBufferPassCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::GBufferPassFinish);

	// Depth buffer downscale and readback ==================================================//
	{
		d3dGBufferPassCL->ResourceBarrier(1,
			&D3D12ResourceBarrier_Transition(gBuffer.d3dDepthTexture,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device.rtvHeap.getCPUHandle(
			gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::DownscaledX2Depth);
		d3dGBufferPassCL->OMSetRenderTargets(1, &rtvDescriptorsHandle, FALSE, nullptr);

		d3dGBufferPassCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportHalfSizeF.x, viewportHalfSizeF.y));
		d3dGBufferPassCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportHalfSize.x, viewportHalfSize.y));

		d3dGBufferPassCL->SetGraphicsRootSignature(d3dDepthBufferDownscaleRS);
		d3dGBufferPassCL->SetGraphicsRootDescriptorTable(0, device.srvHeap.getGPUHandle(
			gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::Depth));

		d3dGBufferPassCL->SetPipelineState(d3dDepthBufferDownscalePSO);
		d3dGBufferPassCL->DrawInstanced(3, 1, 0, 0);

		d3dGBufferPassCL->ResourceBarrier(1,
			&D3D12ResourceBarrier_Transition(gBuffer.d3dDownscaledX2DepthTexture,
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		uint16 readbackRowPitch = alignup(viewportHalfSize.x * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		d3dGBufferPassCL->CopyTextureRegion(
			&D3D12TextureCopyLocation_PlacedFootprint(d3dReadbackBuffer, 0,
				DXGI_FORMAT_R32_FLOAT, viewportHalfSize.x, viewportHalfSize.y, 1, readbackRowPitch),
			0, 0, 0,
			&D3D12TextureCopyLocation_Subresource(gBuffer.d3dDownscaledX2DepthTexture, 0),
			&D3D12Box(0, viewportHalfSize.x, 0, viewportHalfSize.y));

		d3dGBufferPassCL->ResourceBarrier(1,
			&D3D12ResourceBarrier_Transition(gBuffer.d3dDownscaledX2DepthTexture,
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	d3dGBufferPassCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::DepthBufferDownscaleFinish);

	// Submit G-Buffer pass command list ====================================================//
	{
		d3dGBufferPassCL->Close();

		ID3D12CommandList *d3dCommandListsToExecute[] = { d3dGBufferPassCL };
		device.d3dGraphicsQueue->ExecuteCommandLists(
			countof(d3dCommandListsToExecute), d3dCommandListsToExecute);
	}

	// NOTE: Temporary implementation
	//gpuQueueSyncronizer.synchronize(device.d3dGraphicsQueue);

	//uint16 *t = nullptr;
	//d3dReadbackBuffer->Map(0, &D3D12Range(), ...);
	//d3dReadbackBuffer->Unmap(0, nullptr);

	// Initialize frame finish command list =================================================//
	{
		d3dFrameFinishCA->Reset();
		d3dFrameFinishCL->Reset(d3dFrameFinishCA, nullptr);

		d3dFrameFinishCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device.srvHeap.getD3D12DescriptorHeap() };
		d3dFrameFinishCL->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);
	}

	// Shadow map pass ======================================================================//

	d3dFrameFinishCL->SetGraphicsRootSignature(d3dGBufferPassRS);

	d3dFrameFinishCL->SetPipelineState(d3dShadowPassPSO);

	if (scene.pointLightCount)
	{
		const Scene::PointLight &light = scene.pointLights[0];

		for (uint8 direction = 0; direction < 6; direction++)
		{
			ShadowTransformConstants *mappedShadowTransformConstants =
				to<ShadowTransformConstants*>(mappedFrameConstantsBuffer + frameConstantsBufferBytesUsed);
			const D3D12_GPU_VIRTUAL_ADDRESS shadowTransformConstantsAddress =
				frameConstantsBufferAddress + frameConstantsBufferBytesUsed;
			frameConstantsBufferBytesUsed += alignup<uint32>(
				sizeof(ShadowTransformConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

			mappedShadowTransformConstants->transform =
				getPointLightShadowDirectionTransform(light.desc.position, direction);

			const D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle =
				device.dsvHeap.getCPUHandle(light.dsvDescriptorsBaseIndex + direction);
			d3dFrameFinishCL->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);
			d3dFrameFinishCL->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			const uint32 dim = Scene::pointLightShadowMapDim;
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim, dim));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim, dim));

			d3dFrameFinishCL->SetGraphicsRootConstantBufferView(4, shadowTransformConstantsAddress);

			scene.populateCommandListForShadowPass(d3dFrameFinishCL, d3dGBufferPassICS);
		}
	}

	for (uint8 i = 0; i < scene.directionalLightCount; i++)
	{
		const Scene::DirectionalLight &light = scene.directionalLights[0];

		ShadowTransformConstants *mappedShadowTransformConstants =
			to<ShadowTransformConstants*>(mappedFrameConstantsBuffer + frameConstantsBufferBytesUsed);
		const D3D12_GPU_VIRTUAL_ADDRESS shadowTransformConstantsAddress =
			frameConstantsBufferAddress + frameConstantsBufferBytesUsed;
		frameConstantsBufferBytesUsed += alignup<uint32>(
			sizeof(ShadowTransformConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		mappedShadowTransformConstants->transform =
			Matrix4x4::LookAtCentered(light.shadowVolumeOrigin, light.desc.direction, { 0.0f, 0.0f, 1.0f }) *
			Matrix4x4::Scale(float32x3(1.0f, 1.0f, 1.0f) / light.shadowVolumeSize);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle =
			device.dsvHeap.getCPUHandle(light.dsvDescriptorIndex);
		d3dFrameFinishCL->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);
		d3dFrameFinishCL->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, light.shadowMapDim, light.shadowMapDim));
		d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, light.shadowMapDim, light.shadowMapDim));

		d3dFrameFinishCL->SetGraphicsRootConstantBufferView(4, shadowTransformConstantsAddress);

		scene.populateCommandListForShadowPass(d3dFrameFinishCL, d3dGBufferPassICS);
	}

	d3dFrameFinishCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::ShadowPassFinish);

	// Lighting pass ========================================================================//
	{
		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dDiffuseTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(gBuffer.d3dNormalRoughnessMetalnessTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(scene.d3dShadowMapAtlas,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(scene.d3dPointLightShadowMaps,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			};

			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
			gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::HDR);
		d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dFrameFinishCL->SetGraphicsRootSignature(d3dLightingPassRS);
		d3dFrameFinishCL->SetGraphicsRootConstantBufferView(0, lightingPassConstantsAddress);
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(gBuffer.srvDescriptorsBaseIndex));
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(2, device.srvHeap.getGPUHandle(scene.srvDescriptorsBaseIndex));

		d3dFrameFinishCL->SetPipelineState(d3dLightingPassPSO);
		d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dDiffuseTexture,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D12ResourceBarrier_Transition(gBuffer.d3dNormalRoughnessMetalnessTexture,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D12ResourceBarrier_Transition(gBuffer.d3dDepthTexture,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				D3D12ResourceBarrier_Transition(scene.d3dShadowMapAtlas,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				D3D12ResourceBarrier_Transition(scene.d3dPointLightShadowMaps,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
			};

			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}
	}

	d3dFrameFinishCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::LightingPassFinished);

	// Post-process =========================================================================//
	{
		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dHDRTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				{},
			};

			uint32 barrierCount = countof(d3dBarriers);
			if (target.stateRenderTarget)
				barrierCount--;
			else
			{
				target.stateRenderTarget = true;
				d3dBarriers[1] = D3D12ResourceBarrier_Transition(target.d3dTexture,
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			d3dFrameFinishCL->ResourceBarrier(barrierCount, d3dBarriers);
		}

		d3dFrameFinishCL->SetGraphicsRootSignature(d3dPostProcessRS);

		// Generate bloom ===================================================================//

		uint16x2 bloomLevelDims[GBuffer::BloomLevelCount];
		{
			const uint16 alignment = 1 << GBuffer::BloomLevelCount;
			bloomLevelDims[0].x = alignup(viewportSize.x / 4, alignment);
			bloomLevelDims[0].y = alignup(viewportSize.y / 4, alignment);

			for (uint32 i = 1; i < GBuffer::BloomLevelCount; i++)
				bloomLevelDims[i] = bloomLevelDims[i - 1] / 2;
		}

		// Filter and downscale HDR buffer to first bloom level

		{
			d3dFrameFinishCL->SetPipelineState(d3dBloomFilterAndDownscalePSO);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomABase);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			const uint16x2 dim = bloomLevelDims[0];
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim.x, dim.y));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim.x, dim.y));

			float32x2 sampleOffset = float32x2(0.25f, 0.25f) / float32x2(bloomLevelDims[0]);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstants(0, 2, &sampleOffset, 0);
			d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
				gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::HDR));

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);
		}

		// Downscale filtered bloom

		d3dFrameFinishCL->ResourceBarrier(1,
			&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0));

		d3dFrameFinishCL->SetPipelineState(d3dBloomDownscalePSO);

		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
			gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomA));

		for (uint32 bloomLevel = 1; bloomLevel < GBuffer::BloomLevelCount; bloomLevel++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomABase + bloomLevel);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			const uint16x2 dim = bloomLevelDims[bloomLevel];
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim.x, dim.y));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim.x, dim.y));

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(float32(bloomLevel - 1)), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(0.5f / float32(dim.x)), 1);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(0.5f / float32(dim.y)), 2);

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, bloomLevel));
		}

		// Blur bloom levels horizontally

		d3dFrameFinishCL->SetPipelineState(d3dBloomBlurHorizontalPSO);

		for (uint32 bloomLevel = 0; bloomLevel < GBuffer::BloomLevelCount; bloomLevel++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomBBase + bloomLevel);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			const uint16x2 dim = bloomLevelDims[bloomLevel];
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim.x, dim.y));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim.x, dim.y));

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(float32(bloomLevel)), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / float32(dim.x)), 1);
			d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
				gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomA));

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);
		}

		// Blur last bloom level vertically

		{
			d3dFrameFinishCL->SetPipelineState(d3dBloomBlurVerticalPSO);

			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET,
					GBuffer::BloomLevelCount - 1),
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureB,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					GBuffer::BloomLevelCount - 1),
			};
			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomABase + GBuffer::BloomLevelCount - 1);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			const uint16x2 dim = bloomLevelDims[GBuffer::BloomLevelCount - 1];
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim.x, dim.y));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim.x, dim.y));

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(float32(GBuffer::BloomLevelCount - 1)), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / float32(dim.y)), 1);
			d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
				gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomB));

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					GBuffer::BloomLevelCount - 1));
		}

		// Blur previous bloom levels vertically and accumulate result

		d3dFrameFinishCL->SetPipelineState(d3dBloomBlurVerticalAndAccumulatePSO);

		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
			gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomA));

		for (sint32 bloomLevel = GBuffer::BloomLevelCount - 2; bloomLevel >= 0; bloomLevel--)
		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, bloomLevel),
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureB,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, bloomLevel),
			};
			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomABase + bloomLevel);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			const uint16x2 dim = bloomLevelDims[bloomLevel];
			d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, dim.x, dim.y));
			d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, dim.x, dim.y));

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(float32(bloomLevel)), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / float32(dim.y)), 1);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(float32(1.0f)), 2);

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, bloomLevel));
		}

		// Tone map =========================================================================//

		d3dFrameFinishCL->SetPipelineState(d3dToneMappingPSO);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle =
			device.rtvHeap.getCPUHandle(target.rtvDescriptorIndex);
		d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f), 0);
		// NOTE: assuming that HDR texture and BloomA texture descriptors are single range
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
			gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::HDR));

		d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

		{
			D3D12_RESOURCE_BARRIER d3dBarriers[2] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureA,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextureB,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
			};
			// NOTE: this can be done as a split barrier
			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}

		// Debug wireframe ==================================================================//
		// TODO: move from here
		if (debugOutput == DebugOutput::Wireframe)
		{
			d3dFrameFinishCL->SetGraphicsRootSignature(d3dGBufferPassRS);
			// NOTE: assuming that first matrix in this CB is view VP matrix
			d3dFrameFinishCL->SetGraphicsRootConstantBufferView(4, cameraTransformConstantsAddress);
			d3dFrameFinishCL->SetPipelineState(d3dDebugWireframePSO);

			scene.populateCommandListForGBufferPass(d3dFrameFinishCL, d3dGBufferPassICS, false);
		}

		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dHDRTexture,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				{},
			};

			uint32 barrierCount = countof(d3dBarriers);
			if (finalizeTarget)
			{
				target.stateRenderTarget = false;
				d3dBarriers[1] = D3D12ResourceBarrier_Transition(target.d3dTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
			}
			else
				barrierCount--;

			d3dFrameFinishCL->ResourceBarrier(barrierCount, d3dBarriers);
		}
	}

	d3dFrameFinishCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::PostProcessFinished);

	d3dFrameFinishCL->ResolveQueryData(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP,
		0, GPUTimestampId::Count, d3dReadbackBuffer, 0);

	timings.cpuCommandListSubmit = Timer::GetRecord();

	// Submit frame finish command list =====================================================//
	{
		d3dFrameFinishCL->Close();

		ID3D12CommandList *d3dCommandListsToExecute[] = { d3dFrameFinishCL };
		device.d3dGraphicsQueue->ExecuteCommandLists(
			countof(d3dCommandListsToExecute), d3dCommandListsToExecute);
	}
}

void SceneRenderer::updateTimings()
{
	uint64 *timestamps = nullptr;
	d3dReadbackBuffer->Map(0, &D3D12Range(), (void**)&timestamps);

	uint64 cpuClockFrequency = 0;
	QueryPerformanceFrequency(PLARGE_INTEGER(&cpuClockFrequency));

	auto gpu2cpuTimestamp = [&](uint64 gpuTimestamp) -> uint64
	{
		return cpuTimerCalibrationTimespamp +
			(gpuTimestamp - gpuTimerCalibrationTimespamp) * cpuClockFrequency / device.graphicsQueueClockFrequency;
	};

	timings.gpuGBufferPassStart				= gpu2cpuTimestamp(timestamps[GPUTimestampId::Start]);
	timings.gpuGBufferPassFinish			= gpu2cpuTimestamp(timestamps[GPUTimestampId::GBufferPassFinish]);
	timings.gpuDepthBufferDownscaleFinish	= gpu2cpuTimestamp(timestamps[GPUTimestampId::DepthBufferDownscaleFinish]);
	timings.gpuShadowPassFinish				= gpu2cpuTimestamp(timestamps[GPUTimestampId::ShadowPassFinish]);
	timings.gpuLightingPassFinish			= gpu2cpuTimestamp(timestamps[GPUTimestampId::LightingPassFinished]);
	timings.gpuPostProcessFinish			= gpu2cpuTimestamp(timestamps[GPUTimestampId::PostProcessFinished]);

	d3dReadbackBuffer->Unmap(0, nullptr);
}