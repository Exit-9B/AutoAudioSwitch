#pragma once

namespace Hooks
{
	class Scaleform final
	{
	public:
		static void Install();

		struct LoadMovieHook
		{
			static void Install();
			static void
			Func(RE::GFxMovieView* a_view, RE::GFxMovieView::ScaleModeType a_scaleMode);

			inline static REL::Relocation<decltype(&Func)> _originalFunc;
		};
	};

	template <nttp::zstring Prototype, nttp::zstring Fn>
	class ScaleformHook final
	{
	public:
		static bool Install(RE::GFxMovieView* a_view);

	private:
		class Func : public RE::GFxFunctionHandler
		{
		public:
			Func(const RE::GFxValue& a_originalFunc) : _originalFunc{ a_originalFunc } {}

			void Call(Params& a_params) override;

		private:
			RE::GFxValue _originalFunc;
		};
	};

	template <nttp::zstring Prototype, nttp::zstring Fn>
	inline bool ScaleformHook<Prototype, Fn>::Install(RE::GFxMovieView* a_view)
	{
		assert(a_view);

		RE::GFxValue obj;
		if (!a_view->GetVariable(&obj, Prototype.data()) || !obj.IsObject()) {
			return false;
		}

		RE::GFxValue original;
		if (!obj.GetMember(Fn.data(), &original) || !original.IsObject()) {
			return false;
		}

		RE::GFxValue func;
		const auto impl = RE::make_gptr<Func>(original);

		a_view->CreateFunction(&func, impl.get());
		return obj.SetMember(Fn.data(), func);
	}
}
