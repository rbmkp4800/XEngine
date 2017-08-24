#pragma once

#include <XLib.Types.h>

namespace XEngine::Internal
{
	struct ShaderData
	{
		const void* data;
		uint32 size;
	};

	class Shaders abstract final
	{
	public:
		static ShaderData ScreenQuadVS;
		static ShaderData ClearDefaultUAVxCS;
		static ShaderData DepthBufferDownscalePS;

		static ShaderData LightingPassPS;

		static ShaderData OCxBBoxVS;
		static ShaderData OCxBBoxPS;
		static ShaderData OCxICLUpdateCS;

		static ShaderData DebugPositionOnlyVS;
		static ShaderData DebugWhitePS;

		static ShaderData UIColorVS;
		static ShaderData UIColorPS;
		static ShaderData UIFontVS;
		static ShaderData UIFontPS;

		static ShaderData EffectPlainVS;
		static ShaderData EffectPlainPS;
		static ShaderData EffectPlainSkinnedVS;
		static ShaderData EffectPlainSkinnedPS;
		static ShaderData EffectTextureVS;
		static ShaderData EffectTexturePS;
	};
}