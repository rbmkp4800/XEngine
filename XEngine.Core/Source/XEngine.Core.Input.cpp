#include <XLib.System.Threading.Lock.h>

#include "XEngine.Core.Input.h"

using namespace XLib;
using namespace XEngine::Core;

Input::HandlersList Input::handlersList;
static Lock handlersListLock;

void Input::AddHandler(InputHandler* handler)
{
	ScopedLock lock(handlersListLock);
	if (!handler->handlersListItemHook.isHooked())
		handlersList.insert(handler);
}

void Input::RemoveHandler(InputHandler* handler)
{
	ScopedLock lock(handlersListLock);
	if (handler->handlersListItemHook.isHooked())
		handlersList.remove(handler);
}

bool Input::IsKeyDown(XLib::VirtualKey key)
{
	return false;
}

bool Input::IsMouseButtonDown(XLib::MouseButton button)
{
	return false;
}

InputHandler::~InputHandler()
{
	Input::RemoveHandler(this);
}

void Input::OnMouseMove(sint16x2 delta)
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onMouseMove(delta);
}
void Input::OnMouseButton(XLib::MouseButton button, bool state)
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onMouseButton(button, state);
}
void Input::OnMouseWheel(float32 delta)
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onMouseWheel(delta);
}
void Input::OnKeyboard(XLib::VirtualKey key, bool state)
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onKeyboard(key, state);
}
void Input::OnCharacter(wchar character)
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onCharacter(character);
}
void Input::OnCloseRequest()
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onCloseRequest();
}