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

			Count,
		};
	};
}

struct SceneRenderer::CameraTransformConstants
{
	Matrix4x4 viewProjection;
	Matrix4x4 view;
};

struct SceneRenderer::LightingPassConstants
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
		float32x3 viewSpacePosition;
		uint32 _padding0;
		float32x3 color;
		uint32 _padding1;
	} pointLights[4];
};

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

	// Lighting pass RS
	{
		D3D12_DESCRIPTOR_RANGE gBufferRanges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};
		D3D12_DESCRIPTOR_RANGE shadowMapRanges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
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
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + GBuffer::BloomLevelCount, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_Constants(3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
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
		psoDesc.pRootSignature = d3dGBufferPassRS; // TODO: check if this should be separated.
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

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dLightingPassPSO.uuid(), d3dLightingPassPSO.voidInitRef());
	}

	// Bloom filter and downscale x4 PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dPostProcessRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomFilterAndDownscaleX4PS.data, Shaders::BloomFilterAndDownscaleX4PS.size);
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

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dBloomFilterAndDownscaleX4PSO.uuid(), d3dBloomFilterAndDownscaleX4PSO.voidInitRef());
	}

	// Bloom blur PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dPostProcessRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomBlurPS.data, Shaders::BloomBlurPS.size);
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

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dBloomBlurPSO.uuid(), d3dBloomBlurPSO.voidInitRef());
	}

	// Bloom downscale x2 PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dPostProcessRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::BloomDownscaleX2PS.data, Shaders::BloomDownscaleX2PS.size);
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

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dBloomDownscaleX2PSO.uuid(), d3dBloomDownscaleX2PSO.voidInitRef());
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

	// Constant buffers
	{
		d3dDevice->CreateCommittedResource(
			&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(sizeof(CameraTransformConstants)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			d3dCameraTransformCB.uuid(), d3dCameraTransformCB.voidInitRef());

		d3dDevice->CreateCommittedResource(
			&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(sizeof(LightingPassConstants)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			d3dLightingPassCB.uuid(), d3dLightingPassCB.voidInitRef());

		d3dCameraTransformCB->Map(0, &D3D12Range(), to<void**>(&mappedCameraTransformCB));
		d3dLightingPassCB->Map(0, &D3D12Range(), to<void**>(&mappedLightingPassCB));
	}

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

	uint16x2 viewportSize = viewport.getSize();
	uint16x2 viewportHalfSize = viewportSize / 2;
	float32x2 viewportSizeF = float32x2(viewportSize);
	float32x2 viewportHalfSizeF = float32x2(viewportHalfSize);

	// Updating frame constants =============================================================//
	{
		float32 aspect = viewportSizeF.x / viewportSizeF.y;
		Matrix4x4 view = camera.getViewMatrix();
		Matrix4x4 viewProjection = view * camera.getProjectionMatrix(aspect);
		mappedCameraTransformCB->viewProjection = viewProjection;
		mappedCameraTransformCB->view = view;

		float32 invCameraClipPlanesDelta = 1.0f / (camera.zFar - camera.zNear);
		mappedLightingPassCB->inverseView.setInverse(view);
		mappedLightingPassCB->ndcToViewDepthConversionA = (camera.zFar * camera.zNear) * invCameraClipPlanesDelta;
		mappedLightingPassCB->ndcToViewDepthConversionB = camera.zFar * invCameraClipPlanesDelta;
		mappedLightingPassCB->aspect = aspect;
		mappedLightingPassCB->halfFOVTan = Math::Tan(camera.fov * 0.5f);

		mappedLightingPassCB->pointLightCount = 1;
		mappedLightingPassCB->pointLights[0].viewSpacePosition = (float32x3(10.0f, 10.0f, 10.0f) * view).xyz;
		mappedLightingPassCB->pointLights[0].color = float32x3(1000.0f, 1000.0f, 1000.0f);
		mappedLightingPassCB->directionalLightCount = scene.directionalLightCount;
		for (uint8 i = 0; i < scene.directionalLightCount; i++)
		{
			const Scene::DirectionalLight &srcLight = scene.directionalLights[i];
			LightingPassConstants::DirectionalLight &dstLight = mappedLightingPassCB->directionalLights[i];

			dstLight.shadowTextureTransform =
				Matrix4x4::LookAtCentered(srcLight.shadowVolumeOrigin, srcLight.desc.direction, { 0.0f, 0.0f, 1.0f }) *
				Matrix4x4::Scale(float32x3(0.5f, -0.5f, 1.0f) / srcLight.shadowVolumeSize) *
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
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device.rtvHeap.getCPUHandle(gBuffer.rtvDescriptorsBaseIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle = device.dsvHeap.getCPUHandle(gBuffer.dsvDescriptorIndex);
		d3dGBufferPassCL->OMSetRenderTargets(2, &rtvDescriptorsHandle, TRUE, &dsvDescriptorHandle);

		const float32 clearColor[4] = {};
		d3dGBufferPassCL->ClearRenderTargetView(rtvDescriptorsHandle, clearColor, 0, nullptr);
		d3dGBufferPassCL->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		d3dGBufferPassCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dGBufferPassCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dGBufferPassCL->SetGraphicsRootSignature(d3dGBufferPassRS);
		d3dGBufferPassCL->SetGraphicsRootConstantBufferView(4, d3dCameraTransformCB->GetGPUVirtualAddress());
		d3dGBufferPassCL->SetGraphicsRootDescriptorTable(5, device.srvHeap.getGPUHandle(0));

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

		// Assuming G-buffer pass RS is set
		// TODO: check if this should use separate RS
		d3dGBufferPassCL->SetGraphicsRootDescriptorTable(5, device.srvHeap.getGPUHandle(gBuffer.srvDescriptorsBaseIndex + 2));

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
	scene.populateCommandListForShadowPass(d3dFrameFinishCL, d3dGBufferPassICS, d3dShadowPassPSO);

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
			};

			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
			gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::HDR);
		d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dFrameFinishCL->SetGraphicsRootSignature(d3dLightingPassRS);
		d3dFrameFinishCL->SetGraphicsRootConstantBufferView(0, d3dLightingPassCB->GetGPUVirtualAddress());
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(gBuffer.srvDescriptorsBaseIndex));
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(2, device.srvHeap.getGPUHandle(scene.shadowMapAtlasSRVDescriptorIndex));

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
			};

			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}
	}

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

		uint32 bloomLevelScaleFactor = 4;
		for (uint32 bloomLevel = 0; bloomLevel < GBuffer::BloomLevelCount; bloomLevel++)
		{
			// Generate current level data

			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomBase + bloomLevel);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			d3dFrameFinishCL->RSSetViewports(1,
				&D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x / bloomLevelScaleFactor, viewportSizeF.y / bloomLevelScaleFactor));
			d3dFrameFinishCL->RSSetScissorRects(1,
				&D3D12Rect(0, 0, viewportSize.x / bloomLevelScaleFactor, viewportSize.y / bloomLevelScaleFactor));

			if (bloomLevel == 0)
			{
				d3dFrameFinishCL->SetPipelineState(d3dBloomFilterAndDownscaleX4PSO);

				d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / viewportSizeF.x), 0);
				d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / viewportSizeF.y), 1);
				d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
					gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::HDR));
			}
			else
			{
				d3dFrameFinishCL->SetPipelineState(d3dBloomDownscaleX2PSO);

				// NOTE: state will be reverted during tone mapping (for now).
				d3dFrameFinishCL->ResourceBarrier(1,
					&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextures[bloomLevel - 1],
						D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

				d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
					gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomBase + bloomLevel - 1));
			}

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			// Blur vertically

			d3dFrameFinishCL->SetPipelineState(d3dBloomBlurPSO);

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextures[bloomLevel],
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomBlurTemp);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(0.0f), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(bloomLevelScaleFactor / viewportSizeF.y), 1);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f), 2);
			d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
				gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomBase + bloomLevel));
		
			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			// TODO: collapse to single barrier call
			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextures[bloomLevel],
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Blur horizontally

			rtvDescriptorHandle = device.rtvHeap.getCPUHandle(
				gBuffer.rtvDescriptorsBaseIndex + GBuffer::RTVDescriptorIndex::BloomBase + bloomLevel);
			d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(bloomLevelScaleFactor / viewportSizeF.x), 0);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(0.0f), 1);
			d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f / float(1 << bloomLevel)), 2);
			d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
				gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::BloomBlurTemp));

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomBlurTemp,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

			d3dFrameFinishCL->ResourceBarrier(1,
				&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomBlurTemp,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			/////////////////

			bloomLevelScaleFactor *= 2;
		}

		// Tone map =========================================================================//

		// NOTE: bloom levels upsample is done in tone mapping. This should be optimized and fixed.

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle =
			device.rtvHeap.getCPUHandle(target.rtvDescriptorIndex);
		d3dFrameFinishCL->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dFrameFinishCL->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dFrameFinishCL->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dFrameFinishCL->SetGraphicsRoot32BitConstant(0, as<UINT>(1.0f), 0);
		d3dFrameFinishCL->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(
			gBuffer.srvDescriptorsBaseIndex + GBuffer::SRVDescriptorIndex::HDR));

		d3dFrameFinishCL->ResourceBarrier(1,
			&D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextures[GBuffer::BloomLevelCount - 1],
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		d3dFrameFinishCL->SetPipelineState(d3dToneMappingPSO);
		d3dFrameFinishCL->DrawInstanced(3, 1, 0, 0);

		{
			D3D12_RESOURCE_BARRIER d3dBarriers[GBuffer::BloomLevelCount];
			for (uint32 i = 0; i < GBuffer::BloomLevelCount; i++)
			{
				d3dBarriers[i] = D3D12ResourceBarrier_Transition(gBuffer.d3dBloomTextures[i],
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			d3dFrameFinishCL->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}

		// Debug wireframe ==================================================================//
		// TODO: move from here
		if (debugOutput == DebugOutput::Wireframe)
		{
			d3dFrameFinishCL->SetGraphicsRootSignature(d3dGBufferPassRS);
			// NOTE: assuming that first matrix in this CB is view VP matrix
			d3dFrameFinishCL->SetGraphicsRootConstantBufferView(4, d3dCameraTransformCB->GetGPUVirtualAddress());
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

	d3dFrameFinishCL->EndQuery(d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GPUTimestampId::LightingPassFinished);

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

	timings.gpuGBufferPassStart		= gpu2cpuTimestamp(timestamps[GPUTimestampId::Start]);
	timings.gpuGBufferPassFinish	= gpu2cpuTimestamp(timestamps[GPUTimestampId::GBufferPassFinish]);
	timings.gpuShadowPassFinish		= gpu2cpuTimestamp(timestamps[GPUTimestampId::ShadowPassFinish]);
	timings.gpuLightingPassFinish	= gpu2cpuTimestamp(timestamps[GPUTimestampId::LightingPassFinished]);

	d3dReadbackBuffer->Unmap(0, nullptr);
}