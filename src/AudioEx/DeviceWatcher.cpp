#include "DeviceWatcher.h"

#include "AudioEx/EngineCallback.h"

enum WindowMessage : std::uint32_t
{
	WM_CLOSE = 0x0010,
	WM_DEVICECHANGE = 0x0219,
};

static constexpr SKSE::WinAPI::GUID DEVINTERFACE_AUDIO_RENDER{
	0xE6327CADL, 0xDCEC, 0x4949, 0xAE, 0x8A, 0x99, 0x1E, 0x97, 0x6A, 0x79, 0xD2,
};

namespace AudioEx
{
	void DeviceWatcher::Init(SKSE::WinAPI::HWND a_wnd)
	{
		assert(a_wnd);

		_WindowProc = reinterpret_cast<decltype(_WindowProc)>(
			SKSE::WinAPI::SetWindowLongPtr(a_wnd, SKSE::WinAPI::GWL::WNDPROC, &WindowProc));

		SKSE::WinAPI::DEV_BROADCAST_DEVICEINTERFACE filter{
			.dbcc_size = sizeof(filter),
			.dbcc_devicetype = SKSE::WinAPI::DBT::DEVTYP::DEVICEINTERFACE,
			.dbcc_classguid = DEVINTERFACE_AUDIO_RENDER,
		};

		NotifyNewAudio = SKSE::WinAPI::RegisterDeviceNotification(
			a_wnd,
			&filter,
			SKSE::WinAPI::DEVICE_NOTIFY::WINDOW_HANDLE);
	}

	std::intptr_t DeviceWatcher::WindowProc(
		SKSE::WinAPI::HWND a_wnd,
		std::uint32_t a_msg,
		std::uintptr_t a_wParam,
		std::intptr_t a_lParam)
	{
		if (a_msg == WM_DEVICECHANGE && a_wParam == SKSE::WinAPI::DBT::DEVICEARRIVAL) {
			const auto dev = reinterpret_cast<SKSE::WinAPI::DEV_BROADCAST_HDR*>(a_lParam);
			if (dev && dev->dbch_devicetype == SKSE::WinAPI::DBT::DEVTYP::DEVICEINTERFACE) {
				const auto devint =
					reinterpret_cast<SKSE::WinAPI::DEV_BROADCAST_DEVICEINTERFACE*>(dev);

				logger::info("New device arrived: {}"sv, devint->dbcc_name);

				if (devint->dbcc_classguid == DEVINTERFACE_AUDIO_RENDER) {

					logger::trace("Reporting new audio device");

					AudioEx::EngineCallback::Instance.OnNewAudioDevice();
					return 1;
				}
			}
		}
		else if (a_msg == WM_CLOSE) {
			NotifyNewAudio.reset();
		}

		return SKSE::WinAPI::CallWindowProc(_WindowProc, a_wnd, a_msg, a_wParam, a_lParam);
	}
}
