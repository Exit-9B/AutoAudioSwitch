#pragma once

namespace AudioEx
{
	namespace Prefs
	{
		inline constinit std::optional<std::wstring> PreferredDevice;

		[[nodiscard]] std::optional<std::filesystem::path> GetPath();
		void Load();
		void Reset();
		void SetPreferredDevice(std::optional<std::wstring>&& a_deviceID);
	}
}
