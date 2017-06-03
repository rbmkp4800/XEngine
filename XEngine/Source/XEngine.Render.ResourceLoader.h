#pragma once

#include <XLib.Types.h>

namespace XEngine
{
	class XERMonospacedFont;
	class XERDevice;

	class XERResourceLoader abstract final
	{
	public:
		static void LoadDefaultFont(XERDevice* device, XERMonospacedFont* font);
		//static void LoadTextureFromFile(const char* filename, XERDevice* device, XERTexture* texture);
		//static void LoadGeometryFromFile(const char* filename, XERDevice* device, XERGeometry* geomtry);
		//static void LoadDefaultFont(XERDevice* device, XEUIMonospacedFont* font);
	};
}