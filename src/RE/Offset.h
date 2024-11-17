#pragma once

namespace RE
{
	namespace Offset
	{
		namespace BGSAudioMonitors
		{
			constexpr auto bInitialized = REL::ID(402630);
			constexpr auto bigAmp = REL::ID(402633);
			constexpr auto smallAmp = REL::ID(402632);
			constexpr auto uiBigRumbleMonitorID = REL::ID(380155);
			constexpr auto uiSmallRumbleMonitorID = REL::ID(380154);
		}

		namespace BSAudioManager
		{
			namespace AudioLoadTask
			{
				constexpr auto uiTaskCount = REL::ID(410105);
			}

			constexpr auto DeferredMessages = REL::ID(410110);
			constexpr auto MessageSpinLock = REL::ID(410113);
			constexpr auto OutputOverrideLock = REL::ID(410117);
			constexpr auto PendingIOA = REL::ID(410119);
			constexpr auto StateMapRWLock = REL::ID(410112);
			constexpr auto _tManagerInstanceS = REL::ID(410108);
			constexpr auto _tPlatformInstanceS = REL::ID(410109);
			constexpr auto pFreeSoundInfos = REL::ID(410107);
			constexpr auto uiNextID = REL::ID(410106);
		}

		namespace BSAudioMonitor
		{
			namespace Receiver
			{
				constexpr auto NoMonitor = REL::ID(388361);
			}
		}

		namespace BSAudioManagerThread
		{
			constexpr auto ThreadProc = REL::ID(67746);
		}

		namespace BSXAudio2Audio
		{
			namespace ReverbModCallback
			{
				constexpr auto Instance = REL::ID(388389);
			}

			constexpr auto ActiveMonitorsA = REL::ID(388390);
			constexpr auto InactiveMonitorsA = REL::ID(388393);
			constexpr auto MonitorLock = REL::ID(410144);
			constexpr auto XAudio2Name = REL::ID(410143);
			constexpr auto hXAudio2Dll = REL::ID(410138);
			constexpr auto pReverbModVoice = REL::ID(410140);

			constexpr auto Init = REL::ID(67952);
			constexpr auto Vtbl = REL::ID(236527);
		}

		namespace Journal_SystemTab
		{
			constexpr auto ProcessCallbacks = REL::ID(53306);
		}

		namespace Main
		{
			constexpr auto StartUp = REL::ID(36547);
		}

		constexpr auto WinMain = REL::ID(36544);
	}
}
