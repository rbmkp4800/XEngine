#pragma once

#include <XLib.Types.h>

#pragma pack(push, 1)
namespace XEngine::Formats::XEGeometry
{
	static constexpr uint64 FileSignature = 0xAABBCCDDEEFF; // TODO: replace
	static constexpr uint32 SupportedFileVersion = 0;

	struct FileHeader
	{
		uint64 signature;
		uint32 version;
		uint32 vertexStride;
		uint32 vertexCount;
		uint32 indexCount;
	};
}
#pragma pack(pop)