#pragma once

#include <XLib.Util.h>

#define XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(tagetClass, targetDeviceMember)	\
	XEngine::Render::Device& XEngine::Render::Device_::tagetClass::getDevice() {	\
		uintptr uintThis = uintptr(this);											\
		uintptr thisOffset = offsetof(XEngine::Render::Device, targetDeviceMember);	\
		return *to<XEngine::Render::Device*>(uintThis - thisOffset);				}

XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(Uploader,		uploader);
XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(SceneRenderer,	sceneRenderer);
XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(EffectHeap,	effectHeap);
//XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(MaterialHeap,	materialHeap);
XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(BufferHeap,	bufferHeap);
//XENGINE_RENDER_DEVICE_CLASS_LINKAGE_DECL(TextureHeap,	textureHeap);
