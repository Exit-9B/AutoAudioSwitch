#include "Engine.h"

#include "Bink.h"
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
	[[nodiscard]] static RE::IXAudio2SourceVoice* GetCompatableSourceVoice(
		RE::BSXAudio2Audio* a_audioImpl,
		const RE::WAVEFORMATEX* a_format,
		const RE::BSTSmallArray<RE::BSAudioMonitor::Request, 2>& a_sends,
		RE::IXAudio2VoiceCallback* a_callback,
		bool a_isMusic)
	{
		using func_t = decltype(&GetCompatableSourceVoice);
		REL::Relocation<func_t> func{ STATIC_OFFSET(BSXAudio2Audio::GetCompatableSourceVoice) };
		return func(a_audioImpl, a_format, a_sends, a_callback, a_isMusic);
	}

	static void KillGameSounds()
	{
		const auto audioManager = RE::BSAudioManager::GetSingleton();
		const auto audio = RE::BSAudioManager::QPlatformInstance();

		if (const auto audioImpl = skyrim_cast<RE::BSXAudio2Audio*>(audio)) {
			for (auto& [soundID, gameSound] : audioManager->activeSounds) {
				const auto xaudioGameSound = static_cast<RE::BSXAudio2GameSound*>(gameSound);

				if (xaudioGameSound->sourceVoice) {
					RE::XAUDIO2_VOICE_STATE voiceState{};
					xaudioGameSound->sourceVoice->GetState(std::addressof(voiceState));
					xaudioGameSound->samplesPlayed =
						static_cast<std::uint32_t>(voiceState.SamplesPlayed);

					xaudioGameSound->sourceVoice->FlushSourceBuffers();
					xaudioGameSound->buffersQueued = 0;
					std::exchange(xaudioGameSound->sourceVoice, nullptr)->DestroyVoice();
				}

				xaudioGameSound->lastUpdateTime = std::numeric_limits<std::uint32_t>::max();
			}
		}
		else {
			for (const auto& [soundID, gameSound] : audioManager->activeSounds) {
				audio->ReleaseGameSound(gameSound);
			}
			audioManager->activeSounds.clear();

			for (const auto& [soundID, soundInfo] : audioManager->stateMap) {
				delete soundInfo;
			}
			audioManager->stateMap.clear();
		}

		audioManager->ClearCache();
	}

	static bool
	ReviveGameSound(RE::BSXAudio2Audio* a_audioImpl, RE::BSXAudio2GameSound* a_gameSound)
	{
		const auto src = a_gameSound->src;
		if (!src) {
			return false;
		}

		const std::uint32_t srcChannels = src->format.nChannels;
		const std::uint32_t dstChannels = a_audioImpl->speakerChannels;

		a_gameSound->dspSettings.SrcChannelCount = srcChannels;
		a_gameSound->dspSettings.DstChannelCount = dstChannels;

		RE::free(std::exchange(
			a_gameSound->dspSettings.pMatrixCoefficients,
			RE::calloc<float>(static_cast<size_t>(srcChannels) * dstChannels)));

		a_gameSound->emitter.ChannelCount = a_gameSound->dspSettings.SrcChannelCount;

		a_gameSound->sourceVoice = GetCompatableSourceVoice(
			a_audioImpl,
			std::addressof(src->format),
			a_gameSound->requests,
			a_gameSound,
			a_gameSound->IsMusic());

		if (!a_gameSound->sourceVoice) {
			return false;
		}

		a_gameSound->OutputModelChangedImpl();
		a_gameSound->SetVolumeImpl();

		if ((a_gameSound->soundType.underlying() & 0x3800) == 0) {
			a_gameSound->playbackPosition += std::exchange(a_gameSound->samplesPlayed, 0);
			if (src->format.wFormatTag == 358) {
				a_gameSound->playbackPosition &= ~0x7F;
			}
		}

		if (a_gameSound->IsPlaying()) {
			a_gameSound->SeekInSamples(a_gameSound->playbackPosition);
		}

		return true;
	}

	static void KillAudioMonitors()
	{
		auto& monitorLock =
			*REL::Relocation<RE::BSSpinLock*>(STATIC_OFFSET(BSXAudio2Audio::MonitorLock));

		auto& activeMonitors = *REL::Relocation<RE::BSTSmallArray<RE::XAudio2Monitor, 8>*>(
			STATIC_OFFSET(BSXAudio2Audio::ActiveMonitorsA));
		auto& inactiveMonitors = *REL::Relocation<RE::BSTSmallArray<RE::XAudio2Monitor, 8>*>(
			STATIC_OFFSET(BSXAudio2Audio::InactiveMonitorsA));

		RE::BSSpinLockGuard locker{ monitorLock };

		for (auto& monitor : activeMonitors) {
			if (monitor.submixVoice) {
				std::exchange(monitor.submixVoice, nullptr)->DestroyVoice();
			}
		}

		for (auto& monitor : inactiveMonitors) {
			if (monitor.submixVoice) {
				std::exchange(monitor.submixVoice, nullptr)->DestroyVoice();
			}
			if (monitor.monitorAPO) {
				delete std::exchange(monitor.monitorAPO, nullptr);
			}
		}
		inactiveMonitors.clear();
	}

	static void ReviveAudioMonitors(RE::BSXAudio2Audio* a_audioImpl)
	{
		auto& monitorLock =
			*REL::Relocation<RE::BSSpinLock*>(STATIC_OFFSET(BSXAudio2Audio::MonitorLock));

		auto& activeMonitors = *REL::Relocation<RE::BSTSmallArray<RE::XAudio2Monitor, 8>*>(
			STATIC_OFFSET(BSXAudio2Audio::ActiveMonitorsA));

		RE::BSSpinLockGuard locker{ monitorLock };

		for (auto& monitor : activeMonitors) {
			RE::XAUDIO2_EFFECT_DESCRIPTOR effectDescriptor{
				.pEffect = monitor.monitorAPO,
				.InitialState = 1,
				.OutputChannels = 2,
			};

			const RE::XAUDIO2_EFFECT_CHAIN effectChain{
				.EffectCount = 1,
				.pEffectDescriptors = std::addressof(effectDescriptor),
			};

			a_audioImpl->XAudio->CreateSubmixVoice(
				std::addressof(monitor.submixVoice),
				2,
				a_audioImpl->outputFormat.Format.nSamplesPerSec,
				0,
				std::numeric_limits<std::uint32_t>::max(),
				nullptr,
				std::addressof(effectChain));
		}
	}

	static void KillEngine(RE::BSXAudio2Audio* a_audio)
	{
		if (a_audio->XAudio) {
			a_audio->XAudio->StopEngine();
		}

		auto& reverbModVoice = *REL::Relocation<RE::IXAudio2SubmixVoice**>(
			STATIC_OFFSET(BSXAudio2Audio::pReverbModVoice));

		if (reverbModVoice) {
			a_audio->XAudio->UnregisterForCallbacks(
				REL::Relocation<RE::IXAudio2EngineCallback*>(
					STATIC_OFFSET(BSXAudio2Audio::ReverbModCallback::Instance))
					.get());

			std::exchange(reverbModVoice, nullptr)->DestroyVoice();
		}

		if (a_audio->masteringVoice) {
			std::exchange(a_audio->masteringVoice, nullptr)->DestroyVoice();
		}

		if (a_audio->XAudio) {
			std::exchange(a_audio->XAudio, nullptr)->Release();
		}

		if (a_audio->audioListener) {
			delete std::exchange(a_audio->audioListener, nullptr);
		}
	}

	void Engine::SetSilentMode(RE::BSXAudio2Audio* a_audio)
	{
		KillGameSounds();
		KillAudioMonitors();
		KillEngine(a_audio);
	}

	void Engine::Reset(RE::BSAudioManager* a_audioManager)
	{
		criticalError = false;

		const auto audio = RE::BSAudioManager::QPlatformInstance();
		logger::trace("Current audio implementation: {}"sv, typeid(*audio).name());

		const auto audioImpl = RE::BSXAudio2GameSound::GetAudioImplementation();

		if (!audioImpl) {
			return;
		}

		if (audioImpl->XAudio) {
			std::uint32_t deviceCount{};
			audioImpl->XAudio->GetDeviceCount(std::addressof(deviceCount));
			if (deviceCount == 0) {
				return;
			}
		}

		SetSilentMode(audioImpl);
		a_audioManager->flags.reset(RE::BSAudioManager::Flags::PlatformInitialized);
		a_audioManager->flags.reset(RE::BSAudioManager::Flags::PlatformInitFailed);

		logger::trace("Reinitializing XAudio2..."sv);
		if (audioImpl->Init(&a_audioManager->initSettings.wnd)) {
			audioImpl->XAudio->StartEngine();
			a_audioManager->flags.set(RE::BSAudioManager::Flags::PlatformInitialized);

			Bink::SetSoundSystem();

			ReviveAudioMonitors(audioImpl);

			auto& activeSounds = a_audioManager->activeSounds;
			for (auto it = activeSounds.begin(); it != activeSounds.end();) {
				const auto& [soundID, gameSound] = *it;

				if (ReviveGameSound(audioImpl, static_cast<RE::BSXAudio2GameSound*>(gameSound))) {
					++it;
				}
				else {
					audioImpl->ReleaseGameSound(gameSound);
					it = activeSounds.erase(it);
				}
			}
		}
		else {
			a_audioManager->flags.set(RE::BSAudioManager::Flags::PlatformInitFailed);

			for (const auto& [soundID, gameSound] : a_audioManager->activeSounds) {
				delete gameSound;
			}
			a_audioManager->activeSounds.clear();

			for (const auto& [soundID, soundInfo] : a_audioManager->stateMap) {
				delete soundInfo;
			}
			a_audioManager->stateMap.clear();
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
			// DirectXTK recommended stopping the engine here, but it seems harmless to keep it
			// running, and keeping the interface allows us to query the live device count before
			// attempting a reset.
			return false;

		case WAIT::FAILED:
			logger::error("WaitForSingleObjectEx failed"sv);
			break;
		}

		return true;
	}
}
