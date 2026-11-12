// Heading NaN Guard 1.0.2 -- Skyrim SE/AE 1.6.353 build (public domain, see LICENSE).
// Replaces the vector-to-heading helper with the upstream NaN-safe calculation.
// The entire 1.6.353 helper was checked against the supplied executable; see
// docs/runtime-1.6.353.md. No game structures or Address Library IDs are used.
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <intrin.h>

namespace
{
	constexpr float kPi = 3.1415927f;
	constexpr float kTwoPi = 6.2831855f;
	constexpr float kHalfPi = 1.5707964f;
	constexpr float kThreeHalfPi = 4.712389f;

	std::atomic<std::uint64_t> g_hits{ 0 };
	std::uintptr_t g_base = 0;

	float __fastcall HeadingAngle(const float* a_vec)
	{
		const float x = a_vec[0];
		const float y = a_vec[1];
		if (!std::isfinite(x) || !std::isfinite(y)) [[unlikely]] {
			const auto n = ++g_hits;
			if (n <= 50 || n % 1000 == 0) {
				const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
				SKSE::log::warn("bad vector ({}, {}) from SkyrimSE.exe+{:X} (hit {})", x, y, caller - g_base, n);
			}
			return 0.0f;
		}
		float r;
		if (y == 0.0f) {
			if (x > 0.0f) {
				return kHalfPi;
			}
			r = kThreeHalfPi;
		} else {
			r = std::atan(x / y);
			if (y < 0.0f) {
				r += kPi;
			}
		}
		while (r < 0.0f) {
			r += kTwoPi;
		}
		while (r > kTwoPi) {
			r -= kTwoPi;
		}
		return r;
	}

	void SetupLog()
	{
		auto dir = SKSE::log::log_directory();
		if (!dir) return;
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "HeadingNaNGuard.log").string(), true);
		auto log = std::make_shared<spdlog::logger>("log", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
	}

	bool Install()
	{
		g_base = REL::Module::get().base();
		REL::Relocation<std::uintptr_t> target{ REL::Offset(0xC7A230) };
		// All 158 bytes through RET, including relative operands, from Skyrim 1.6.353.0.
		// Reject an unexpected executable or an existing patch anywhere in this helper.
		constexpr std::uint8_t expected[] = {
			0x48, 0x83, 0xEC, 0x58, 0xF3, 0x0F, 0x10, 0x01, 0x0F, 0x29, 0x74, 0x24, 0x40, 0xF3, 0x0F, 0x10,
			0x35, 0xFB, 0xE2, 0x9A, 0x00, 0x0F, 0x29, 0x7C, 0x24, 0x30, 0x0F, 0x57, 0xFF, 0x44, 0x0F, 0x29,
			0x44, 0x24, 0x20, 0xF3, 0x44, 0x0F, 0x10, 0x41, 0x04, 0x44, 0x0F, 0x2E, 0xC7, 0x74, 0x1A, 0xF3,
			0x41, 0x0F, 0x5E, 0xC0, 0xE8, 0x2B, 0xE4, 0x7F, 0x00, 0x44, 0x0F, 0x2F, 0xC7, 0x73, 0x21, 0xF3,
			0x0F, 0x58, 0x05, 0xC5, 0xE2, 0x9A, 0x00, 0xEB, 0x17, 0x0F, 0x2F, 0xC7, 0x76, 0x0A, 0xF3, 0x0F,
			0x10, 0x05, 0x9A, 0xB4, 0x9D, 0x00, 0xEB, 0x16, 0xF3, 0x0F, 0x10, 0x05, 0x70, 0x9B, 0x9E, 0x00,
			0x0F, 0x2F, 0xC7, 0x73, 0x09, 0xF3, 0x0F, 0x58, 0xC6, 0x0F, 0x2F, 0xC7, 0x72, 0xF7, 0x0F, 0x2F,
			0xC6, 0x76, 0x16, 0xF3, 0x0F, 0x10, 0x0D, 0xA1, 0x4E, 0x9D, 0x00, 0x0F, 0x1F, 0x44, 0x00, 0x00,
			0xF3, 0x0F, 0x58, 0xC1, 0x0F, 0x2F, 0xC6, 0x77, 0xF7, 0x0F, 0x28, 0x74, 0x24, 0x40, 0x0F, 0x28,
			0x7C, 0x24, 0x30, 0x44, 0x0F, 0x28, 0x44, 0x24, 0x20, 0x48, 0x83, 0xC4, 0x58, 0xC3,
		};
		if (std::memcmp(reinterpret_cast<const void*>(target.address()), expected, sizeof(expected)) != 0) {
			SKSE::log::error("heading helper at SkyrimSE.exe+{:X} doesn't match the expected code: NOT patched", target.address() - g_base);
			return false;
		}
		SKSE::AllocTrampoline(14);
		SKSE::GetTrampoline().write_branch<5>(target.address(), &HeadingAngle);
		SKSE::log::info("heading helper at SkyrimSE.exe+{:X} replaced (NaN-safe)", target.address() - g_base);
		return true;
	}
}

extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version(1, 0, 2, 0));
	v.PluginName("HeadingNaNGuard");
	v.AuthorName("WhiteLionNeren & Claude");
	v.versionIndependenceEx = SKSE::PluginVersionData::kVersionIndependentEx_NoStructUse;
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_1_6_353 });
	return v;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "HeadingNaNGuard";
	a_info->version = REL::Version(1, 0, 2, 0).pack();
	return !a_skse->IsEditor() && a_skse->RuntimeVersion() == SKSE::RUNTIME_SSE_1_6_353;
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	if (a_skse->IsEditor() || a_skse->RuntimeVersion() != SKSE::RUNTIME_SSE_1_6_353) {
		return false;
	}
	SKSE::Init(a_skse, false);
	SetupLog();
	SKSE::log::info("Heading NaN Guard 1.0.2 for Skyrim 1.6.353.0");
	return Install();
}
