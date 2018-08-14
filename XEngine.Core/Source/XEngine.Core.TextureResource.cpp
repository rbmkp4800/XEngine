#include <XLib.System.File.h>
#include <XLib.Vectors.h>
#include <XLib.Heap.h>
#include <XEngine.Render.Device.h>

#include "XEngine.Core.TextureResource.h"

#include "XEngine.Core.Engine.h"

using namespace XLib;
using namespace XEngine::Core;
using namespace XEngine::Render;

bool TextureResource::createFromFile(const char* filename)
{
	File file;
	if (!file.open(filename, FileAccessMode::Read))
		return false;

	uint16x2 size(0, 0);
	if (!file.read(size))
		return false;

	HeapPtr<uint32> data(size.x * size.y);
	file.read(data, size.x * size.y * 4);

	Render::Device& renderDevice = Engine::GetRenderDevice();
	texture = renderDevice.createTexture(size.x, size.y);
	renderDevice.updateTexture(texture, data, size.x * 4, data);

	return true;
}