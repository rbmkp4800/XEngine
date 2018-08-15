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
		static const ShaderData DepthBufferDownscalePS;
		static const ShaderData LightingPassPS;

		static const ShaderData SceneGeometryPositionOnlyVS;

		static const ShaderData Effect_NormalVS;
		static const ShaderData Effect_NormalTexcoordVS;
		static const ShaderData Effect_NormalTangentTexcoordVS;
		static const ShaderData Effect_PerMaterialAlbedoRoughtnessMetalnessPS;
		static const ShaderData Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPS;
		static const ShaderData Effect_AlbedoNormalRoughtnessMetalnessTexturePS;

		static const ShaderData UIColorVS;
		static const ShaderData UIColorPS;
		static const ShaderData UIColorAlphaTextureVS;
		static const ShaderData UIColorAlphaTexturePS;

		static const ShaderData DebugWhitePS;
	};
}