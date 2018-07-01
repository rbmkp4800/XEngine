#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Math.Matrix4x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.SceneRenderer.h"

#include "XEngine.Render.Camera.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Scene.h"
#include "XEngine.Render.Target.h"
#include "XEngine.Render.GBuffer.h"

#define device this->getDevice()

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine::Render;
using namespace XEngine::Render::Device_;

namespace
{
	struct ConstantBuffer
	{
		Matrix4x4 viewProjection;
		Matrix4x4 view;
	};
}

void SceneRenderer::inititalize(ID3D12Device* d3dDevice)
{
	// G-buffer pass RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 16, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_CBV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX),
			D3D12RootParameter_Constants(1, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX),
			D3D12RootParameter_SRV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX),
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
			// TODO: need this for objects pass (transforms). Maybe move to separate RS
			D3D12RootParameter_UAV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
			// TODO: remove after moving OC to CPU
		};

		D3D12_STATIC_SAMPLER_DESC staticSamplers[] =
		{
			D3D12StaticSamplerDesc_Default(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
			countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dGBufferPassRS.uuid(), d3dGBufferPassRS.voidInitRef());
	}

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(ConstantBuffer)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, d3dConstantBuffer.uuid(), d3dConstantBuffer.voidInitRef());
}

void SceneRenderer::destroy()
{

}

void SceneRenderer::render(ID3D12GraphicsCommandList2* d3dCommandList,
	ID3D12CommandAllocator* d3dCommandAllocator, Scene& scene,
	const Camera& camera, GBuffer& gBuffer, Target& target, rectu16 viewport)
{
	uint16x2 viewportSize = viewport.getSize();

	// basic initialization =================================================================//
	{
		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator, nullptr);

		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(viewportSize.x), float32(viewportSize.y)));
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
		d3dCommandList->SetGraphicsRootConstantBufferView(0, d3dConstantBuffer->GetGPUVirtualAddress());
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