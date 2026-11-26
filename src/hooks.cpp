#include "hooks.h"
#include "assets.h"
#include "games.h"

#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>
#include <dxgi.h>
#include <dinput.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace tg
{
    namespace
    {
        using PresentFn       = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
        using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
        using GetDevStateFn   = HRESULT(WINAPI*)(IDirectInputDevice8A*, DWORD, LPVOID);
        using GetDevDataFn    = HRESULT(WINAPI*)(IDirectInputDevice8A*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

        PresentFn       g_origPresent = nullptr;
        ResizeBuffersFn g_origResize  = nullptr;
        GetDevStateFn   g_origDevState = nullptr;
        GetDevDataFn    g_origDevData  = nullptr;
        WNDPROC         g_origWndProc  = nullptr;

        ID3D11Device*           g_device  = nullptr;
        ID3D11DeviceContext*    g_context = nullptr;
        ID3D11RenderTargetView* g_rtv     = nullptr;
        HWND                    g_hwnd    = nullptr;

        std::atomic<bool> g_initialized{ false };
        std::atomic<bool> g_isOpen{ false };
        std::atomic<bool> g_shuttingDown{ false };
        std::chrono::steady_clock::time_point g_lastFrame{};

        void CreateRTV(IDXGISwapChain* sc)
        {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            ID3D11Texture2D* backBuf = nullptr;
            if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuf))) && backBuf) {
                g_device->CreateRenderTargetView(backBuf, nullptr, &g_rtv);
                backBuf->Release();
            }
        }

        void ApplyNordicTheme()
        {
            ImGuiStyle& st = ImGui::GetStyle();
            st.WindowRounding = 10.0f;
            st.ChildRounding  = 8.0f;
            st.FrameRounding  = 6.0f;
            st.FrameBorderSize = 1.0f;
            st.Colors[ImGuiCol_Text]          = ImVec4(0.95f, 0.90f, 0.80f, 1.00f);
            st.Colors[ImGuiCol_Button]        = ImVec4(0.36f, 0.22f, 0.11f, 0.95f);
            st.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.54f, 0.34f, 0.16f, 1.00f);
            st.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.68f, 0.46f, 0.18f, 1.00f);
            st.Colors[ImGuiCol_Border]        = ImVec4(0.72f, 0.56f, 0.24f, 0.85f);
            st.Colors[ImGuiCol_Separator]     = ImVec4(0.72f, 0.56f, 0.24f, 0.55f);
        }

        void LoadRussianFont()
        {
            ImGuiIO& io = ImGui::GetIO();
            static const ImWchar kRanges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement (includes «»)
                0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
                0x2010, 0x203A, // Dashes, quotes, bullets
                0
            };
            static const char* kFontPaths[] = {
                "C:\\Windows\\Fonts\\georgia.ttf",
                "C:\\Windows\\Fonts\\segoeui.ttf",
                "C:\\Windows\\Fonts\\arial.ttf",
                "C:\\Windows\\Fonts\\tahoma.ttf"
            };
            for (const char* fp : kFontPaths) {
                if (std::filesystem::exists(fp)) {
                    io.Fonts->AddFontFromFileTTF(fp, 19.0f, nullptr, kRanges);
                    AssetManager::Get().Log(std::string("Loaded font: ") + fp);
                    return;
                }
            }
            io.Fonts->AddFontDefault();
        }

        LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            int toggleKey = AssetManager::Get().config.toggleKey;
            if (msg == WM_KEYDOWN && ((lParam & (1 << 30)) == 0)) {
                if (static_cast<int>(wParam) == toggleKey) {
                    bool next = !g_isOpen.load();
                    g_isOpen.store(next);
                    if (next) ClipCursor(nullptr);
                    return 0;
                }
                if (wParam == VK_ESCAPE && g_isOpen.load()) {
                    g_isOpen.store(false);
                    return 0;
                }
            }

            if (g_isOpen.load()) {
                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
                switch (msg) {
                    case WM_MOUSEMOVE:
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_LBUTTONDBLCLK:
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                    case WM_MOUSEWHEEL:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_CHAR:
                    case WM_INPUT:
                        return 0;
                    case WM_SETCURSOR:
                        SetCursor(LoadCursor(nullptr, IDC_ARROW));
                        return TRUE;
                    default:
                        break;
                }
            }

            return CallWindowProcW(g_origWndProc, hWnd, msg, wParam, lParam);
        }

        HRESULT WINAPI HookedGetDeviceState(IDirectInputDevice8A* self, DWORD cbData, LPVOID lpvData)
        {
            HRESULT hr = g_origDevState(self, cbData, lpvData);
            if (g_isOpen.load() && SUCCEEDED(hr) && lpvData && cbData > 0) {
                std::memset(lpvData, 0, cbData);
            }
            return hr;
        }

        HRESULT WINAPI HookedGetDeviceData(IDirectInputDevice8A* self, DWORD cbObj, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
        {
            HRESULT hr = g_origDevData(self, cbObj, rgdod, pdwInOut, dwFlags);
            if (g_isOpen.load() && SUCCEEDED(hr) && pdwInOut) {
                *pdwInOut = 0;
            }
            return hr;
        }

        void TryHookDirectInput()
        {
            IDirectInput8A* di = nullptr;
            if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                                          IID_IDirectInput8A, reinterpret_cast<void**>(&di), nullptr)) || !di) {
                return;
            }
            IDirectInputDevice8A* dev = nullptr;
            if (SUCCEEDED(di->CreateDevice(GUID_SysMouse, &dev, nullptr)) && dev) {
                void** vtbl = *reinterpret_cast<void***>(dev);
                MH_CreateHook(vtbl[9],  reinterpret_cast<void*>(&HookedGetDeviceState), reinterpret_cast<void**>(&g_origDevState));
                MH_CreateHook(vtbl[10], reinterpret_cast<void*>(&HookedGetDeviceData),  reinterpret_cast<void**>(&g_origDevData));
                MH_EnableHook(vtbl[9]);
                MH_EnableHook(vtbl[10]);
                dev->Release();
                AssetManager::Get().Log("Hooked DirectInput8 input blocker.");
            }
            di->Release();
        }

        HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
        {
            if (g_shuttingDown.load()) {
                return g_origPresent(pSwapChain, SyncInterval, Flags);
            }

            if (!g_initialized.load()) {
                if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device))) && g_device) {
                    g_device->GetImmediateContext(&g_context);
                    DXGI_SWAP_CHAIN_DESC sd{};
                    pSwapChain->GetDesc(&sd);
                    g_hwnd = sd.OutputWindow;

                    CreateRTV(pSwapChain);

                    ImGui::CreateContext();
                    ImGuiIO& io = ImGui::GetIO();
                    io.IniFilename = nullptr;
                    LoadRussianFont();
                    ApplyNordicTheme();

                    ImGui_ImplWin32_Init(g_hwnd);
                    ImGui_ImplDX11_Init(g_device, g_context);

                    AssetManager::Get().LoadConfigAndProfile();
                    AssetManager::Get().LoadAllTextures(g_device);

                    g_origWndProc = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));

                    TryHookDirectInput();

                    g_lastFrame = std::chrono::steady_clock::now();
                    g_initialized.store(true);
                    AssetManager::Get().Log("DX11 + ImGui initialized successfully! Press F8 in-game.");
                }
            }

            if (g_initialized.load() && g_isOpen.load()) {
                if (!g_rtv) CreateRTV(pSwapChain);
                if (g_rtv) {
                    auto now = std::chrono::steady_clock::now();
                    float dt = std::chrono::duration<float>(now - g_lastFrame).count();
                    g_lastFrame = now;
                    if (dt <= 0.0f || dt > 0.1f) dt = 1.0f / 60.0f;

                    ImGuiIO& io = ImGui::GetIO();
                    io.MouseDrawCursor = true;

                    ImGui_ImplDX11_NewFrame();
                    ImGui_ImplWin32_NewFrame();
                    ImGui::NewFrame();

                    auto& ui = TavernGamesUI::Get();
                    ui.wantsClose = false;
                    ui.Render(dt);
                    if (ui.wantsClose) g_isOpen.store(false);

                    ImGui::Render();
                    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                }
            } else if (g_initialized.load()) {
                ImGui::GetIO().MouseDrawCursor = false;
                g_lastFrame = std::chrono::steady_clock::now();
            }

            return g_origPresent(pSwapChain, SyncInterval, Flags);
        }

        HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height,
                                           DXGI_FORMAT NewFormat, UINT SwapChainFlags)
        {
            if (g_rtv) {
                g_rtv->Release();
                g_rtv = nullptr;
            }
            return g_origResize(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        }

        void InitWorker()
        {
            for (int i = 0; i < 120 && !g_shuttingDown.load(); ++i) {
                if (GetModuleHandleW(L"d3d11.dll") && GetModuleHandleW(L"dxgi.dll")) break;
                Sleep(100);
            }
            Sleep(800);
            if (g_shuttingDown.load()) return;

            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_CLASSDC;
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"TG_DummyDX11Wnd";
            RegisterClassExW(&wc);

            HWND dummyWnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                                            0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

            DXGI_SWAP_CHAIN_DESC sd{};
            sd.BufferCount = 1;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = dummyWnd;
            sd.SampleDesc.Count = 1;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            IDXGISwapChain* dummySc = nullptr;
            ID3D11Device* dummyDev = nullptr;
            ID3D11DeviceContext* dummyCtx = nullptr;
            D3D_FEATURE_LEVEL fl{};

            HRESULT hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                nullptr, 0, D3D11_SDK_VERSION, &sd,
                &dummySc, &dummyDev, &fl, &dummyCtx);
            if (FAILED(hr)) {
                hr = D3D11CreateDeviceAndSwapChain(
                    nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                    nullptr, 0, D3D11_SDK_VERSION, &sd,
                    &dummySc, &dummyDev, &fl, &dummyCtx);
            }

            if (SUCCEEDED(hr) && dummySc) {
                void** vtbl = *reinterpret_cast<void***>(dummySc);
                void* presentAddr = vtbl[8];
                void* resizeAddr  = vtbl[13];

                dummySc->Release();
                if (dummyCtx) dummyCtx->Release();
                if (dummyDev) dummyDev->Release();

                if (MH_Initialize() == MH_OK) {
                    MH_CreateHook(presentAddr, reinterpret_cast<void*>(&HookedPresent), reinterpret_cast<void**>(&g_origPresent));
                    MH_CreateHook(resizeAddr,  reinterpret_cast<void*>(&HookedResizeBuffers), reinterpret_cast<void**>(&g_origResize));
                    MH_EnableHook(MH_ALL_HOOKS);
                    AssetManager::Get().Log("Installed DX11 Present & ResizeBuffers hooks.");
                }
            } else {
                AssetManager::Get().Log("Failed to create dummy DX11 swapchain.");
            }

            if (dummyWnd) DestroyWindow(dummyWnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
        }
    }

    void StartHooksThread(HMODULE hModule)
    {
        AssetManager::Get().InitBasePaths(hModule);
        AssetManager::Get().Log("Plugin loaded. Starting DX11 hook thread...");
        std::thread(InitWorker).detach();
    }

    void ShutdownHooks()
    {
        g_shuttingDown.store(true);
        g_isOpen.store(false);
        if (g_hwnd && g_origWndProc) {
            SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
        }
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        AssetManager::Get().ReleaseAllTextures();
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    }
}
