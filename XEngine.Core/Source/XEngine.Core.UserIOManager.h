#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.System.Window.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Event.h>

namespace XEngine::Core
{
	class UserInputHandler : public XLib::NonCopyable
	{
	private:
		UserInputHandler *prevNode = nullptr;
		UserInputHandler *nextNode = nullptr;

	protected:
		virtual void onKeyboard() {}
		virtual void onMouse() {}
		virtual void onChar() {}
		virtual void onCloseRequest() {}
		virtual void onActivationChange() {}

	public:
		UserInputHandler() = default;
		~UserInputHandler();
	};

	class UserIOManager : public XLib::NonCopyable
	{
	private:
		XLib::WindowBase window;
		XLib::Thread dispatchThread;
		XLib::Event controlEvent;

	private:
		static uint32 __stdcall DispatchThreadMain(UserIOManager* self);
		void dispatchThreadMain();

	public:
		UserIOManager() = default;
		~UserIOManager() = default;

		void initialize();
		void destroy();

		void update();

		void addInputHandler(UserInputHandler* handler);
		void removeInputHandler(UserInputHandler* handler);

		bool isOutputActive();
		bool isKeyDown(XLib::VirtualKey key);
		bool isMouseButtonDown(XLib::MouseButton button);

		uint32 getOutputViewCount();
		uint16x2 getOutputViewResolution(uint32 outputViewIndex);
		void* getOutputViewWindowHandle(uint32 outputViewIndex);
	};
}