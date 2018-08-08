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
				// PS t1
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

	// Shadow pass PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dGBufferPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ShadowPassVS.data, Shaders::ShadowPassVS.size);
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
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc,
			d3dLightingPassPSO.uuid(), d3dLightingPassPSO.voidInitRef());
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
}

void SceneRenderer::destroy()
{

}

void SceneRenderer::render(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12CommandAllocator* d3dCommandAllocator, Scene& scene,
	const Camera& camera, GBuffer& gBuffer, Target& target,
	rectu16 viewport, bool finalizeTarget)
{
	uint16x2 viewportSize = viewport.getSize();
	float32x2 viewportSizeF = float32x2(viewportSize);

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

	// Basic initialization =================================================================//
	{
		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator, nullptr);

		d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device.srvHeap.getD3D12DescriptorHeap() };
		d3dCommandList->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);
	}

	// G-buffer pass ========================================================================//
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device.rtvHeap.getCPUHandle(gBuffer.rtvDescriptorsBaseIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle = device.dsvHeap.getCPUHandle(gBuffer.dsvDescriptorIndex);
		d3dCommandList->OMSetRenderTargets(2, &rtvDescriptorsHandle, TRUE, &dsvDescriptorHandle);

		const float32 clearColor[4] = {};
		d3dCommandList->ClearRenderTargetView(rtvDescriptorsHandle, clearColor, 0, nullptr);
		d3dCommandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dCommandList->SetGraphicsRootSignature(d3dGBufferPassRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(4, d3dCameraTransformCB->GetGPUVirtualAddress());
		d3dCommandList->SetGraphicsRootDescriptorTable(5, device.srvHeap.getGPUHandle(0));

		scene.populateCommandListForGBufferPass(d3dCommandList, d3dGBufferPassICS);
	}

	// Shadow map pass ======================================================================//
	scene.populateCommandListForShadowPass(d3dCommandList, d3dGBufferPassICS, d3dShadowPassPSO);

	// Lighting pass ========================================================================//
	{
		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dDiffuseTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(gBuffer.d3dNormalRoughnessMetalnessTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(gBuffer.d3dDepthTexture,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(scene.d3dShadowMapAtlas,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				// Optional:
				D3D12ResourceBarrier_Transition(target.d3dTexture,
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET),
			};

			uint32 barrierCount = countof(d3dBarriers);
			if (target.stateRenderTarget)
				barrierCount--;
			else
				target.stateRenderTarget = true;

			d3dCommandList->ResourceBarrier(barrierCount, d3dBarriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(target.rtvDescriptorIndex);
		d3dCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

		d3dCommandList->SetGraphicsRootSignature(d3dLightingPassRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(0, d3dLightingPassCB->GetGPUVirtualAddress());
		d3dCommandList->SetGraphicsRootDescriptorTable(1, device.srvHeap.getGPUHandle(gBuffer.srvDescriptorsBaseIndex));
		d3dCommandList->SetGraphicsRootDescriptorTable(2, device.srvHeap.getGPUHandle(scene.shadowMapAtlasSRVDescriptorIndex));

		d3dCommandList->SetPipelineState(d3dLightingPassPSO);
		d3dCommandList->DrawInstanced(3, 1, 0, 0);

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
				// Optional:
				D3D12ResourceBarrier_Transition(target.d3dTexture,
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON),
			};

			uint32 barrierCount = countof(d3dBarriers);
			if (finalizeTarget)
				target.stateRenderTarget = false;
			else
				barrierCount--;

			d3dCommandList->ResourceBarrier(barrierCount, d3dBarriers);
		}
	}

	// Finalize =============================================================================//
	{
		d3dCommandList->Close();

		ID3D12CommandList *d3dCommandListsToExecute[] = { d3dCommandList };
		device.d3dGraphicsQueue->ExecuteCommandLists(
			countof(d3dCommandListsToExecute), d3dCommandListsToExecute);
	}
}