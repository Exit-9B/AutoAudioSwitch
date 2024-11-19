#pragma once

namespace Hooks
{
	class Options
	{
	public:
		static void Install();

		struct AudioOptionsHook
		{
			static void Install();
			static void Func(const RE::FxDelegateArgs& a_params);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};

		struct OptionChangeHook
		{
			static void Install();
			static void Func(const RE::FxDelegateArgs& a_params);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};
	};
}
