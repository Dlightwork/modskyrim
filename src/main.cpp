#include "skse_min.h"
#include "hooks.h"
#include <windows.h>

static HMODULE g_selfModule = nullptr;

extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() constexpr {
    SKSEPluginVersionData v{};
    v.dataVersion = SKSEPluginVersionData::kVersion;
    v.pluginVersion = 0x01000000;
    const char* kName = "TavernGamesVisual";
    for (int i = 0; kName[i] != '\0'; ++i) v.name[i] = kName[i];
    const char* kAuthor = "TavernGamesVisual";
    for (int i = 0; kAuthor[i] != '\0'; ++i) v.author[i] = kAuthor[i];
    v.versionIndependenceEx = SKSEPluginVersionData::kVersionIndependentEx_NoStructUse;
    v.versionIndependence = SKSEPluginVersionData::kVersionIndependent_Signatures |
                            SKSEPluginVersionData::kVersionIndependent_StructsPost629;
    v.compatibleVersions[0] = RUNTIME_VERSION_1_6_1170;
    v.compatibleVersions[1] = RUNTIME_VERSION_1_6_1130;
    v.compatibleVersions[2] = RUNTIME_VERSION_1_6_640;
    v.compatibleVersions[3] = 0;
    v.seVersionRequired = 0;
    return v;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface* a_skse)
{
    if (!a_skse || a_skse->isEditor) {
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
