#include "XEngine.Render.ResourceLoader.h"

using namespace XEngine;

namespace
{
	struct FileHeader
	{
		uint32 signature;
		uint32 version;
		uint32 vertexComponentCount;
	};
}

//void XERResourceLoader::LoadGeometryFromFile(const char* filename, XERDevice* device, XERGeometry* geomtry)
//{
//
//}