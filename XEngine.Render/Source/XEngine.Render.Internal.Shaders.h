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
	};
}