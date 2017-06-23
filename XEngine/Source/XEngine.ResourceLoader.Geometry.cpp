#include <XLib.Heap.h>
#include <XLib.System.File.h>

#include "XEngine.ResourceLoader.h"
#include "XEngine.Formats.XEGeometry.h"
#include "XEngine.Render.Vertices.h"
#include "XEngine.Render.Resources.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Formats::XEGeometry;

bool XEResourceLoader::LoadGeometryFromFile(const char* filename, XERDevice* device, XERGeometry* geometry)
{
	File file;
	if (!file.open(filename, FileAccessMode::Read))
		return false;

	FileHeader header;
	if (!file.read(header))
		return false;

	if (header.signature != FileSignature ||
		header.version != SupportedFileVersion ||
		header.vertexStride != sizeof(VertexBase))
	{
		return false;
	}

	uint64 expectedFileSize = uint64(header.vertexStride) * uint64(header.vertexCount) +
		uint64(header.indexCount) * sizeof(uint32) + sizeof(header);
	if (expectedFileSize != file.getSize())
		return false;

	HeapPtr<VertexBase> vertices(header.vertexCount);
	if (!file.read(vertices, header.vertexStride * header.vertexCount))
		return false;

	HeapPtr<uint32> indices(header.indexCount);
	if (!file.read(indices, header.indexCount * sizeof(uint32)))
		return false;

	geometry->initialize(device, vertices, header.vertexCount, header.vertexStride, indices, header.indexCount);

	return true;
}