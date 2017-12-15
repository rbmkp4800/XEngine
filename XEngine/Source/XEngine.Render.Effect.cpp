#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Effect.h"
#include "XEngine.Render.Device.h"
#include "XEngine.Render.Internal.Shaders.h"

using namespace XEngine;
using namespace XEngine::Internal;

void XEREffect::initializePlain(XERDevice* device)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = device->d3dDefaultGraphicsRS;
	psoDesc.VS = D3D12ShaderBytecode(Shaders::EffectPlainVS.data, Shaders::EffectPlainVS.size);
	psoDesc.PS = D3D12ShaderBytecode(Shaders::EffectPlainPS.data, Shaders::EffectPlainPS.size);
	psoDesc.BlendState = D3D12BlendDesc_NoBlend();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
	psoDesc.DepthStencilState = D3D12DepthStencilDesc_Default();
	psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_SNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	device->d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dPipelineState.uuid(), d3dPipelineState.voidInitRef());
}

void XEREffect::initializeTexture(XERDevice* device)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = device->d3dDefaultGraphicsRS;
	psoDesc.VS = D3D12ShaderBytecode(Shaders::EffectTextureVS.data, Shaders::EffectTextureVS.size);
	psoDesc.PS = D3D12ShaderBytecode(Shaders::EffectTexturePS.data, Shaders::EffectTexturePS.size);
	psoDesc.BlendState = D3D12BlendDesc_NoBlend();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
	psoDesc.DepthStencilState = D3D12DepthStencilDesc_Default();
	psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_SNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	device->d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dPipelineState.uuid(), d3dPipelineState.voidInitRef());
}

void XEREffect::initializePlainSkinned(XERDevice* device)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "JOINTWEIGHTS", 0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "JOINTINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TRANSFORM",	2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = device->d3dDefaultGraphicsRS;
	psoDesc.VS = D3D12ShaderBytecode(Shaders::EffectPlainSkinnedVS.data, Shaders::EffectPlainSkinnedVS.size);
	psoDesc.PS = D3D12ShaderBytecode(Shaders::EffectPlainSkinnedPS.data, Shaders::EffectPlainSkinnedPS.size);
	psoDesc.BlendState = D3D12BlendDesc_NoBlend();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = D3D12RasterizerDesc_Default();
	psoDesc.DepthStencilState = D3D12DepthStencilDesc_Default();
	psoDesc.InputLayout = D3D12InputLayoutDesc(inputElementDescs, countof(inputElementDescs));
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_SNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	device->d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dPipelineState.uuid(), d3dPipelineState.voidInitRef());
}