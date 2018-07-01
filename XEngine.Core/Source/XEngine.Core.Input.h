#pragma once

#include <XLib.Containers.IntrusiveDoubleLinkedList.h>
#include <XLib.System.Window.h>

namespace XEngine::Core
{
	class InputHandler abstract : public XLib::NonCopyable
	{
	private:

	protected:
		virtual void onKeyboard() {}
		virtual void onMouse() {}
		virtual void onChar() {}
		virtual void onCloseRequest() {}
		virtual void onActivationChange() {}

	public:
		InputHandler() = default;
		~InputHandler();
	};

	class Input abstract final
	{
	public:
		static void AddHandler(InputHandler* handler);
		static void RemoveHandler(InputHandler* handler);

		static bool IsKeyDown(XLib::VirtualKey key);
		static bool IsMouseButtonDown(XLib::MouseButton button);
	};
}