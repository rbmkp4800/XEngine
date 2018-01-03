#include <d3d12.h>
#include <dxgi1_5.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.h"
#include "XEngine.Render.Internal.Shaders.h"
#include "XEngine.Render.Internal.GPUStructs.h"

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine;
using namespace XEngine::Internal;

static constexpr uint32 rtvDescriptorsLimit = 8;
static constexpr uint32 dsvDescriptorsLimit = 8;
static constexpr uint32 srvDescriptorsLimit = 8;

COMPtr<IDXGIFactory5> XERDevice::dxgiFactory;
COMPtr<ID3D12Debug> d3dDebug;

bool XERDevice::initialize()
{
	if (!dxgiFactory.isInitialized())
		CreateDXGIFactory1(dxgiFactory.uuid(), dxgiFactory.voidInitRef());

	if (!d3dDebug.isInitialized())
	{
		D3D12GetDebugInterface(d3dDebug.uuid(), d3dDebug.voidInitRef());
		d3dDebug->EnableDebugLayer();
	}

	for (uint32 adapterIndex = 0;; adapterIndex++)
	{
		COMPtr<IDXGIAdapter1> dxgiAdapter;
		if (dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.initRef()) == DXGI_ERROR_NOT_FOUND)
		{
			dxgiFactory->EnumWarpAdapter(dxgiAdapter.uuid(), dxgiAdapter.voidInitRef());
			if (FAILED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, d3dDevice.uuid(), d3dDevice.voidInitRef())))
				return false;
			break;
		}

		DXGI_ADAPTER_DESC1 desc = {};
		dxgiAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, d3dDevice.uuid(), d3dDevice.voidInitRef())))
			break;
	}

	{
		COMPtr<IDXGIAdapter> dxgiAdapter;
		dxgiFactory->EnumAdapterByLuid(d3dDevice->GetAdapterLuid(), dxgiAdapter.uuid(), dxgiAdapter.voidInitRef());
		DXGI_ADAPTER_DESC desc;
		dxgiAdapter->GetDesc(&desc);

		for (uint32 i = 0; i < countof(name) - 1; i++)
		{
			name[i] = char(desc.Description[i]);
			if (!name[i])
				break;
		}
	}

	// root signatures creation =============================================================//

	// default grapics RS
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

		D3D12_STATIC_SAMPLER_DESC staticSapmplers[] =
		{
			D3D12StaticSamplerDesc_Default(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
			countof(staticSapmplers), staticSapmplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dDefaultGraphicsRS.uuid(), d3dDefaultGraphicsRS.voidInitRef());
	}

	// default compute RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_Constants(1, 0, 0),
			D3D12RootParameter_SRV(0, 0),
			D3D12RootParameter_UAV(0, 0),
			D3D12RootParameter_Table(countof(ranges), ranges),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE), D3D_ROOT_SIGNATURE_VERSION_1,
			d3dSignature.initRef(), d3dError.initRef());
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dDefaultComputeRS.uuid(), d3dDefaultComputeRS.voidInitRef());
	}

	// lighting pass RS
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

	// UI pass RS
	{
		D3D12_DESCRIPTOR_RANGE ranges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_PARAMETER rootParameters[] =
		{
			D3D12RootParameter_Table(countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		D3D12_STATIC_SAMPLER_DESC staticSapmplers[] =
		{
			D3D12StaticSamplerDesc_DisableMIPs(0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
		};

		COMPtr<ID3DBlob> d3dSignature, d3dError;
		D3D12SerializeRootSignature(&D3D12RootSignatureDesc(countof(rootParameters), rootParameters,
			countof(staticSapmplers), staticSapmplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, d3dSignature.initRef(), d3dError.initRef());
		d3dDevice->CreateRootSignature(0, d3dSignature->GetBufferPointer(), d3dSignature->GetBufferSize(),
			d3dUIPassRS.uuid(), d3dUIPassRS.voidInitRef());
	}

	// pipeline states creation =============================================================//

	// lighting pass PSO
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

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dLightingPassPSO.uuid(), d3dLightingPassPSO.voidInitRef());
	}

	// depth buffer downscale PSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultGraphicsRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::ScreenQuadVS.data, Shaders::ScreenQuadVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::DepthBufferDownscalePS.data, Shaders::DepthBufferDownscalePS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_DisableRead();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dDepthBufferDownscalePSO.uuid(), d3dDepthBufferDownscalePSO.voidInitRef());
	}

	// debug wireframe PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "TRANSFORM",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
			{ "TRANSFORM",	2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultGraphicsRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::DebugPositionOnlyVS.data, Shaders::DebugPositionOnlyVS.size);
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

	// debug OC BBox draw PSO
	{
		// TODO: change RS to disable input assembler

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultGraphicsRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::DebugOCxBBoxVS.data, Shaders::DebugOCxBBoxVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::DebugWhitePS.data, Shaders::DebugWhitePS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc(D3D12_CULL_MODE_NONE);
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dDebugOCxBBoxPSO.uuid(), d3dDebugOCxBBoxPSO.voidInitRef());
	}

	// UI color PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dUIPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::UIColorVS.data, Shaders::UIColorVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::UIColorPS.data, Shaders::UIColorPS.size);
		psoDesc.BlendState = D3D12BlendDesc_DefaultBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dUIColorPSO.uuid(), d3dUIColorPSO.voidInitRef());
	}

	// UI font PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dUIPassRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::UIFontVS.data, Shaders::UIFontVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::UIFontPS.data, Shaders::UIFontPS.size);
		psoDesc.BlendState = D3D12BlendDesc_DefaultBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_Disable();
		psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dUIFontPSO.uuid(), d3dUIFontPSO.voidInitRef());
	}

	// OC BBox draw PSO
	{
		// TODO: change RS to disable input assembler

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultGraphicsRS;
		psoDesc.VS = D3D12ShaderBytecode(Shaders::OCxBBoxVS.data, Shaders::OCxBBoxVS.size);
		psoDesc.PS = D3D12ShaderBytecode(Shaders::OCxBBoxPS.data, Shaders::OCxBBoxPS.size);
		psoDesc.BlendState = D3D12BlendDesc_NoBlend();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = D3D12RasterizerDesc(D3D12_CULL_MODE_NONE);
		psoDesc.DepthStencilState = D3D12DepthStencilDesc_DisableWrite();
		psoDesc.InputLayout = D3D12InputLayoutDesc();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dOCxBBoxDrawPSO.uuid(), d3dOCxBBoxDrawPSO.voidInitRef());
	}

	// Clear default UAV PSO
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultComputeRS;
		psoDesc.CS = D3D12ShaderBytecode(Shaders::ClearDefaultUAVxCS.data, Shaders::ClearDefaultUAVxCS.size);
		d3dDevice->CreateComputePipelineState(&psoDesc, d3dClearDefaultUAVxPSO.uuid(), d3dClearDefaultUAVxPSO.voidInitRef());
	}

	// OC ICL Update PSO
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = d3dDefaultComputeRS;
		psoDesc.CS = D3D12ShaderBytecode(Shaders::OCxICLUpdateCS.data, Shaders::OCxICLUpdateCS.size);
		d3dDevice->CreateComputePipelineState(&psoDesc, d3dOCxICLUpdatePSO.uuid(), d3dOCxICLUpdatePSO.voidInitRef());
	}

	// Default drawing ICS
	{
		D3D12_INDIRECT_ARGUMENT_DESC indirectArgumentDescs[] =
		{
			D3D12IndirectArgumentDesc_VBV(0),
			D3D12IndirectArgumentDesc_IBV(),
			D3D12IndirectArgumentDesc_Constants(1, 0, 1),
			D3D12IndirectArgumentDesc_DrawIndexed(),
		};

		d3dDevice->CreateCommandSignature(&D3D12CommandSignatureDesc(sizeof(GPUDefaultDrawingIC),
			countof(indirectArgumentDescs), indirectArgumentDescs), d3dDefaultGraphicsRS,
			d3dDefaultDrawingICS.uuid(), d3dDefaultDrawingICS.voidInitRef());
	}

	// queries
	{
		// TODO: refactor
		d3dDevice->CreateCommittedResource(&D3D12HeapProperties(D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAG_NONE,
			&D3D12ResourceDesc_Buffer(256), D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			d3dReadbackBuffer.uuid(), d3dReadbackBuffer.voidInitRef());
		d3dDevice->CreateQueryHeap(&D3D12QueryHeapDesc(D3D12_QUERY_HEAP_TYPE_TIMESTAMP, 16),
			d3dTimestampQueryHeap.uuid(), d3dTimestampQueryHeap.voidInitRef());
	}

	rtvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvDescriptorsLimit, false);
	dsvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvDescriptorsLimit, false);
	srvHeap.initalize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvDescriptorsLimit, true);
	gpuGraphicsQueue.initialize(d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	uploadEngine.initalize(d3dDevice);
	graphicsCommandListPool.initialize(d3dDevice);

	gpuTickPeriod = 1.0f / float32(gpuGraphicsQueue.getTimestampFrequency());

	return true;
}

// GPUQueue =================================================================================//

void XERDevice::GPUQueue::initialize(ID3D12Device* d3dDevice, D3D12_COMMAND_LIST_TYPE type)
{
	d3dDevice->CreateCommandQueue(&D3D12CommandQueueDesc(type), d3dCommandQueue.uuid(), d3dCommandQueue.voidInitRef());
	d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, d3dFence.uuid(), d3dFence.voidInitRef());
	fenceSyncEvent.initialize(false, false);
	fenceValue = 1;

	synchronize();
}

void XERDevice::GPUQueue::synchronize()
{
	d3dCommandQueue->Signal(d3dFence, fenceValue);
	if (d3dFence->GetCompletedValue() < fenceValue)
	{
		d3dFence->SetEventOnCompletion(fenceValue, fenceSyncEvent.getHandle());
		fenceSyncEvent.wait();
	}

	fenceValue++;
}

void XERDevice::GPUQueue::execute(ID3D12CommandList** d3dCommandLists, uint32 count)
{
	d3dCommandQueue->ExecuteCommandLists(count, d3dCommandLists);
	synchronize();
}

uint64 XERDevice::GPUQueue::getTimestampFrequency()
{
	UINT64 frequency = 0;
	d3dCommandQueue->GetTimestampFrequency(&frequency);
	return frequency;
}

// DescriptorHeap ===========================================================================//

void XERDevice::DescriptorHeap::initalize(ID3D12Device* d3dDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 size, bool shaderVisible)
{
	d3dDevice->CreateDescriptorHeap(&D3D12DescriptorHeapDesc(type, size,
		shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
		d3dDescriptorHeap.uuid(), d3dDescriptorHeap.voidInitRef());
	allocatedDescriptorsCount = 0;
	descritorSize = d3dDevice->GetDescriptorHandleIncrementSize(type);
}

uint32 XERDevice::DescriptorHeap::allocateDescriptors(uint32 count)
{
	uint32 result = allocatedDescriptorsCount;
	allocatedDescriptorsCount += count;
	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE XERDevice::DescriptorHeap::getCPUHandle(uint32 descriptor)
	{ return d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart() + descriptor * descritorSize; }
D3D12_GPU_DESCRIPTOR_HANDLE XERDevice::DescriptorHeap::getGPUHandle(uint32 descriptor)
	{ return d3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart() + descriptor * descritorSize; }

// GraphicsCommandListPool ==================================================================//

void XERDevice::GraphicsCommandListPool::initialize(ID3D12Device* d3dDevice)
{
	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCommandAllocator.uuid(), d3dCommandAllocator.voidInitRef());
	d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCommandAllocator, nullptr, d3dCommandList.uuid(), d3dCommandList.voidInitRef());
	d3dCommandList->Close();
}

bool XERDevice::GraphicsCommandListPool::acquireDefault(ID3D12GraphicsCommandList*& d3dCommandList,
	ID3D12CommandAllocator*& d3dCommandAllocator)
{
	if (acquired)
		return false;

	acquired = true;
	d3dCommandList = this->d3dCommandList;
	d3dCommandAllocator = this->d3dCommandAllocator;
	return true;
}
void XERDevice::GraphicsCommandListPool::releaseDefault()
{
	acquired = false;
}