#include "Prefs.h"

#include <SimpleIni.h>

namespace AudioEx
{
	std::optional<std::filesystem::path> Prefs::GetPath()
	{
		auto path = logger::log_directory();
		if (path) {
			*path /= fmt::format("{}.ini"sv, Plugin::NAME);
		}

		return path;
	}

	void Prefs::Load()
	{
		const auto path = GetPath();
		if (!path) {
			logger::warn("Failed to find preferences directory; using default values"sv);
			Reset();
			return;
		}

		::CSimpleIniW ini{};
		ini.SetUnicode();
		const ::SI_Error error = ini.LoadFile(path->c_str());
		if (error < 0) {
			logger::warn("Failed to parse preferences"sv);
			return;
		}

		const auto preferredDevice = ini.GetValue(L"Audio", L"PreferredDevice");
		if (preferredDevice && preferredDevice[0]) {
			SetPreferredDevice(preferredDevice);
		}
		else {
			SetPreferredDevice(std::nullopt);
		}
	}

	void Prefs::Reset()
	{
		PreferredDevice = std::nullopt;
	}

	void Prefs::SetPreferredDevice(std::optional<std::wstring>&& a_deviceID)
	{
		if (const auto deviceID_utf8 =
				a_deviceID ? SKSE::stl::utf16_to_utf8(*a_deviceID) : "Auto"s) {
			logger::info("Preferred device: {}"sv, *deviceID_utf8);
		}

		PreferredDevice = std::move(a_deviceID);

		const auto path = GetPath();
		if (!path) {
			logger::warn("Failed to find preferences directory; not saving"sv);
			return;
		}

		::CSimpleIniW ini{};
		ini.SetUnicode();

		std::error_code ec;
		if (std::filesystem::directory_entry(*path, ec).exists(ec)) {
			const ::SI_Error error = ini.LoadFile(path->c_str());
			if (error < 0) {
				logger::warn("Failed to parse preferences"sv);
			}
		}

		if (PreferredDevice) {
			ini.SetValue(L"Audio", L"PreferredDevice", PreferredDevice->c_str());
		}
		else {
			ini.Delete(L"Audio", L"PreferredDevice");
		}

		const ::SI_Error error = ini.SaveFile(path->c_str());
		if (error < 0) {
			logger::warn("Failed to save preferences"sv);
		}
	}
}
