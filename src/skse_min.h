#pragma once
#include <cstdint>

constexpr std::uint32_t MAKE_EXE_VERSION(std::uint32_t major, std::uint32_t minor, std::uint32_t build, std::uint32_t sub = 0)
{
    return ((major & 0xFFu) << 24) | ((minor & 0xFFu) << 16) | ((build & 0xFFFu) << 4) | (sub & 0xFu);
}

constexpr std::uint32_t RUNTIME_VERSION_1_6_1170 = MAKE_EXE_VERSION(1, 6, 1170, 0); // 0x01064920
constexpr std::uint32_t RUNTIME_VERSION_1_6_1130 = MAKE_EXE_VERSION(1, 6, 1130, 0);
constexpr std::uint32_t RUNTIME_VERSION_1_6_640  = MAKE_EXE_VERSION(1, 6, 640, 0);

struct SKSEPluginVersionData
{
    enum { kVersion = 1 };

    enum
    {
        kVersionIndependent_AddressLibraryPostAE = 1u << 0,
        kVersionIndependent_Signatures           = 1u << 1,
        kVersionIndependent_StructsPost629       = 1u << 2,
    };

    enum
    {
        kVersionIndependentEx_NoStructUse = 1u << 0,
    };

    std::uint32_t dataVersion;
    std::uint32_t pluginVersion;
    char          name[256];
    char          author[256];
    char          supportEmail[252];
    std::uint32_t versionIndependenceEx;
    std::uint32_t versionIndependence;
    std::uint32_t compatibleVersions[16];
    std::uint32_t seVersionRequired;
};

static_assert(sizeof(SKSEPluginVersionData) == 848, "SKSEPluginVersionData ABI size mismatch");

struct SKSEInterface
{
    std::uint32_t skseVersion;
    std::uint32_t runtimeVersion;
    std::uint32_t editorVersion;
    std::uint32_t isEditor;
    void*         (*QueryInterface)(std::uint32_t id);
    std::uint32_t (*GetPluginHandle)();
    std::uint32_t (*GetReleaseIndex)();
    const void*   (*GetPluginInfo)(const char* name);
};
