#pragma once

#include <XLib.Types.h>

namespace XEngine::Render
{
	using GeometryHandle = uint32;
	using TextureHandle = uint32;
	using EffectHandle = uint16;
	using MaterialHandle = uint16;

	using TransformHandle = uint32;
	using GeometryInstanceHandle = uint32;

	class Device;
	class Scene;
	class SceneRenderer;
}