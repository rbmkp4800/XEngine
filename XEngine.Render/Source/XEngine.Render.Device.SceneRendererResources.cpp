#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Device.SceneRendererResources.h"

using namespace XLib;
using namespace XLib::Platform;
using namespace XEngine::Render::Device_;

void SceneRendererResources::inititalize()
{
	ID3D12Device *d3dDevice = ;

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
}

void SceneRendererResources::destroy()
{

}