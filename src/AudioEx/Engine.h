#pragma once

namespace AudioEx
{
	namespace Engine
	{
		inline constinit RE::XAUDIO2_DEVICE_DETAILS CurrentDevice;
		inline constinit bool criticalError = false;
		inline constinit bool retryAudio = false;

		void SetSilentMode(RE::BSXAudio2Audio* a_audio);
		void Reset(RE::BSAudioManager* a_audioManager);
		bool Update();
	}
}
