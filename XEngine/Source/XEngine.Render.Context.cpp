#include <d3d12.h>
#include "Util.D3D12.h"

#include <XLib.Math.Matrix4x4.h>

#include "XEngine.Render.Context.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Scene.h"
#include "XEngine.Render.Effect.h"
#include "XEngine.Render.UI.h"
#include "XEngine.Render.Targets.h"
#include "XEngine.Render.Camera.h"
#include "XEngine.Render.Internal.GPUStructs.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Internal;

static constexpr uint32 initTempTargetBufferWidth = 1920;
static constexpr uint32 initTempTargetBufferHeight = 1080;
static constexpr uint32 tempBufferSize = 0x100000;

static constexpr uint32 timestampId_frameBegin = 0;
static constexpr uint32 timestampId_objectsPassComplete = 1;
static constexpr uint32 timestampId_occlusionCullingComplete = 2;
static constexpr uint32 timestampId_lightingPassComplete = 3;
static constexpr uint32 timestampId_frameEnd = timestampId_lightingPassComplete;
static constexpr uint32 timestampsCount = timestampId_frameEnd + 1;

namespace XEngine::Internal
{
	struct CameraTransformCB
	{
		Matrix4x4 viewProjection;
		Matrix4x4 view;
	};

	struct LightingPassCB
	{
		static constexpr uint32 lightsLimit = 4;

		float32 zNear, zFar, aspect, fovTg;

		struct Light
		{
			float32x3 viewSpacePosition;
			uint32 _padding;
			float32x3 color;
			float32 intensity;

			inline void set(const float32x3& _viewSpacePosition, const float32x3& _color, float32 _intensity)
				{ viewSpacePosition = _viewSpacePosition; _padding = 0; color = _color; intensity = _intensity; }
		} lights[lightsLimit];
		uint32 lightsCount;
	};
}

bool XERContext::initialize(XERDevice* device)
{
	this->device = device;
	ID3D12Device *d3dDevice = device->d3dDevice;

	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCommandAllocator.uuid(), d3dCommandAllocator.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCommandAllocator, nullptr, d3dCommandList.uuid(), d3dCommandList.voidInitRef());
	d3dCommandList->Close();

	// creating temp target buffers
	{
		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R8G8B8A8_UNORM, initTempTargetBufferWidth, initTempTargetBufferHeight,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_RENDER_TARGET,
			&D3D12ClearValue_Color(DXGI_FORMAT_R8G8B8A8_UNORM), d3dDiffuseTexture.uuid(), d3dDiffuseTexture.voidInitRef());

		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R16G16_SNORM, initTempTargetBufferWidth, initTempTargetBufferHeight,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATE_RENDER_TARGET,
			nullptr, d3dNormalTexture.uuid(), d3dNormalTexture.voidInitRef());

		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R24G8_TYPELESS, initTempTargetBufferWidth, initTempTargetBufferHeight,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&D3D12ClearValue_D24S8(1.0f), d3dDepthTexture.uuid(), d3dDepthTexture.voidInitRef());

		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_D32_FLOAT, initTempTargetBufferWidth / 4, initTempTargetBufferHeight / 4,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE,
			nullptr, d3dDownscaledDepthTexture.uuid(), d3dDownscaledDepthTexture.voidInitRef());

		rtvDescriptors = device->rtvHeap.allocateDescriptors(2);
		d3dDevice->CreateRenderTargetView(d3dDiffuseTexture, nullptr, device->rtvHeap.getCPUHandle(rtvDescriptors + 0));
		d3dDevice->CreateRenderTargetView(d3dNormalTexture, nullptr, device->rtvHeap.getCPUHandle(rtvDescriptors + 1));

		dsvDescriptor = device->dsvHeap.allocateDescriptors(2);
		d3dDevice->CreateDepthStencilView(d3dDepthTexture, &D3D12DepthStencilViewDesc_Texture2D(
			DXGI_FORMAT_D24_UNORM_S8_UINT), device->dsvHeap.getCPUHandle(dsvDescriptor));
		d3dDevice->CreateDepthStencilView(d3dDownscaledDepthTexture, &D3D12DepthStencilViewDesc_Texture2D(
			DXGI_FORMAT_D32_FLOAT), device->dsvHeap.getCPUHandle(dsvDescriptor + 1));

		srvDescriptors = device->srvHeap.allocateDescriptors(3);
		d3dDevice->CreateShaderResourceView(d3dDiffuseTexture, nullptr, device->srvHeap.getCPUHandle(srvDescriptors + 0));
		d3dDevice->CreateShaderResourceView(d3dNormalTexture, nullptr, device->srvHeap.getCPUHandle(srvDescriptors + 1));
		d3dDevice->CreateShaderResourceView(d3dDepthTexture, &D3D12ShaderResourceViewDesc_Texture2D(
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS), device->srvHeap.getCPUHandle(srvDescriptors + 2));
	}

	d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(tempBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, d3dTempBuffer.uuid(), d3dTempBuffer.voidInitRef());
	tempRTVDescriptorsBase = device->srvHeap.allocateDescriptors(1);

	// creating constant buffers
	{
		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(sizeof(CameraTransformCB)), D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, d3dCameraTransformCB.uuid(), d3dCameraTransformCB.voidInitRef());
		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(sizeof(LightingPassCB)), D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, d3dLightingPassCB.uuid(), d3dLightingPassCB.voidInitRef());

		d3dCameraTransformCB->Map(0, &D3D12Range(), (void**)&mappedCameraTransformCB);
		d3dLightingPassCB->Map(0, &D3D12Range(), (void**)&mappedLightingPassCB);
	}

	return true;
}

void XERContext::draw(XERTargetBuffer* target, XERScene* scene, const XERCamera& camera,
	XERDebugWireframeMode debugWireframeMode, bool updateOcclusionCulling, XERDrawTimers* timers)
{
	uint32x2 targetSize = { 0, 0 };
	{
		D3D12_RESOURCE_DESC desc = target->d3dTexture->GetDesc();
		targetSize.x = uint32(desc.Width);
		targetSize.y = uint32(desc.Height);
	}

	// updating constant buffers ============================================================//
	{
		float32 aspect = float32(targetSize.x) / float32(targetSize.y);
		Matrix4x4 view = camera.getViewMatrix();
		Matrix4x4 viewProjection = view * camera.getProjectionMatrix(aspect);
		mappedCameraTransformCB->viewProjection = viewProjection;
		mappedCameraTransformCB->view = view;

		mappedLightingPassCB->zNear = camera.zNear;
		mappedLightingPassCB->zFar = camera.zFar;
		mappedLightingPassCB->aspect = aspect;
		mappedLightingPassCB->fovTg = Math::Tan(camera.fov / 2.0f);

		uint32 lightCount = scene->getLightCount();
		for (uint32 i = 0; i < lightCount; i++)
		{
			LightDesc &desc = scene->getLightDesc(i);

			float32x3 viewSpacePosition = (desc.position * view).xyz;
			float32x3 color = (desc.color.toF32x4()).xyz;
			mappedLightingPassCB->lights[i].set(viewSpacePosition, color, desc.intensity);
		}
		mappedLightingPassCB->lightsCount = lightCount;
	}
	
	// basic initialization =================================================================//
	{
		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator, nullptr);

		d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(targetSize.x), float32(targetSize.y)));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, targetSize.x, targetSize.y));

		ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device->srvHeap.getD3D12DescriptorHeap() };
		d3dCommandList->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);
	}

	d3dCommandList->EndQuery(device->d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, timestampId_frameBegin);

	// materials pass =======================================================================//
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device->rtvHeap.getCPUHandle(rtvDescriptors);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle = device->dsvHeap.getCPUHandle(dsvDescriptor);
		d3dCommandList->OMSetRenderTargets(2, &rtvDescriptorsHandle, TRUE, &dsvDescriptorHandle);

		const float32 clearColor[4] = {};
		d3dCommandList->ClearRenderTargetView(rtvDescriptorsHandle, clearColor, 0, nullptr);
		d3dCommandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		d3dCommandList->SetGraphicsRootSignature(device->d3dDefaultGraphicsRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(0, d3dCameraTransformCB->GetGPUVirtualAddress());

		d3dCommandList->IASetVertexBuffers(1, 1, &D3D12VertexBufferView(scene->d3dTransformsBuffer->GetGPUVirtualAddress(),
			scene->geometryInstanceCount * sizeof(Matrix3x4), sizeof(Matrix3x4)));

		for (uint32 i = 0; i < scene->effectCount; i++)
		{
			XERScene::EffectData &effectData = scene->effectsDataList[i];

			d3dCommandList->SetPipelineState(effectData.effect->d3dPipelineState);
			d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS,
				effectData.commandCount, effectData.d3dCommandsBuffer, 0, nullptr, 0);
		}
	}

	d3dCommandList->EndQuery(device->d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, timestampId_objectsPassComplete);

	// occlusion culling ====================================================================//
	if (updateOcclusionCulling)
	{
		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(targetSize.x / 4), float32(targetSize.y / 4)));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, targetSize.x / 4, targetSize.y / 4));

		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle = device->dsvHeap.getCPUHandle(dsvDescriptor + 1);
		d3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);

		// downscale depth buffer ===========================================================//
		{
			d3dCommandList->SetGraphicsRootSignature(device->d3dLightingPassRS);
			d3dCommandList->SetGraphicsRootDescriptorTable(1, device->srvHeap.getGPUHandle(srvDescriptors));
			d3dCommandList->SetPipelineState(device->d3dDepthBufferDownscalePSO);
			d3dCommandList->DrawInstanced(3, 1, 0, 0);
		}

		// temp buffer layout
		//   4 bytes - counter (must be aligned to D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT (4096))
		//   4 * geometryInstanceCount bytes - visibility flags
		//   remaining bytes - appeared geometry instances draw command list (aligned to size of command)

		// drawing bboxes ===================================================================//
		{
			d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			d3dCommandList->SetComputeRootSignature(device->d3dDefaultComputeRS);
			d3dCommandList->SetComputeRootUnorderedAccessView(2, d3dTempBuffer->GetGPUVirtualAddress());
			d3dCommandList->SetPipelineState(device->d3dClearDefaultUAVxPSO);
			d3dCommandList->Dispatch(tempBufferSize / (sizeof(uint32) * 1024), 1, 1);	// TODO: refactor that 1024

			d3dCommandList->SetGraphicsRootSignature(device->d3dDefaultGraphicsRS);
			d3dCommandList->SetGraphicsRootShaderResourceView(1, scene->d3dTransformsBuffer->GetGPUVirtualAddress());
			d3dCommandList->SetGraphicsRootUnorderedAccessView(3, d3dTempBuffer->GetGPUVirtualAddress() + sizeof(uint32));
			d3dCommandList->SetPipelineState(device->d3dOCxBBoxDrawPSO);
			d3dCommandList->DrawInstanced(scene->geometryInstanceCount * 36, 1, 0, 0);

			d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		}

		d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(targetSize.x), float32(targetSize.y)));
		d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, targetSize.x, targetSize.y));

		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorsHandle = device->rtvHeap.getCPUHandle(rtvDescriptors);
		dsvDescriptorHandle = device->dsvHeap.getCPUHandle(dsvDescriptor);
		d3dCommandList->OMSetRenderTargets(2, &rtvDescriptorsHandle, TRUE, &dsvDescriptorHandle);

		// updating command list & drawing appeared objects =================================//
		{
			uint32 appearedGeometryInstancesDrawCommandsByteOffset =
				alignup<uint32>(sizeof(uint32) * scene->geometryInstanceCount + sizeof(uint32), sizeof(GPUDefaultDrawingIC));
			uint32 appearedGeometryInstancesDrawCommandsStructOffset =
				appearedGeometryInstancesDrawCommandsByteOffset / sizeof(GPUDefaultDrawingIC);
			uint32 appearedGeometryInstancesDrawCommandsLimit =
				(tempBufferSize - appearedGeometryInstancesDrawCommandsByteOffset) / sizeof(GPUDefaultDrawingIC);

			device->d3dDevice->CreateUnorderedAccessView(d3dTempBuffer, d3dTempBuffer,
				&D3D12UnorderedAccessViewDesc_Buffer(
				appearedGeometryInstancesDrawCommandsStructOffset,
				appearedGeometryInstancesDrawCommandsLimit,
				sizeof(GPUDefaultDrawingIC), 0),
				device->srvHeap.getCPUHandle(tempRTVDescriptorsBase));

			for (uint32 i = 0; i < scene->effectCount; i++)
			{
				XERScene::EffectData &effectData = scene->effectsDataList[i];

				d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
					D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

				// d3dDefaultComputeRS after d3dClearDefaultUAVxPSO
				d3dCommandList->SetComputeRoot32BitConstant(0, effectData.commandCount, 0);
				d3dCommandList->SetComputeRootShaderResourceView(1, d3dTempBuffer->GetGPUVirtualAddress() + sizeof(uint32));
				d3dCommandList->SetComputeRootUnorderedAccessView(2, effectData.d3dCommandsBuffer->GetGPUVirtualAddress());
				d3dCommandList->SetComputeRootDescriptorTable(3, device->srvHeap.getGPUHandle(tempRTVDescriptorsBase));
				d3dCommandList->SetPipelineState(device->d3dOCxICLUpdatePSO);
				d3dCommandList->Dispatch((effectData.commandCount - 1) / 1024 + 1, 1, 1);

				d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(d3dTempBuffer,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

				d3dCommandList->SetPipelineState(effectData.effect->d3dPipelineState);
				d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS, appearedGeometryInstancesDrawCommandsLimit,
					d3dTempBuffer, appearedGeometryInstancesDrawCommandsByteOffset, d3dTempBuffer, 0);
			}
		}
	}

	d3dCommandList->EndQuery(device->d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, timestampId_occlusionCullingComplete);

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(target->d3dTexture,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// lighting pass ========================================================================//
	{
		D3D12_CPU_DESCRIPTOR_HANDLE targetRTVDescriptorHandle = device->rtvHeap.getCPUHandle(target->rtvDescriptor);
		d3dCommandList->OMSetRenderTargets(1, &targetRTVDescriptorHandle, FALSE, nullptr);

		const float32 clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		d3dCommandList->ClearRenderTargetView(targetRTVDescriptorHandle, clearColor, 0, nullptr);

		d3dCommandList->SetGraphicsRootSignature(device->d3dLightingPassRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(0, d3dLightingPassCB->GetGPUVirtualAddress());
		d3dCommandList->SetGraphicsRootDescriptorTable(1, device->srvHeap.getGPUHandle(srvDescriptors));
		d3dCommandList->SetPipelineState(device->d3dLightingPassPSO);
		d3dCommandList->DrawInstanced(3, 1, 0, 0);
	}

	d3dCommandList->EndQuery(device->d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, timestampId_lightingPassComplete);

	// debug geometry =======================================================================//
	if (debugWireframeMode != XERDebugWireframeMode::Disabled)
	{
		d3dCommandList->SetGraphicsRootSignature(device->d3dDefaultGraphicsRS);
		d3dCommandList->SetGraphicsRootConstantBufferView(0, d3dCameraTransformCB->GetGPUVirtualAddress());

		d3dCommandList->SetPipelineState(device->d3dDebugWireframePSO);

		for (uint32 i = 0; i < scene->effectCount; i++)
		{
			XERScene::EffectData &effectData = scene->effectsDataList[i];
			d3dCommandList->ExecuteIndirect(device->d3dDefaultDrawingICS,
				effectData.commandCount, effectData.d3dCommandsBuffer, 0, nullptr, 0);
		}
	}

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(target->d3dTexture,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	d3dCommandList->ResolveQueryData(device->d3dTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP,
		0, timestampsCount, device->d3dReadbackBuffer, 0);

	d3dCommandList->Close();

	ID3D12CommandList *d3dCommandListsToExecute[] = { d3dCommandList };
	device->graphicsGPUQueue.execute(d3dCommandListsToExecute, countof(d3dCommandListsToExecute));

	if (timers)
	{
		uint64 *timestamps = nullptr;
		device->d3dReadbackBuffer->Map(0, &D3D12Range(), (void**)&timestamps);

		uint64 totalDelta = timestamps[timestampId_frameEnd] - timestamps[timestampId_frameBegin];
		uint64 objectPassDelta = timestamps[timestampId_objectsPassComplete] - timestamps[timestampId_frameBegin];
		uint64 occlusionCullingDelta = timestamps[timestampId_occlusionCullingComplete] - timestamps[timestampId_objectsPassComplete];
		uint64 lightingPassDelta = timestamps[timestampId_lightingPassComplete] - timestamps[timestampId_occlusionCullingComplete];

		timers->totalTime = float32(totalDelta) * device->gpuTickPeriod;
		timers->objectsPassTime = float32(objectPassDelta) * device->gpuTickPeriod;
		timers->occlusionCullingTime = float32(occlusionCullingDelta) * device->gpuTickPeriod;
		timers->lightingPassTime = float32(lightingPassDelta) * device->gpuTickPeriod;

		device->d3dReadbackBuffer->Unmap(0, nullptr);
	}
}

void XERContext::draw(XERTargetBuffer* target, XERUIGeometryRenderer* renderer)
{
	uint32x2 targetSize = { 0, 0 };
	{
		D3D12_RESOURCE_DESC desc = target->d3dTexture->GetDesc();
		targetSize.x = uint32(desc.Width);
		targetSize.y = uint32(desc.Height);
	}

	d3dCommandAllocator->Reset();
	d3dCommandList->Reset(d3dCommandAllocator, nullptr);

	d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dCommandList->RSSetViewports(1, &D3D12ViewPort(0.0f, 0.0f, float32(targetSize.x), float32(targetSize.y)));
	d3dCommandList->RSSetScissorRects(1, &D3D12Rect(0, 0, targetSize.x, targetSize.y));

	d3dCommandList->SetGraphicsRootSignature(device->d3dUIPassRS);
	ID3D12DescriptorHeap *d3dDescriptorHeaps[] = { device->srvHeap.getD3D12DescriptorHeap() };
	d3dCommandList->SetDescriptorHeaps(countof(d3dDescriptorHeaps), d3dDescriptorHeaps);

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(target->d3dTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE targetRTVDescriptorHandle = device->rtvHeap.getCPUHandle(target->rtvDescriptor);
	d3dCommandList->OMSetRenderTargets(1, &targetRTVDescriptorHandle, FALSE, nullptr);

	renderer->fillD3DCommandList(d3dCommandList);

	d3dCommandList->ResourceBarrier(1, &D3D12ResourceBarrier_Transition(target->d3dTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	d3dCommandList->Close();

	ID3D12CommandList *d3dCommandListsToExecute[] = { d3dCommandList };
	device->graphicsGPUQueue.execute(d3dCommandListsToExecute, countof(d3dCommandListsToExecute));
}