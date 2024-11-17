#pragma once

namespace AudioEx
{
	class EngineCallback final : public RE::IXAudio2EngineCallback
	{
	public:
		static EngineCallback Instance;

		EngineCallback();

		void OnProcessingPassStart() override {}

		void OnProcessingPassEnd() override {}

		void OnCriticalError(std::int32_t a_error) override;

		void OnNewAudioDevice();

		SKSE::WinAPI::ScopedHandle criticalError;
	};

	inline EngineCallback EngineCallback::Instance;
}
