#include "Engine.h"

#include "EngineCallback.h"
#include "RE/Offset.h"

struct WAIT
{
	enum Result : std::uint32_t
	{
		ABANDONED = 0x00000080L,
		IO_COMPLETION = 0x000000C0L,
		OBJECT_0 = 0x00000000L,
		TIMEOUT = 0x00000102L,
		FAILED = 0xFFFFFFFF,
	};
};

namespace AudioEx
{
	static void ClearSounds()
	{
		const auto audioManager = RE::BSAudioManager::GetSingleton();
		const auto audio = RE::BSAudioManager::QPlatformInstance();

		audioManager->KillAll();
		for (const auto& [soundID, gameSound] : audioManager->activeSounds) {
			audio->ReleaseGameSound(gameSound);
		}
		audioManager->activeSounds.clear();

		for (const auto& [soundID, soundInfo] : audioManager->stateMap) {
			delete soundInfo;
		}
		audioManager->stateMap.clear();
		audioManager->movingSounds.clear();
		audioManager->objectOutputOverrides.clear();

		audioManager->ClearCache();
	}

	static void KillAudioMonitors()
	{
		auto& bInitialized =
			*REL::Relocation<bool*>(STATIC_OFFSET(BGSAudioMonitors::bInitialized));
		if (!bInitialized) {
			return;
		}

		const auto audio = RE::BSAudioManager::QPlatformInstance();

		const auto& uiSmallRumbleMonitorID = *REL::Relocation<std::uint32_t*>(
			STATIC_OFFSET(BGSAudioMonitors::uiSmallRumbleMonitorID));
		const auto& uiBigRumbleMonitorID = *REL::Relocation<std::uint32_t*>(
			STATIC_OFFSET(BGSAudioMonitors::uiBigRumbleMonitorID));

		audio->ReleaseMonitor(uiSmallRumbleMonitorID);
		audio->ReleaseMonitor(uiBigRumbleMonitorID);

		const auto& NoMonitor = *REL::Relocation<RE::BSAudioMonitor::Receiver*>(
			STATIC_OFFSET(BSAudioMonitor::Receiver::NoMonitor));

		auto& smallAmp = *REL::Relocation<RE::BSAudioMonitor::Receiver*>(
			STATIC_OFFSET(BGSAudioMonitors::smallAmp));
		smallAmp = NoMonitor;

		auto& bigAmp = *REL::Relocation<RE::BSAudioMonitor::Receiver*>(
			STATIC_OFFSET(BGSAudioMonitors::bigAmp));
		bigAmp = NoMonitor;

		bInitialized = false;
	}

	static void InitAudioMonitors()
	{
		const auto audio = RE::BSAudioManager::QPlatformInstance();

		auto& uiSmallRumbleMonitorID = *REL::Relocation<std::uint32_t*>(
			STATIC_OFFSET(BGSAudioMonitors::uiSmallRumbleMonitorID));
		auto& uiBigRumbleMonitorID = *REL::Relocation<std::uint32_t*>(
			STATIC_OFFSET(BGSAudioMonitors::uiBigRumbleMonitorID));

		uiSmallRumbleMonitorID = audio->CreateMonitor();
		uiBigRumbleMonitorID = audio->CreateMonitor();

		*REL::Relocation<RE::BSAudioMonitor::Receiver*>(STATIC_OFFSET(
			BGSAudioMonitors::smallAmp)) = audio->GetReceiver(uiSmallRumbleMonitorID);
		*REL::Relocation<RE::BSAudioMonitor::Receiver*>(STATIC_OFFSET(BGSAudioMonitors::bigAmp)) =
			audio->GetReceiver(uiBigRumbleMonitorID);

		auto& bInitialized =
			*REL::Relocation<bool*>(STATIC_OFFSET(BGSAudioMonitors::bInitialized));
		bInitialized = true;
	}

	static void ClearMonitors()
	{
		auto& activeMonitors = *REL::Relocation<RE::BSTSmallArray<RE::XAudio2Monitor, 8>*>(
			STATIC_OFFSET(BSXAudio2Audio::ActiveMonitorsA));
		auto& inactiveMonitors = *REL::Relocation<RE::BSTSmallArray<RE::XAudio2Monitor, 8>*>(
			STATIC_OFFSET(BSXAudio2Audio::InactiveMonitorsA));

		auto& monitorLock =
			*REL::Relocation<RE::BSSpinLock*>(STATIC_OFFSET(BSXAudio2Audio::MonitorLock));

		RE::BSSpinLockGuard locker{ monitorLock };
		for (auto& monitor : activeMonitors) {
			if (monitor.submixVoice) {
				monitor.submixVoice->DestroyVoice();
			}
			if (monitor.monitorAPO) {
				logger::warn(
					"Monitor {} is still active, forcing release"sv,
					std::distance(activeMonitors.data(), std::addressof(monitor)));
				delete monitor.monitorAPO;
			}
			monitor = { nullptr, nullptr };
		}
		activeMonitors.clear();

		for (auto& monitor : inactiveMonitors) {
			if (monitor.submixVoice) {
				monitor.submixVoice->DestroyVoice();
			}
			if (monitor.monitorAPO) {
				delete monitor.monitorAPO;
			}
			monitor = { nullptr, nullptr };
		}
		inactiveMonitors.clear();
	}

	static void Shutdown(RE::BSXAudio2Audio* a_audio)
	{
		if (a_audio->XAudio) {
			a_audio->XAudio->StopEngine();
		}

		ClearMonitors();

		auto& reverbModVoice = *REL::Relocation<RE::IXAudio2SubmixVoice**>(
			STATIC_OFFSET(BSXAudio2Audio::pReverbModVoice));

		if (reverbModVoice) {
			a_audio->XAudio->UnregisterForCallbacks(
				REL::Relocation<RE::IXAudio2EngineCallback*>(
					STATIC_OFFSET(BSXAudio2Audio::ReverbModCallback::Instance))
					.get());

			reverbModVoice->DestroyVoice();
			reverbModVoice = nullptr;
		}

		if (a_audio->masteringVoice) {
			a_audio->masteringVoice->DestroyVoice();
			a_audio->masteringVoice = nullptr;
		}

		if (a_audio->XAudio) {
			a_audio->XAudio->Release();
			a_audio->XAudio = nullptr;
		}

		if (a_audio->audioListener) {
			delete a_audio->audioListener;
			a_audio->audioListener = nullptr;
		}
	}

	void Engine::SetSilentMode(RE::BSXAudio2Audio* a_audio)
	{
		ClearSounds();
		KillAudioMonitors();
		Shutdown(a_audio);
	}

	void Engine::Reset(RE::BSAudioManager* a_audioManager)
	{
		criticalError = false;

		const auto audio = RE::BSAudioManager::QPlatformInstance();
		logger::trace("Current audio implementation: {}"sv, typeid(*audio).name());

		const auto audioImpl = RE::BSXAudio2GameSound::GetAudioImplementation();

		if (audioImpl) {
			SetSilentMode(audioImpl);
			a_audioManager->flags.reset(RE::BSAudioManager::Flags::PlatformInitialized);
			a_audioManager->flags.reset(RE::BSAudioManager::Flags::PlatformInitFailed);

			if (audioImpl->Init(&a_audioManager->initSettings.wnd)) {
				a_audioManager->flags.set(RE::BSAudioManager::Flags::PlatformInitialized);
				InitAudioMonitors();
			}
			else {
				a_audioManager->flags.set(RE::BSAudioManager::Flags::PlatformInitFailed);
			}
		}
	}

	bool Engine::Update()
	{
		switch (SKSE::WinAPI::WaitForSingleObjectEx(
			EngineCallback::Instance.criticalError,
			0,
			false)) {

		default:
		case WAIT::TIMEOUT:
			break;

		case WAIT::OBJECT_0:
			criticalError = true;
			if (const auto audio = RE::BSXAudio2GameSound::GetAudioImplementation()) {
				SetSilentMode(audio);
			}
			return false;

		case WAIT::FAILED:
			logger::error("WaitForSingleObjectEx failed"sv);
			break;
		}

		return true;
	}
}
