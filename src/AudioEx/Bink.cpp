#include "Bink.h"

namespace AudioEx
{
	bool Bink::Init()
	{
		const auto bink2w64 = SKSE::WinAPI::GetModuleHandle("bink2w64");
		if (!bink2w64) {
			return false;
		}

		BinkOpenXAudio27 = reinterpret_cast<BinkOpen2_t>(
			SKSE::WinAPI::GetProcAddress(bink2w64, "BinkOpenXAudio27"));
		BinkSetSoundSystem2 = reinterpret_cast<BinkSetSoundSystem2_t>(
			SKSE::WinAPI::GetProcAddress(bink2w64, "BinkSetSoundSystem2"));

		return Initialized();
	}

	bool Bink::Initialized()
	{
		return BinkOpenXAudio27 && BinkSetSoundSystem2;
	}

	void Bink::SetSoundSystem()
	{
		const auto audioImpl = RE::BSXAudio2GameSound::GetAudioImplementation();
		if (audioImpl && audioImpl->XAudio) {
			BinkSetSoundSystem2(BinkOpenXAudio27, audioImpl->XAudio, audioImpl->masteringVoice);
		}
	}
}
