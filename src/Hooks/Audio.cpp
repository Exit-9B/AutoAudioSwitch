#include "Audio.h"

#include "AudioEx/DeviceWatcher.h"
#include "AudioEx/Engine.h"
#include "AudioEx/EngineCallback.h"
#include "AudioEx/Prefs.h"
#include "RE/Offset.h"

namespace Hooks
{
	void Audio::Install()
	{
		StartUpHook::Install();
		XAudioHook::Install();
		AudioInitHook::Install();
		ProcessSoundUpdatesHook::Install();
	}

	void Audio::StartUpHook::Install()
	{
		auto hook = REL::Relocation<std::uintptr_t>(RE::Offset::WinMain, 0x15);
		REL::make_pattern<"E8">().match_or_fail(hook.address());

		_originalFunc = SKSE::GetTrampoline().write_call<5>(hook.address(), &Func);
	}

	void Audio::StartUpHook::Func()
	{
		_originalFunc();

		if (const auto main = RE::Main::GetSingleton()) {
			if (main->wnd) {
				AudioEx::DeviceWatcher::Init(main->wnd);
			}
		}
	}

	void Audio::XAudioHook::Install()
	{
		auto hook = REL::Relocation<std::uintptr_t>(RE::Offset::BSXAudio2Audio::Init, 0xB7);
		REL::make_pattern<"FF 15">().match_or_fail(hook.address());

		_originalFunc = *reinterpret_cast<std::uintptr_t*>(
			SKSE::GetTrampoline().write_call<6>(hook.address(), &Func));
	}

	std::int32_t Audio::XAudioHook::Func(
		SKSE::WinAPI::REFCLSID a_rclsid,
		RE::IUnknown* a_unkOuter,
		std::uint32_t a_clsContext,
		SKSE::WinAPI::REFIID a_riid,
		void* a_ppv)
	{
		const auto result = _originalFunc(a_rclsid, a_unkOuter, a_clsContext, a_riid, a_ppv);

		static constinit std::once_flag flag;
		std::call_once(
			flag,
			[a_ppv]()
			{
				const auto vfptr = *reinterpret_cast<std::uintptr_t**>(a_ppv);
				const auto vftable = *vfptr;
				MasteringVoiceHook::Install(vftable);
			});

		return result;
	}

	void Audio::MasteringVoiceHook::Install(std::uintptr_t a_vtbl)
	{
		_originalFunc = REL::Relocation<std::uintptr_t>(a_vtbl).write_vfunc(10, &Func);
	}

	std::int32_t Audio::MasteringVoiceHook::Func(
		RE::IXAudio2* a_xaudio,
		RE::IXAudio2MasteringVoice** a_masteringVoice,
		std::uint32_t a_inputChannels,
		std::uint32_t a_inputSampleRate,
		std::uint32_t a_flags,
		std::uint32_t a_deviceIndex,
		const RE::XAUDIO2_EFFECT_CHAIN* a_effectChain)
	{
		std::uint32_t deviceCount{};
		a_xaudio->GetDeviceCount(&deviceCount);
		logger::debug("{} audio devices available"sv, deviceCount);

		if (AudioEx::Prefs::PreferredDevice) {
			for (const auto i : std::views::iota(0u, deviceCount)) {
				RE::XAUDIO2_DEVICE_DETAILS details{};
				a_xaudio->GetDeviceDetails(i, std::addressof(details));
				if (::wcsncmp(AudioEx::Prefs::PreferredDevice->c_str(), details.DeviceID, 256) ==
					0) {

					logger::trace("Overriding device selection with user preference"sv);
					a_deviceIndex = i;
					break;
				}
			}
		}

		RE::XAUDIO2_DEVICE_DETAILS details{};
		a_xaudio->GetDeviceDetails(a_deviceIndex, std::addressof(AudioEx::Engine::CurrentDevice));
		const wchar_t* const displayName = AudioEx::Engine::CurrentDevice.DisplayName;
		logger::info(
			"Using device at index {}: {}"sv,
			a_deviceIndex,
			SKSE::stl::utf16_to_utf8(std::wstring_view(displayName, ::wcsnlen(displayName, 256)))
				.value_or("<error reading string>"s));

		return _originalFunc(
			a_xaudio,
			a_masteringVoice,
			a_inputChannels,
			a_inputSampleRate,
			a_flags,
			a_deviceIndex,
			a_effectChain);
	}

	void Audio::AudioInitHook::Install()
	{
		auto vtbl = REL::Relocation<std::uintptr_t>(RE::Offset::BSXAudio2Audio::Vtbl);
		_originalFunc = vtbl.write_vfunc(1, &Func);
	}

	bool Audio::AudioInitHook::Func(RE::BSXAudio2Audio* a_audioImpl)
	{
		const auto result = _originalFunc(a_audioImpl);

		if (result) {
			logger::debug("BSXAudio2Audio init succeeded"sv);

			if (const auto XAudio = a_audioImpl->XAudio) {
				XAudio->RegisterForCallbacks(&AudioEx::EngineCallback::Instance);
			}
		}
		else {
			logger::warn("BSXAudio2Audio init failed"sv);
		}

		return result;
	}

	void Audio::ProcessSoundUpdatesHook::Install()
	{
		auto hook =
			REL::Relocation<std::uintptr_t>(RE::Offset::BSAudioManagerThread::ThreadProc, 0x6C);
		REL::make_pattern<"E8">().match_or_fail(hook.address());

		_originalFunc = SKSE::GetTrampoline().write_call<5>(hook.address(), &Func);
	}

	void Audio::ProcessSoundUpdatesHook::Func(RE::BSAudioManager* a_audioManager)
	{
		if (SKSE::WinAPI::GetCurrentThreadID() == a_audioManager->ownerThreadID) {
			if (AudioEx::Engine::retryAudio) {
				AudioEx::Engine::Reset(a_audioManager);
				AudioEx::Engine::retryAudio = false;
				return;
			}
			else {
				if (!AudioEx::Engine::Update()) {
					AudioEx::Engine::retryAudio = true;
					return;
				}
			}
		}

		if (!AudioEx::Engine::retryAudio) {
			_originalFunc(a_audioManager);
		}
	}
}
