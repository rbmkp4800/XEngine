#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.System.Window.h>
#include <XLib.System.Threading.h>
#include <XLib.System.Threading.Event.h>

namespace XEngine::Core
{
	class MainWindow : public XLib::NonCopyable
	{
	private:
		XLib::WindowBase window;
		XLib::Thread dispatchThread;
		XLib::Event controlEvent;

	private:
		static uint32 __stdcall DispatchThreadMain(MainWindow* self);
		void dispatchThreadMain();

	public:
		MainWindow() = default;
		~MainWindow() = default;

		void initialize();
		void destroy();

		inline bool isActive();
		inline bool isOpened() { return window.isOpened(); }
	};
}