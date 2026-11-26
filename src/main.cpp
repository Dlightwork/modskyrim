#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "hooks.h"

#include <windows.h>

static HMODULE g_selfModule = nullptr;

namespace
{
    void OpenGamesPapyrus(RE::StaticFunctionTag*)
    {
        tg::OpenGames();
    }

    bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm)
    {
        if (!vm) {
            return false;
        }

        vm->RegisterFunction("OpenGames", "TavernGamesNative", OpenGamesPapyrus);
        return true;
    }
}

extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() constexpr {
    SKSE::PluginVersionData version;
    version.PluginVersion(REL::Version{ 1, 1, 0, 0 });
    version.PluginName("TavernGamesVisual");
    version.AuthorName("TavernGamesVisual");
    version.UsesAddressLibrary();
    version.UsesAddressLibraryV5();
    version.UsesNoStructs();
    version.CompatibleVersions({ REL::Version{ 1, 6, 1170, 0 } });
    return version;
}();

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(
    const SKSE::QueryInterface* skse,
    SKSE::PluginInfo* info)
{
    if (!skse || !info || skse->IsEditor()) {
        return false;
    }

    info->infoVersion = SKSE::PluginInfo::kVersion;
    info->name = "TavernGamesVisual";
    info->version = 1;
    return true;
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse)
{
    if (!skse) {
        return false;
    }

    SKSE::Init(skse);

    const auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus || !papyrus->Register(RegisterPapyrusFunctions)) {
        return false;
    }

    tg::StartHooksThread(g_selfModule);
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_selfModule = hModule;
        DisableThreadLibraryCalls(hModule);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        tg::ShutdownHooks();
    }
    return TRUE;
}
