#pragma once

#include <XLib.Types.h>

namespace XEngine
{
	class XERGeometry;
	class XERMonospacedFont;
	class XERDevice;

	class XEResourceLoader abstract final
	{
	public:
		static void LoadDefaultFont(XERDevice* device, XERMonospacedFont* font);
		//static void LoadTextureFromFile(const char* filename, XERDevice* device, XERTexture* texture);
		static bool LoadGeometryFromFile(const char* filename, XERDevice* device, XERGeometry* geometry);
		//static void LoadDefaultFont(XERDevice* device, XEUIMonospacedFont* font);
	};
}