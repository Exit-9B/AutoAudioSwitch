#pragma once

namespace AudioEx
{
	class DeviceWatcher final
	{
	public:
		static void Init(SKSE::WinAPI::HWND a_wnd);

	private:
		static std::intptr_t WindowProc(
			SKSE::WinAPI::HWND a_wnd,
			std::uint32_t a_msg,
			std::uintptr_t a_wParam,
			std::intptr_t a_lParam);

		inline static SKSE::WinAPI::ScopedDeviceNotify NotifyNewAudio;
		inline static SKSE::WinAPI::WNDPROC _WindowProc;
	};
}
