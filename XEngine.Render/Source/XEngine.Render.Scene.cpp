#include <d3d12.h>

#include <XLib.Util.h>
#include <XLib.Math.Matrix3x4.h>
#include <XLib.Math.Matrix4x4.h>
#include <XLib.Platform.D3D12.Helpers.h>

#include "XEngine.Render.Scene.h"

#include "XEngine.Render.Device.h"

using namespace XLib;
using namespace XEngine::Render;
using namespace XEngine::Render::Internal;

static constexpr uint32 commandListArenaSegmentSize = 0x4000;
static constexpr uint16 shadowMapDim = 2048;

static constexpr uint32 bvhLeafFlag = 0x80'00'00'00;

#pragma pack(push, 1)

namespace
{
	struct IndirectCommand
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		uint32 baseTransformIndex;
		uint32 materialIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};
}

#pragma pack(pop)

namespace
{
	struct AABB
	{
		float32x3 minPoint;
		float32x3 maxPoint;

		inline void fill(const AABB& a, const AABB& b)
		{
			minPoint.x = min<float32>(a.minPoint.x, b.minPoint.x);
			minPoint.y = min<float32>(a.minPoint.y, b.minPoint.y);
			minPoint.z = min<float32>(a.minPoint.z, b.minPoint.z);
			maxPoint.x = max<float32>(a.maxPoint.x, b.maxPoint.x);
			maxPoint.y = max<float32>(a.maxPoint.y, b.maxPoint.y);
			maxPoint.z = max<float32>(a.maxPoint.z, b.maxPoint.z);
		}
	};
}

struct Scene::TransformGroup // 32 bytes
{
	AABB aabb;
	uint32 baseTransformIndex;
	uint32 parentBVHNode;
};

struct Scene::BVHNode // 36 bytes
{
	AABB aabb;
	uint32 children[2];
	uint32 parent;
};

// public ===================================================================================//

void Scene::initialize(Device& device, uint32 initialTransformBufferSize)
{
	this->device = &device;
	transformBufferSize = initialTransformBufferSize;

	ID3D12Device *d3dDevice = device.d3dDevice;

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(sizeof(Matrix3x4) * transformBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dTransformBuffer.uuid(), d3dTransformBuffer.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Buffer(commandListArenaSegmentSize * 4),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		d3dCommandListArena.uuid(), d3dCommandListArena.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R16_TYPELESS, shadowMapDim, shadowMapDim,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 1),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&D3D12ClearValue_DepthStencil(DXGI_FORMAT_D16_UNORM, 1.0f),
		d3dShadowMapAtlas.uuid(), d3dShadowMapAtlas.voidInitRef());

	d3dDevice->CreateCommittedResource(
		&D3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&D3D12ResourceDesc_Texture2D(DXGI_FORMAT_R16_TYPELESS,
			pointLightShadowMapDim, pointLightShadowMapDim,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 1, 6),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&D3D12ClearValue_DepthStencil(DXGI_FORMAT_D16_UNORM, 1.0f),
		d3dPointLightShadowMaps.uuid(), d3dPointLightShadowMaps.voidInitRef());

	d3dTransformBuffer->Map(0, &D3D12Range(), to<void**>(&mappedTransformBuffer));
	d3dCommandListArena->Map(0, &D3D12Range(), to<void**>(&mappedCommandListArena));

	srvDescriptorsBaseIndex = device.srvHeap.allocate(2);

	d3dDevice->CreateShaderResourceView(d3dShadowMapAtlas,
		&D3D12ShaderResourceViewDesc_Texture2D(DXGI_FORMAT_R16_UNORM),
		device.srvHeap.getCPUHandle(srvDescriptorsBaseIndex + 0));
	d3dDevice->CreateShaderResourceView(d3dPointLightShadowMaps,
		&D3D12ShaderResourceViewDesc_Texture2DArray(DXGI_FORMAT_R16_UNORM, 0, 6),
		device.srvHeap.getCPUHandle(srvDescriptorsBaseIndex + 1));
}

void Scene::destroy()
{

}

TransformGroupHandle Scene::createTransformGroup(uint32 size)
{
	uint32 baseTransformIndex = allocatedTansformCount;
	allocatedTansformCount += size;
	
	TransformGroupHandle result = TransformGroupHandle(transformGroups.getSize());
	TransformGroup &group = transformGroups.allocateBack();
	//group.aabb.fill();
	group.baseTransformIndex = baseTransformIndex;
	group.parentBVHNode = uint32(-1);

	if (bvhNodes.isEmpty())
	{
		if (bvhRootIndex == uint32(-1))
		{
			// Empty scene
			bvhRootIndex = uint32(result);
		}
		else
		{
			// No BVH nodes allocated, single existing transform group -> create BVH
			TransformGroup& existingGroup = transformGroups[bvhRootIndex];

			BVHNode &node = bvhNodes.allocateBack();
			node.aabb.fill(group.aabb, existingGroup.aabb);
			node.children[0] = bvhRootIndex | bvhLeafFlag;
			node.children[1] = uint32(result) | bvhLeafFlag;
			node.parent = uint32(-1);
		}
	}
	else
	{
		BVHNode &root = bvhNodes[bvhRootIndex];

		uint32 nodeIndex = bvhNodes.getSize();
		BVHNode &node = bvhNodes.allocateBack();
		node.aabb.fill(root.aabb, group.aabb);
		node.children[0] = bvhRootIndex;
		node.children[1] = nodeIndex;
		node.parent = uint32(-1);

		root.parent = nodeIndex;
	}

	return result;
}

void Scene::updateTransform(TransformGroupHandle handle, uint32 index,
	const XLib::Matrix3x4& transform)
{
	uint32 globalTransformIndex = transformGroups[handle].baseTransformIndex + index;
	mappedTransformBuffer[globalTransformIndex] = transform;
}

GeometryInstanceHandle Scene::createGeometryInstance(const GeometryDesc& geometryDesc,
	MaterialHandle material, TransformGroupHandle transformGroupHandle,	uint32 transformOffset)
{
	TransformGroup &transformGroup = transformGroups[transformGroupHandle];
	uint32 baseTransformIndex = transformGroup.baseTransformIndex + transformOffset;

	GeometryInstanceHandle result = GeometryInstanceHandle(instances.getSize());

	{
		Instance &instance = instances.allocateBack();
		instance.geometryDesc = geometryDesc;
		instance.baseTransformIndex = baseTransformIndex;
		instance.material = material;
	}

	{
		EffectHandle effect = device->materialHeap.getEffect(material);

		CommandListDesc *commandListDesc = nullptr;
		for (CommandListDesc& i : commandLists)
		{
			if (i.effect == effect)
			{
				commandListDesc = &i;
				break;
			}
		}

		if (!commandListDesc)
		{
			commandListDesc = &commandLists.allocateBack();
			commandListDesc->effect = effect;
			commandListDesc->arenaBaseSegment = allocatedCommandListArenaSegmentCount;
			commandListDesc->length = 0;
			allocatedCommandListArenaSegmentCount++;
		}

		uint32 commandIndex = commandListDesc->length;
		commandListDesc->length++;

		IndirectCommand *mappedCommandList = to<IndirectCommand*>(mappedCommandListArena +
			commandListDesc->arenaBaseSegment * commandListArenaSegmentSize);

		IndirectCommand &command = mappedCommandList[commandIndex];
			// VBV
		command.vertexBufferView.BufferLocation =
			device->bufferHeap.getBufferGPUAddress(geometryDesc.vertexBufferHandle) + geometryDesc.vertexDataOffset;
		command.vertexBufferView.SizeInBytes = geometryDesc.vertexDataSize;
		command.vertexBufferView.StrideInBytes = geometryDesc.vertexStride;
			// IBV
		command.indexBufferView.BufferLocation =
			device->bufferHeap.getBufferGPUAddress(geometryDesc.indexBufferHandle) + geometryDesc.indexDataOffset;
		command.indexBufferView.SizeInBytes = geometryDesc.indexCount * (geometryDesc.indexIs32Bit ? 4 : 2);
		command.indexBufferView.Format = geometryDesc.indexIs32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			// Constats
		command.baseTransformIndex = baseTransformIndex;
		command.materialIndex = uint32(device->materialHeap.getMaterialConstantsTableEntryIndex(material));
			// Draw arguments
		command.drawIndexedArguments.IndexCountPerInstance = geometryDesc.indexCount;
		command.drawIndexedArguments.InstanceCount = 1;
		command.drawIndexedArguments.StartIndexLocation = 0;
		command.drawIndexedArguments.BaseVertexLocation = 0;
		command.drawIndexedArguments.StartInstanceLocation = 0;
	}

	return result;
}

uint8 Scene::createDirectionalLight(const DirectionalLightDesc& desc)
{
	// TODO: handle directional lights limit

	const uint8 result = directionalLightCount;
	directionalLightCount++;

	DirectionalLight &light = directionalLights[result];
	light.desc = desc;
	light.shadowVolumeOrigin = VectorMath::Normalize(desc.direction) * -20.0f;
	light.shadowVolumeSize = { 40.0f, 40.0f, 40.0f };
	light.shadowMapDim = shadowMapDim;
	light.dsvDescriptorIndex = device->dsvHeap.allocate(1);

	device->d3dDevice->CreateDepthStencilView(d3dShadowMapAtlas,
		&D3D12DepthStencilViewDesc_Texture2D(DXGI_FORMAT_D16_UNORM),
		device->dsvHeap.getCPUHandle(light.dsvDescriptorIndex));

	return result;
}

uint16 Scene::createPointLight(const PointLightDesc& desc)
{
	// TODO: handle point lights limit

	const uint16 result = pointLightCount;
	pointLightCount++;

	PointLight &light = pointLights[result];
	light.desc = desc;
	light.dsvDescriptorsBaseIndex = device->dsvHeap.allocate(6);

	for (uint16 i = 0; i < 6; i++)
	{
		device->d3dDevice->CreateDepthStencilView(d3dPointLightShadowMaps,
			&D3D12DepthStencilViewDesc_Texture2DArray(DXGI_FORMAT_D16_UNORM, 0, i, 1),
			device->dsvHeap.getCPUHandle(light.dsvDescriptorsBaseIndex + i));
	}

	return result;
}

void Scene::updateDirectionalLightDirection(uint8 id, float32x3 direction)
{
	// TODO: handle wrong id

	directionalLights[id].desc.direction = direction;
	directionalLights[id].shadowVolumeOrigin = VectorMath::Normalize(direction) * -16.0f;
}

// private ==================================================================================//

void Scene::populateCommandListForGBufferPass(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12CommandSignature* d3dICS, bool useEffectPSOs)
{
	d3dCommandList->SetGraphicsRootShaderResourceView(3, d3dTransformBuffer->GetGPUVirtualAddress());

	for (CommandListDesc& commandList : commandLists)
	{
		if (useEffectPSOs)
		{
			ID3D12PipelineState *d3dPSO = device->materialHeap.getEffectPSO(commandList.effect);
			d3dCommandList->SetPipelineState(d3dPSO);
		}

		d3dCommandList->SetGraphicsRootConstantBufferView(2,
			device->materialHeap.getMaterialConstantsTableGPUAddress(commandList.effect));

		d3dCommandList->ExecuteIndirect(d3dICS, commandList.length, d3dCommandListArena,
			commandList.arenaBaseSegment * commandListArenaSegmentSize, nullptr, 0);
	}
}

void Scene::populateCommandListForShadowPass(ID3D12GraphicsCommandList* d3dCommandList,
	ID3D12CommandSignature* d3dICS)
{
	d3dCommandList->SetGraphicsRootShaderResourceView(3, d3dTransformBuffer->GetGPUVirtualAddress());

	for (CommandListDesc& commandList : commandLists)
	{
		d3dCommandList->ExecuteIndirect(d3dICS, commandList.length, d3dCommandListArena,
			commandList.arenaBaseSegment * commandListArenaSegmentSize, nullptr, 0);
	}
}