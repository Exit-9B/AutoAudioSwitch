#include "Options.h"

#include "RE/Offset.h"

namespace Hooks
{
	void Options::Install()
	{
		AudioOptionsHook::Install();
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
		deviceOption.SetMember("ID", 0xE632'7CADULL);

		RE::GFxValue options;
		a_params.GetMovie()->CreateArray(&options);

		std::uint32_t deviceCount{};
		audioImpl->XAudio->GetDeviceCount(&deviceCount);
		for (const auto i : std::views::iota(0u, deviceCount)) {
			RE::XAUDIO2_DEVICE_DETAILS details{};
			audioImpl->XAudio->GetDeviceDetails(i, &details);
			options.PushBack(details.DisplayName);
		}

		deviceOption.SetMember("options", options);

		deviceOption.SetMember("value", 0);
		deviceOption.SetMember("defaultVal", 0);

		entryList.PushBack(deviceOption);
	}
}
