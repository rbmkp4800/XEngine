#pragma once

#include <XLib.Types.h>

#pragma pack(push, 1)
namespace XEngine::Formats
{
	class XEGeometryFile abstract final
	{
	public:
		static constexpr uint32 Magic = 0xAABBCCDD; // TODO: replace
		static constexpr uint16 SupportedVersion = 1;

		struct Header
		{
			uint32 magic;
			uint16 version;
			uint16 vertexStride;
			uint32 vertexCount;
			uint32 indexCount;
		};
	};
}
#pragma pack(pop)