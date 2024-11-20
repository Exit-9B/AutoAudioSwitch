#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

#include <ranges>

using namespace std::literals;

namespace logger = SKSE::log;
namespace nttp = SKSE::stl::nttp;

namespace util
{
	using SKSE::stl::report_and_fail;
	using SKSE::stl::to_underlying;
	using SKSE::stl::unrestricted_cast;

	enum struct Op : std::uint8_t
	{
		LEA = 0x8D,
	};

	enum struct Reg : std::uint8_t
	{
		AX = 0b0000,
		CX = 0b0001,
		DX = 0b0010,
		BX = 0b0011,
		SP = 0b0100,
		BP = 0b0101,
		SI = 0b0110,
		DI = 0b0111,
		R8 = 0b1000,
		R9 = 0b1001,
		R10 = 0b1010,
		R11 = 0b1011,
		R12 = 0b1100,
		R13 = 0b1101,
		R14 = 0b1110,
		R15 = 0b1111,
	};

	struct REX
	{
		std::uint8_t b : 1 = 0b0;
		std::uint8_t x : 1 = 0b0;
		std::uint8_t r : 1 = 0b0;
		std::uint8_t w : 1 = 0b0;
		std::uint8_t fixed : 4 = 0b0100;
	};
	static_assert(sizeof(REX) == 0x1);

	struct ModRM
	{
		std::uint8_t rm : 3 = 0b000;
		std::uint8_t reg : 3 = 0b000;
		std::uint8_t mod : 2 = 0b00;
	};
	static_assert(sizeof(ModRM) == 0x1);

	template <std::convertible_to<const void*> F>
	inline void write_14branch(std::uintptr_t a_src, F&& a_dst)
	{
#pragma pack(push, 1)
		// FF /4
		// JMP r/m64
		struct Assembly
		{
			// jmp [rip]
			std::uint8_t  jmp;    // 0 - 0xFF
			std::uint8_t  modrm;  // 1 - 0x25
			std::int32_t  disp;   // 2 - 0x00000000
			std::uint64_t addr;   // 6 - [rip]
		};
		static_assert(offsetof(Assembly, jmp) == 0x0);
		static_assert(offsetof(Assembly, modrm) == 0x1);
		static_assert(offsetof(Assembly, disp) == 0x2);
		static_assert(offsetof(Assembly, addr) == 0x6);
		static_assert(sizeof(Assembly) == 0xE);
#pragma pack(pop)

		const Assembly assembly{
			.jmp = static_cast<std::uint8_t>(0xFF),
			.modrm = static_cast<std::uint8_t>(0x25),
			.disp = static_cast<std::int32_t>(0),
			.addr = static_cast<std::uint64_t>(unrestricted_cast<std::uintptr_t>(a_dst)),
		};

		REL::safe_write(a_src, std::addressof(assembly), sizeof(assembly));
	}

	template <Reg R64, Op OP>
	inline std::uintptr_t
	write_rm(SKSE::Trampoline& a_trampoline, std::uintptr_t a_src, std::uintptr_t a_dst)
	{
#pragma pack(push, 1)
		struct SrcAssembly
		{
			// lea r64, [rip + disp]
			REX rex;            // 0 - REX.W
			Op opcode;          // 1
			ModRM modrm;        // 2 - mod = 0b00, reg = r64, rm = 0b101
			std::int32_t disp;  // 3
		};
		static_assert(offsetof(SrcAssembly, rex) == 0x0);
		static_assert(offsetof(SrcAssembly, opcode) == 0x1);
		static_assert(offsetof(SrcAssembly, modrm) == 0x2);
		static_assert(offsetof(SrcAssembly, disp) == 0x3);
		static_assert(sizeof(SrcAssembly) == 0x7);
#pragma pack(pop)

		const auto pdisp = std::addressof(reinterpret_cast<const SrcAssembly*>(a_src)->disp);
		const auto ret = a_src + sizeof(SrcAssembly) + *pdisp;

		const std::int32_t disp = a_trampoline.make_disp(a_src + sizeof(SrcAssembly), a_dst);

		const SrcAssembly assembly{
			.rex = {
				.r = (to_underlying(R64) >> 3) & 0b1,
				.w = 0b1,
			},
			.opcode = OP,
			.modrm = {
				.rm = 0b101,
				.reg = to_underlying(R64) & 0b111,
				.mod = 0b00,
			},
			.disp = disp,
		};
		REL::safe_write(a_src, &assembly, sizeof(assembly));

		return ret;
	}

	template <Reg R64, std::convertible_to<const void*> F>
	inline std::uintptr_t
	write_lea(SKSE::Trampoline& a_trampoline, std::uintptr_t a_src, F&& a_dst)
	{
		return write_rm<R64, Op::LEA>(
			a_trampoline,
			a_src,
			unrestricted_cast<std::uintptr_t>(a_dst));
	}
}

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"
