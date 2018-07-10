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

#define device this->getDevice()

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;
using namespace XEngine::Render::Device_;

struct SceneRenderer::CameraTransformConstants
{
	Matrix4x4 view;
	Matrix4x4 viewProjection;
};

struct SceneRenderer::LightingPassConstants
{
	float32 ndcToViewDepthConversionA;
	float32 ndcToViewDepthConversionB;
	float32 aspect;
	float32 halfFOVTan;
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
				// t0 transforms buffer
			D3D12RootParameter_SRV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX),
				// b0 material constant buffer
			D3D12RootParameter_CBV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				// b1 camera transform constant buffer
			D3D12RootParameter_CBV(1, 0, D3D12_SHADER_VISIBILITY_VERTEX),
				// t1
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
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dGBufferPassRS.uuid(), d3dGBufferPassRS.voidInitRef());
	}

	// Lighting pass RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_CBV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(&D3D12RootSignatureDesc(countof(rootParameters), rootParameters),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dLightingPassRS.uuid(), d3dLightingPassRS.voidInitRef());
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

	// Camera transform constants buffer
	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(CameraTransformConstants)),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dCameraTransformCB.uuid(), d3dCameraTransformCB.voidInitRef());
	d3dCameraTransformCB->Map(0, &D3D12Range(), to<void**>(&mappedCameraTransformCB));
}

void SceneRenderer::destroy()
{

}

void SceneRenderer::render(ID3D12GraphicsCommandList2* d3dCommandList,
	ID3D12CommandAllocator* d3dCommandAllocator, Scene& scene,
	const Camera& camera, GBuffer& gBuffer, Target& target, rectu16 viewport)
{
	uint16x2 viewportSize = viewport.getSize();
	float32x2 viewportSizeF = float32x2(viewportSize);

	// Updating frame constants =============================================================//
	{
		float32 aspect = viewportSizeF.x / viewportSizeF.y;
		Matrix4x4 view = camera.getViewMatrix();
		Matrix4x4 viewProjection = view * camera.getProjectionMatrix(aspect);
		mappedCameraTransformCB->view = view;
		mappedCameraTransformCB->viewProjection = viewProjection;

		//float32 cameraClipPlanesDelta = camera.zFar - camera.zNear;
		//mappedLightingPassCB->ndcToViewDepthConversionA = (camera.zFar * camera.zNear) / cameraClipPlanesDelta;
		//mappedLightingPassCB->ndcToViewDepthConversionB = camera.zFar / cameraClipPlanesDelta;
		//mappedLightingPassCB->aspect = aspect;
		//mappedLightingPassCB->halfFOVTan = Math::Tan(camera.fov * 0.5f);
	}

	// Basic initialization =================================================================//
	{
		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator, nullptr);

		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, viewportSizeF.x, viewportSizeF.y));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, viewportSize.x, viewportSize.y));

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

		d3dCommandList->SetGraphicsRootSignature(d3dGBufferPassRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(2, d3dCameraTransformCB->GetGPUVirtualAddress());
		d3dCommandList->SetGraphicsRootDescriptorTable(3, device.srvHeap.getGPUHandle(0));

		scene.populateCommandList(d3dCommandList);
	}

	// Lighting pass ========================================================================//
	{
		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dDiffuseTexture,	D3D12_RESOURCE_STATE_RENDER_TARGET,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(gBuffer.d3dNormalTexture,	D3D12_RESOURCE_STATE_RENDER_TARGET,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				D3D12ResourceBarrier_Transition(gBuffer.d3dDepthTexture,	D3D12_RESOURCE_STATE_DEPTH_WRITE,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			};
			d3dCommandList->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle = device.rtvHeap.getCPUHandle(target.rtvDescriptorIndex);
		d3dCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

		d3dCommandList->SetGraphicsRootSignature(d3dLightingPassRS);
		d3dCommandList->SetPipelineState(d3dLightingPassPSO);
		d3dCommandList->DrawInstanced(3, 1, 0, 0);

		{
			D3D12_RESOURCE_BARRIER d3dBarriers[] =
			{
				D3D12ResourceBarrier_Transition(gBuffer.d3dDiffuseTexture,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D12ResourceBarrier_Transition(gBuffer.d3dNormalTexture,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D12ResourceBarrier_Transition(gBuffer.d3dDepthTexture,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	D3D12_RESOURCE_STATE_DEPTH_WRITE),
			};
			d3dCommandList->ResourceBarrier(countof(d3dBarriers), d3dBarriers);
		}
	}
}