#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Platform.COMPtr.h>

#include "XEngine.Render.Base.h"

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandSignature;
struct ID3D12PipelineState;

namespace XLib { struct Matrix3x4; }
namespace XEngine::Render { class Device; }
namespace XEngine::Render::Device_ { class SceneRenderer; }

namespace XEngine::Render
{
	using TransformGroupHandle = uint32;

	struct PointLightDesc
	{
		float32x3 position;
		float32 radius;
		float32x3 color;
	};

	struct DirectionalLightDesc
	{
		float32x3 direction;
		float32x3 color;
	};

	struct GeometryDesc // 20 bytes
	{
		BufferHandle vertexBufferHandle;
		BufferHandle indexBufferHandle;
		uint32 vertexDataOffset;
		uint32 indexDataOffset;
		uint32 indexCount;
		struct
		{
			uint indexIs32Bit : 1;
			uint vertexStride : 7;
			uint vertexDataSize : 24;
		};
	};

	class Scene : public XLib::NonCopyable
	{
		friend Device_::SceneRenderer;

	private:
		static constexpr uint8 directionalLightsLimit = 2;

		struct TransformGroup;
		struct BVHNode;

		struct ShadowCameraTransformConstants;
		
		struct Instance // 28 bytes
		{
			GeometryDesc geometryDesc;
			uint32 baseTransformIndex;
			MaterialHandle material;
		};

		struct CommandListDesc
		{
			EffectHandle effect;
			uint32 arenaBaseSegment;
			uint32 length;
		};

		struct DirectionalLight
		{
			DirectionalLightDesc desc;
			float32x3 shadowVolumeOrigin;
			float32x3 shadowVolumeSize;
			//rectu16 shadowMapRect;
			uint16 dsvDescriptorIndex;
		};

		struct PointLight
		{
			PointLightDesc desc;
			rectu16 shadowMapRect;
		};

	private:
		Device *device = nullptr;

		XLib::Vector<TransformGroup> transformGroups;
		XLib::Vector<BVHNode> bvhNodes;

		XLib::Vector<Instance> instances;
		XLib::Vector<CommandListDesc> commandLists;

		XLib::Platform::COMPtr<ID3D12Resource> d3dTransformBuffer;
		XLib::Platform::COMPtr<ID3D12Resource> d3dCommandListArena;
		XLib::Platform::COMPtr<ID3D12Resource> d3dShadowCameraTransformsCB;
		XLib::Platform::COMPtr<ID3D12Resource> d3dShadowMapAtlas;

		XLib::Matrix3x4 *mappedTransformBuffer = nullptr;
		byte *mappedCommandListArena = nullptr;
		ShadowCameraTransformConstants *mappedShadowCameraTransformsCB = nullptr;

		DirectionalLight directionalLights[directionalLightsLimit] = {};

		uint32 transformBufferSize = 0;
		uint32 allocatedTansformCount = 0;
		uint32 allocatedCommandListArenaSegmentCount = 0;

		uint32 bvhRootIndex = uint32(-1);

		uint16 shadowMapAtlasSRVDescriptorIndex = 0;
		uint16 pointLightCount = 0;
		uint8 directionalLightCount = 0;

	private:
		void populateCommandListForGBufferPass(ID3D12GraphicsCommandList* d3dCommandList,
			ID3D12CommandSignature* d3dICS, bool useEffectPSOs);
		void populateCommandListForShadowPass(ID3D12GraphicsCommandList* d3dCommandList,
			ID3D12CommandSignature* d3dICS, ID3D12PipelineState* d3dPSO);

	public:
		Scene() = default;
		~Scene() = default;

		void initialize(Device& device, uint32 initialTransformBufferSize = 256);
		void destroy();

		TransformGroupHandle createTransformGroup(uint32 size = 1);
		void removeTransformGroup(TransformGroupHandle handle);

		void updateTransform(TransformGroupHandle handle, uint32 index,
			const XLib::Matrix3x4& transform);

		GeometryInstanceHandle createGeometryInstance(const GeometryDesc& geometryDesc,
			MaterialHandle material, TransformGroupHandle transformGroupHandle,
			uint32 transformOffset = 0);
		void removeGeometryInstance(GeometryInstanceHandle handle);

		uint8 createDirectionalLight(const DirectionalLightDesc& desc);
		uint16 createPointLight(const PointLightDesc& desc);

		void updateDirectionalLightDirection(uint8 id, float32x3 direction);
	};
}