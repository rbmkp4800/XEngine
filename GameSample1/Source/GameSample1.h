#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>
#include <XEngine.Core.Engine.h>
#include <XEngine.Core.Input.h>
#include <XEngine.Core.GeometryResource.h>
#include <XEngine.Render.Scene.h>
#include <XEngine.Render.GBuffer.h>
#include <XEngine.Render.Camera.h>
#include <XEngine.Render.UI.Batch.h>
#include <XEngine.UI.MonospacedFont.h>

namespace GameSample1
{
	class Game :
		public XEngine::Core::GameBase,
		public XEngine::Core::InputHandler
	{
	private:
		XEngine::Render::Scene scene;
		XEngine::Render::GBuffer gBuffer;
		XEngine::Render::EffectHandle plainEffect;
		XEngine::Render::MaterialHandle plainMaterial;

		//XEngine::Core::GeometryResourceManager geometryResourceManager;
		XEngine::Core::GeometryResource cubeGeometryResource;
		XEngine::Core::GeometryResource sphereGeometryResource;
		XEngine::Render::GeometryInstanceHandle cubeGeometryInstance;

		XEngine::Render::UI::Batch uiBatch;
		XEngine::UI::MonospacedFont font;

		XEngine::Render::Camera camera;
		float32x2 cameraRotation = { 0.0f, 0.0f };

	public:
		Game() = default;
		~Game() = default;

		virtual void initialize() override;
		virtual void update(float32 timeDelta) override;

		virtual void onMouseMove(sint16x2 delta) override;
		virtual void onMouseButton(XLib::MouseButton button, bool state) override {}
		virtual void onMouseWheel(float32 delta) override {}
		virtual void onKeyboard(XLib::VirtualKey key, bool state) override;
		virtual void onCloseRequest() override;
	};
}