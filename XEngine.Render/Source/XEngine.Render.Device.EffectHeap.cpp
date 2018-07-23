#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.EffectHeap.h"

#include "XEngine.Render.Device.h"
#include "XEngine.Render.Internal.Shaders.h"
#include "XEngine.Render.ClassLinkage.h"

#define device this->getDevice()

using namespace XEngine::Render;
using namespace XEngine::Render::Internal;
using namespace XEngine::Render::Device_;

void EffectHeap::initialize()
{

}

EffectHandle EffectHeap::createEffect_plain()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = device.sceneRenderer.getGBufferPassD3DRS();
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
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_SNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	XLib::Platform::COMPtr<ID3D12PipelineState>& d3dPSO = d3dPSOs[effectCount];
	device.d3dDevice->CreateGraphicsPipelineState(&psoDesc, d3dPSO.uuid(), d3dPSO.voidInitRef());

	EffectHandle result = EffectHandle(effectCount);
	effectCount++;
	return result;
}

ID3D12PipelineState* EffectHeap::getPSO(EffectHandle handle)
{
	return d3dPSOs[handle];
}

uint32 EffectHeap::getMaterialConstantsSize(EffectHandle handle) const
{
	return 32;
}