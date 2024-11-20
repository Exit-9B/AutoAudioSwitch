#pragma once

namespace AudioEx
{
	namespace Bink
	{
		using BinkOpen2_t =
			std::int32_t (*(*)(void*, void*))(void*, std::int32_t, std::int32_t, void*, void*);
		using BinkSetSoundSystem2_t = void (*)(BinkOpen2_t, void*, void*);

		inline BinkOpen2_t BinkOpenXAudio27 = nullptr;
		inline BinkSetSoundSystem2_t BinkSetSoundSystem2 = nullptr;

		bool Init();
		[[nodiscard]] bool Initialized();
		void SetSoundSystem();
	}
}
