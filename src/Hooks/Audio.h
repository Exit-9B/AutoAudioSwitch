#pragma once

namespace RE::BSGraphics
{
	class Renderer;
	struct RendererInitOSData;
	struct ApplicationWindowProperties;
	struct RendererInitReturn;
}

namespace Hooks
{
	class Audio final
	{
	public:
		static void Install();

	private:
		static void ProcessIXAudio2(RE::IXAudio2* a_xaudio);

		struct StartUpHook
		{
			static void Install();
			static void Func();

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};

		struct BinkHook
		{
			static void Install();
		};

		struct XAudioHook
		{
			static void Install();
			static std::int32_t Func(
				SKSE::WinAPI::REFCLSID a_rclsid,
				RE::IUnknown* a_unkOuter,
				std::uint32_t a_clsContext,
				SKSE::WinAPI::REFIID a_riid,
				void* a_ppv);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};

		struct MasteringVoiceHook
		{
			static void Install(std::uintptr_t a_vtbl);
			static std::int32_t Func(
				RE::IXAudio2* a_xaudio,
				RE::IXAudio2MasteringVoice** a_masteringVoice,
				std::uint32_t a_inputChannels,
				std::uint32_t a_inputSampleRate,
				std::uint32_t a_flags,
				std::uint32_t a_deviceIndex,
				const RE::XAUDIO2_EFFECT_CHAIN* a_effectChain);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};

		struct AudioInitHook
		{
			static void Install();
			static bool Func(RE::BSXAudio2Audio* a_audioImpl);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};

		struct ProcessSoundUpdatesHook
		{
			static void Install();
			static void Func(RE::BSAudioManager* a_audioManager);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};
	};
}
