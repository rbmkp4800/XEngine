#pragma once

namespace XEngine::Render::Internal
{
	struct ShaderData
	{
		const void* data;
		unsigned int size;
	};

	class Shaders abstract final
	{
	public:
		static const ShaderData ScreenQuadVS;
		static const ShaderData LightingPassPS;

		static const ShaderData SceneGeometryPositionOnlyVS;

		static const ShaderData EffectPlainVS;
		static const ShaderData EffectPlainPS;

		static const ShaderData UIColorVS;
		static const ShaderData UIColorPS;
		static const ShaderData UIColorAlphaTextureVS;
		static const ShaderData UIColorAlphaTexturePS;

		static const ShaderData DebugWhitePS;
	};
}