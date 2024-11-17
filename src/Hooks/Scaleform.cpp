#include "Scaleform.h"

namespace Hooks
{
	void Scaleform::Install()
	{
		LoadMovieHook::Install();
	}

	void Scaleform::LoadMovieHook::Install()
	{
		auto hook =
			REL::Relocation<std::uintptr_t>(RE::Offset::BSScaleformManager::LoadMovie, 0x1DD);
		REL::make_pattern<"FF 15">().match_or_fail(hook.address());

		_originalFunc = *reinterpret_cast<std::uintptr_t*>(
			SKSE::GetTrampoline().write_call<6>(hook.address(), &Func));
	}

	void Scaleform::LoadMovieHook::Func(
		RE::GFxMovieView* a_view,
		RE::GFxMovieView::ScaleModeType a_scaleMode)
	{
		_originalFunc(a_view, a_scaleMode);

		ScaleformHook<"_global.OptionsList.prototype", "SetEntry">::Install(a_view);
	}

	void ScaleformHook<"_global.OptionsList.prototype", "SetEntry">::Func::Call(Params& a_params)
	{
		if (a_params.argCount >= 2) {
			RE::GFxValue aEntryObject = a_params.args[1];
			if (aEntryObject.IsObject() && aEntryObject.HasMember("options")) {
				aEntryObject.SetMember("movieType", 1);
			}
		}

		_originalFunc.Invoke(
			"call",
			a_params.retVal,
			a_params.argsWithThisRef,
			static_cast<std::size_t>(a_params.argCount) + 1);
	}
}
