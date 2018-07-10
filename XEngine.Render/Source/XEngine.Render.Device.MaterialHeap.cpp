#include "XEngine.Render.Device.MaterialHeap.h"

#define device this->getDevice()

using namespace XEngine::Render;
using namespace XEngine::Render::Device_;

MaterialHandle MaterialHeap::createMaterial(EffectHandle effect,
	const void* initialConstants, const TextureHandle* intialTextures)
{
	return 0;
}

EffectHandle MaterialHeap::getEffect(MaterialHandle handle) const
{
	return 0;
}

uint64 MaterialHeap::translateHandleToGPUAddress(MaterialHandle handle) const
{
	return 0;
}