#include "EngineCallback.h"

#include "Engine.h"

struct CREATE_EVENT
{
	enum Flags
	{
		MANUAL_RESET = 0x00000001,
		INITIAL_SET = 0x00000002,
	};
};

enum AccessFlags
{
	DELETE = 0x00010000L,
	READ_CONTROL = 0x00020000L,
	SYNCHRONIZE = 0x00100000L,
	WRITE_DAC = 0x00040000L,
	WRITE_OWNER = 0x00080000L,

	EVENT_ALL_ACCESS = 0x1F0003,
	EVENT_MODIFY_STATE = 0x0002,
};

namespace AudioEx
{
	EngineCallback::EngineCallback()
		: criticalError(SKSE::WinAPI::CreateEventEx(
			  nullptr,
			  (char*)nullptr,
			  0,
			  EVENT_MODIFY_STATE | SYNCHRONIZE))
	{
	}

	void EngineCallback::OnCriticalError([[maybe_unused]] std::int32_t a_error)
	{
		logger::info(
			"XAudio2 encountered critical error ({:08X})",
			static_cast<std::uint32_t>(a_error));

		SKSE::WinAPI::SetEvent(criticalError);
	}

	void EngineCallback::OnNewAudioDevice()
	{
		Engine::retryAudio = true;
	}
}
