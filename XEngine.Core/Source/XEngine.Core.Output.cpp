#include <Windows.h>

#include <XLib.Debug.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Event.h>

#include "XEngine.Core.Output.h"

#include "XEngine.Core.Input.h"

namespace XEngine::Core
{
	class InputProxy abstract final
	{
	public:
		//static inline void OnKeyboard() { Input::OnKeyboard(); }
		//static inline void OnMouse() { Input::OnMouse(); }
		//static inline void OnChar() { Input::OnChar(); }
		//static inline void OnFocusChange() { Input::OnFocusChange(); }
		static inline void OnCloseRequest() { Input::OnCloseRequest(); }
	};
}

using namespace XLib;
using namespace XEngine::Core;

static Thread dispatchThread;
static Event controlEvent;
static HWND hWnd = nullptr;

static LRESULT __stdcall WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		break;

	case WM_CLOSE:
		InputProxy::OnCloseRequest();
		break;

	case WM_INPUT:
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
}

static uint32 __stdcall DispatchThreadMain(void*)
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	constexpr wchar windowClassName[] = L"XEngine.OutputWindow";

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = windowClassName;
	wcex.hIconSm = nullptr;
	RegisterClassEx(&wcex);

	uint32 width = 1440;
	uint32 height = 900;

	RECT rect = { 0, 0, LONG(width), LONG(height) };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	hWnd = CreateWindow(windowClassName, L"XEngine", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
		nullptr, nullptr, hInstance, nullptr);

	controlEvent.set();

	ShowWindow(hWnd, SW_SHOW);

	MSG message = { 0 };
	while (GetMessage(&message, nullptr, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return 0;
}

void Output::Initialize()
{
	Debug::CrashCondition(dispatchThread.isInitialized(), DbgMsgFmt("already initialized"));

	controlEvent.initialize(false);
	dispatchThread.create(&DispatchThreadMain);

	controlEvent.wait();
}