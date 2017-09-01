#pragma once

#include "Util.COMPtr.h"

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

struct ID3D12Resource;

namespace XEngine
{
	class XERScene;
	class XERUIRender;
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
		friend XERUIRender;

	private:
		COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;

	public:
		void initialize(XERDevice* device, const void* data, uint32 width, uint32 height);
	};

	class XERMonospacedFont : public XLib::NonCopyable
	{
		friend XERUIRender;

	private:
		COMPtr<ID3D12Resource> d3dTexture;
		uint32 srvDescriptor;
		uint8 charWidth, charHeight;
		uint8 firstCharCode, charTableSize;

	public:
		void initializeA8(XERDevice* device, uint8* bitmapA8, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);
		void initializeA1(XERDevice* device, uint8* bitmapA1, uint8 charWidth,
			uint8 charHeight, uint8 firstCharCode, uint8 charTableSize);

		inline uint8 getCharWidth() { return charWidth; }
		inline uint8 getCharHeight() { return charHeight; }
	};
}