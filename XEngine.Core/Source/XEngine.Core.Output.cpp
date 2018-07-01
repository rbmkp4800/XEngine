#include <Windows.h>

#include <XLib.Debug.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Event.h>

#include "XEngine.Core.Output.h"

#include "XEngine.Core.Input.h"

// HID usage tables http://www.usb.org/developers/hidpage/Hut1_12v2.pdf (page 26)
#define HID_GENERIC_DESKTOP_USAGE_PAGE	0x01
#define HID_MOUSE_USAGE_ID				0x02
#define HID_KEYBOARD_USAGE_ID			0x06

#define WM_XENGINE_OUTPUT_DESTROY		(WM_USER + 1)

namespace XEngine::Core
{
	class InputProxy abstract final
	{
	public:
		static inline void OnMouseMove(sint16x2 delta)
			{ Input::OnMouseMove(delta); }
		static inline void OnMouseButton(XLib::MouseButton button, bool state)
			{ Input::OnMouseButton(button, state); }
		static inline void OnMouseWheel(float32 delta)
			{ Input::OnMouseWheel(delta); }
		static inline void OnKeyboard(XLib::VirtualKey key, bool state)
			{ Input::OnKeyboard(key, state); }
		static inline void OnCharacter(wchar character)
			{ Input::OnCharacter(character); }
		static inline void OnCloseRequest()
			{ Input::OnCloseRequest(); }
	};
}

using namespace XLib;
using namespace XEngine::Core;

static Thread dispatchThread;
static Event controlEvent;
static HWND hWnd = nullptr;

static bool windowClassRegistered = false;
static bool rawInputRegistered = false;

constexpr wchar windowClassName[] = L"XEngine.OutputWindow";
constexpr wchar windowTitle[] = L"XEngine";

static void HandleRawInput(WPARAM wParam, LPARAM lParam)
{
	if (wParam != RIM_INPUT)
		return;

	RAWINPUT inputBuffer = {};
	UINT inputBufferSize = sizeof(RAWINPUT);

	UINT result = GetRawInputData(HRAWINPUT(lParam), RID_INPUT,
		&inputBuffer, &inputBufferSize, sizeof(RAWINPUTHEADER));

	if (result == -1)
		return;

	if (inputBuffer.header.dwType == RIM_TYPEMOUSE)
	{
		auto& data = inputBuffer.data.mouse;

		if (data.usFlags != 0)
			return;

		if (data.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
			InputProxy::OnMouseButton(MouseButton::Left, true);
		if (data.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
			InputProxy::OnMouseButton(MouseButton::Left, false);

		if (data.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
			InputProxy::OnMouseButton(MouseButton::Middle, true);
		if (data.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
			InputProxy::OnMouseButton(MouseButton::Middle, false);

		if (data.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
			InputProxy::OnMouseButton(MouseButton::Right, true);
		if (data.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
			InputProxy::OnMouseButton(MouseButton::Right, false);

		if (data.usButtonFlags & RI_MOUSE_WHEEL)
			InputProxy::OnMouseWheel(float32(sint16(data.usButtonData)) / 120.0f);

		if (data.lLastX || data.lLastY)
			InputProxy::OnMouseMove({ sint16(data.lLastX), sint16(data.lLastY) });
	}
	else if (inputBuffer.header.dwType == RIM_TYPEKEYBOARD)
	{
		auto& data = inputBuffer.data.keyboard;

		InputProxy::OnKeyboard(VirtualKey(data.VKey), data.Flags == 0);
	}
}

static LRESULT __stdcall WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_XENGINE_OUTPUT_DESTROY:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_CLOSE:
			InputProxy::OnCloseRequest();
			break;

		case WM_INPUT:
			HandleRawInput(wParam, lParam);
			[[fallthrough]];
			// DefWindowProc must be called to cleaunup after WM_INPUT

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

static uint32 __stdcall DispatchThreadMain(void*)
{
	uint32 width = 1440;
	uint32 height = 900;

	RECT rect = { 0, 0, LONG(width), LONG(height) };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	hWnd = CreateWindow(windowClassName, windowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

	controlEvent.set();

	ShowWindow(hWnd, SW_SHOW);

	MSG message = { 0 };
	while (GetMessage(&message, nullptr, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	hWnd = nullptr;

	return 0;
}

void Output::Initialize()
{
	Debug::CrashCondition(dispatchThread.isInitialized(), DbgMsgFmt("already initialized"));

	// Initailize Raw Input
	if (!rawInputRegistered)
	{
		RAWINPUTDEVICE rawInputDevices[2] = {};

		rawInputDevices[0].usUsagePage = HID_GENERIC_DESKTOP_USAGE_PAGE;
		rawInputDevices[0].usUsage = HID_MOUSE_USAGE_ID;
		rawInputDevices[0].dwFlags = 0;
		rawInputDevices[0].hwndTarget = nullptr;

		rawInputDevices[1].usUsagePage = HID_GENERIC_DESKTOP_USAGE_PAGE;
		rawInputDevices[1].usUsage = HID_KEYBOARD_USAGE_ID;
		rawInputDevices[1].dwFlags = 0;
		rawInputDevices[1].hwndTarget = nullptr;

		BOOL result = RegisterRawInputDevices(rawInputDevices, 2, sizeof(RAWINPUTDEVICE));

		rawInputRegistered = true;
	}

	// Register window class
	if (!windowClassRegistered)
	{
		WNDCLASSEX wndClassEx = {};
		wndClassEx.cbSize = sizeof(WNDCLASSEX);
		wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
		wndClassEx.lpfnWndProc = WndProc;
		wndClassEx.cbClsExtra = 0;
		wndClassEx.cbWndExtra = 0;
		wndClassEx.hInstance = GetModuleHandle(nullptr);
		wndClassEx.hIcon = nullptr;
		wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wndClassEx.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
		wndClassEx.lpszMenuName = nullptr;
		wndClassEx.lpszClassName = windowClassName;
		wndClassEx.hIconSm = nullptr;

		RegisterClassEx(&wndClassEx);

		windowClassRegistered = true;
	}

	controlEvent.initialize(false);
	dispatchThread.create(&DispatchThreadMain);

	controlEvent.wait();
}

void Output::Destroy()
{
	PostMessage(hWnd, WM_XENGINE_OUTPUT_DESTROY, 0, 0);

	dispatchThread.wait();

	dispatchThread.destroy();
	controlEvent.destroy();
}