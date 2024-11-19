#include "Options.h"

#include "AudioEx/Engine.h"
#include "AudioEx/Prefs.h"
#include "RE/Offset.h"

namespace Hooks
{
	// Magic number taken from GUID `DEVINTERFACE_AUDIO_RENDER`.
	static constexpr std::uint32_t ID_OutputDevice = 0xE6327CAD;

	void Options::Install()
	{
		AudioOptionsHook::Install();
		OptionChangeHook::Install();
	}

	void Options::AudioOptionsHook::Install()
	{
		auto hook =
			REL::Relocation<std::uintptr_t>(RE::Offset::Journal_SystemTab::ProcessCallbacks, 0xC7);
		REL::make_pattern<"4C 8D 05">().match_or_fail(hook.address());

		_originalFunc =
			util::write_lea<util::Reg::R8>(SKSE::GetTrampoline(), hook.address(), &Func);
	}

	void Options::AudioOptionsHook::Func(const RE::FxDelegateArgs& a_params)
	{
		_originalFunc(a_params);

		RE::GFxValue entryList = a_params[0];
		if (!entryList.IsArray()) {
			return;
		}

		const auto audioImpl = RE::BSXAudio2GameSound::GetAudioImplementation();
		if (!audioImpl || !audioImpl->XAudio) {
			return;
		}

		RE::GFxValue deviceOption;
		a_params.GetMovie()->CreateObject(&deviceOption);

		deviceOption.SetMember("text", "$Output Device");
		deviceOption.SetMember("movieType", 1);  // SystemPage doesn't care lol
		deviceOption.SetMember("ID", ID_OutputDevice);

		RE::GFxValue options;
		a_params.GetMovie()->CreateArray(&options);

		options.PushBack("$Auto");

		std::uint32_t deviceCount{};
		audioImpl->XAudio->GetDeviceCount(&deviceCount);

		std::uint32_t currentDevice = 0;
		for (const auto i : std::views::iota(0u, deviceCount)) {
			RE::XAUDIO2_DEVICE_DETAILS details{};
			audioImpl->XAudio->GetDeviceDetails(i, &details);
			options.PushBack(
				std::wstring_view(details.DisplayName, ::wcsnlen(details.DisplayName, 256)));

			if (currentDevice == 0 && AudioEx::Prefs::PreferredDevice) {
				if (::wcsncmp(AudioEx::Prefs::PreferredDevice->c_str(), details.DeviceID, 256) ==
					0) {

					currentDevice = 1 + i;
				}
			}
		}

		deviceOption.SetMember("options", options);

		deviceOption.SetMember("value", currentDevice);
		deviceOption.SetMember("defaultVal", 0);

		entryList.PushBack(deviceOption);
	}

	void Options::OptionChangeHook::Install()
	{
		auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::Journal_SystemTab::ProcessCallbacks,
			0x112);
		REL::make_pattern<"4C 8D 05">().match_or_fail(hook.address());

		_originalFunc =
			util::write_lea<util::Reg::R8>(SKSE::GetTrampoline(), hook.address(), &Func);
	}

	void Options::OptionChangeHook::Func(const RE::FxDelegateArgs& a_params)
	{
		const auto ID = static_cast<std::uint32_t>(a_params[0].GetNumber());
		if (ID == ID_OutputDevice) {
			const auto value = static_cast<std::uint32_t>(a_params[1].GetNumber());

			const auto audioImpl = RE::BSXAudio2GameSound::GetAudioImplementation();
			if (!audioImpl || !audioImpl->XAudio) {
				return;
			}

			std::uint32_t deviceCount{};
			audioImpl->XAudio->GetDeviceCount(&deviceCount);

			wchar_t deviceID[256]{};

			if (value == 0) {
				for (const auto i : std::views::iota(0u, deviceCount)) {
					RE::XAUDIO2_DEVICE_DETAILS details{};
					audioImpl->XAudio->GetDeviceDetails(i, &details);

					if (details.Role == RE::XAUDIO2_DEVICE_ROLE::DefaultGameDevice) {
						std::ranges::copy(details.DeviceID, deviceID);
						break;
					}
					else if (i == 0) {
						std::ranges::copy(details.DeviceID, deviceID);
					}
				}

				AudioEx::Prefs::SetPreferredDevice(std::nullopt);
			}
			else {
				RE::XAUDIO2_DEVICE_DETAILS details{};
				audioImpl->XAudio->GetDeviceDetails(value - 1, &details);
				std::ranges::copy(details.DeviceID, deviceID);

				AudioEx::Prefs::SetPreferredDevice(
					std::wstring(details.DeviceID, ::wcsnlen(details.DeviceID, 256)));
			}

			if (::wcsncmp(deviceID, AudioEx::Engine::CurrentDevice.DeviceID, 256) != 0) {

				logger::info("Preferred audio device changed"sv);
				AudioEx::Engine::retryAudio = true;
			}
		}
		else {
			_originalFunc(a_params);
		}
	}
}
