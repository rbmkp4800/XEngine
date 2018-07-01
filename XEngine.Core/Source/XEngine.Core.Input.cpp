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

void Input::OnCloseRequest()
{
	ScopedLock lock(handlersListLock);
	for (InputHandler& handler : handlersList)
		handler.onCloseRequest();
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