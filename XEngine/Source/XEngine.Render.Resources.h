#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12Resource;

namespace XEngine
{
	class XERScene;
	class XERDevice;

	/*namespace Internal
	{
		struct SkeletonJoint
		{
			XLib::Matrix3x4 inverseBindPose;
			uint32 parent;
		};

		struct Skeleton
		{
			SkeletonJoint* joints;
			uint32 jointCount;
		};
	}*/

	class XERGeometry : public XLib::NonCopyable
	{
		friend XERScene;

	private:
		COMPtr<ID3D12Resource> d3dBuffer;
		uint32 vertexCount = 0, vertexStride = 0;
		uint32 indexCount = 0;
		uint32 id = 0;

		//Internal::Skeleton skeleton;

	public:
		void initialize(XERDevice* device, const void* vertices, uint32 vertexCount,
			uint32 vertexStride, const uint32* indices, uint32 indexCount);
	};

	class XERTexture : public XLib::NonCopyable
	{
		friend XERScene;

	private:
		COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;

	public:
		void initialize(XERDevice* device, const void* data, uint32 width, uint32 height);
	};
}