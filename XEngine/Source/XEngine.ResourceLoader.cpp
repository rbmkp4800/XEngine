#include "XEngine.ResourceLoader.h"
#include "XEngine.Render.UI.h"
#include "XEngine.Render.StaticResources.DefaultFont.h"

using namespace XEngine;

void XEResourceLoader::LoadDefaultFont(XERDevice* device, XERMonospacedFont* font)
{
	font->initializeA1(device, (uint8*)defaultFontBitmapA1, defaultFontWidth, defaultFontHeight,
		defaultFontFirstCharCode, defaultFontCharTableSize);
}