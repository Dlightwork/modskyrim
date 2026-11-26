#pragma once
#include <windows.h>

namespace tg
{
    void StartHooksThread(HMODULE hModule);
    void ShutdownHooks();
}
