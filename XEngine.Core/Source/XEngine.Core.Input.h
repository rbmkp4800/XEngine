#pragma once

#include <XLib.Containers.IntrusiveDoublyLinkedList.h>
#include <XLib.System.Window.h>

namespace XEngine::Core { class Input; }

namespace XEngine::Core
{
	class InputHandler abstract : public XLib::NonCopyable
	{
		friend Input;

	private:
		XLib::IntrusiveDoublyLinkedListItemHook handlersListItemHook;

	protected:
		virtual void onKeyboard() {}
		virtual void onMouse() {}
		virtual void onChar() {}
		virtual void onFocusChange() {}
		virtual void onCloseRequest() {}

	public:
		InputHandler() = default;
		~InputHandler();
	};

	class Input abstract final
	{
		friend class InputProxy;

	private:
		using HandlersList = XLib::IntrusiveDoublyLinkedList<
			InputHandler, &InputHandler::handlersListItemHook>;

		static HandlersList handlersList;

	private:
		static void OnKeyboard();
		static void OnMouse();
		static void OnChar();
		static void OnFocusChange();
		static void OnCloseRequest();

	public:
		static void AddHandler(InputHandler* handler);
		static void RemoveHandler(InputHandler* handler);

		static bool IsKeyDown(XLib::VirtualKey key);
		static bool IsMouseButtonDown(XLib::MouseButton button);
	};
}