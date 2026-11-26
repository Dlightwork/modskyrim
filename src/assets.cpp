#include "assets.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <windows.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace tg
{
    AssetManager& AssetManager::Get()
    {
        static AssetManager inst;
        return inst;
    }

    void AssetManager::InitBasePaths(HMODULE hModule)
    {
        wchar_t dllPath[MAX_PATH]{};
        GetModuleFileNameW(hModule, dllPath, MAX_PATH);
        fs::path p(dllPath);
        pluginDir_ = p.parent_path().wstring();
        assetsDir_ = (p.parent_path() / L"TavernGamesVisual").wstring();

        PWSTR docsPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docsPath)) && docsPath) {
            fs::path skseLogDir = fs::path(docsPath) / L"My Games" / L"Skyrim Special Edition" / L"SKSE";
            std::error_code ec;
            fs::create_directories(skseLogDir, ec);
            logPath_ = (skseLogDir / L"TavernGamesVisual.log").wstring();
            CoTaskMemFree(docsPath);
        } else {
            logPath_ = (p.parent_path() / L"TavernGamesVisual.log").wstring();
        }
    }

    void AssetManager::Log(const std::string& msg) const
    {
        if (logPath_.empty()) return;
        std::ofstream ofs(fs::path(logPath_), std::ios::app);
        if (ofs) {
            ofs << "[TavernGamesVisual] " << msg << "\n";
        }
    }

    bool AssetManager::LoadPng(ID3D11Device* device, const std::string& key, const std::wstring& fullPath)
    {
        if (!device) return false;
        std::ifstream file(fs::path(fullPath), std::ios::binary | std::ios::ate);
        if (!file) {
            Log("Missing texture: " + key);
            return false;
        }
        std::streamsize sz = file.tellg();
        if (sz <= 0) return false;
        file.seekg(0, std::ios::beg);
        std::string buf(static_cast<size_t>(sz), '\0');
        if (!file.read(buf.data(), sz)) return false;

        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(buf.data()),
            static_cast<int>(sz), &w, &h, &channels, 4);
        if (!pixels) {
            Log("Failed to decode PNG: " + key);
            return false;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(w);
        desc.Height = static_cast<UINT>(h);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = pixels;
        initData.SysMemPitch = static_cast<UINT>(w * 4);

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = device->CreateTexture2D(&desc, &initData, &tex);
        stbi_image_free(pixels);
        if (FAILED(hr) || !tex) {
            Log("CreateTexture2D failed: " + key);
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
        tex->Release();
        if (FAILED(hr) || !srv) {
            Log("CreateShaderResourceView failed: " + key);
            return false;
        }

        textures_[key] = Texture{ srv, w, h };
        return true;
    }

    bool AssetManager::LoadAllTextures(ID3D11Device* device)
    {
        ReleaseAllTextures();
        static const char* names[] = {
            "wood_table", "red_velvet", "dartboard", "checkers_board",
            "dice_atlas", "card_back", "card_front", "cup_thimble",
            "coin_septim", "piece_white", "piece_black",
            "piece_white_king", "piece_black_king"
        };
        int loaded = 0;
        for (const char* name : names) {
            std::wstring wname(name, name + std::strlen(name));
            fs::path path = fs::path(assetsDir_) / L"textures" / (wname + L".png");
            if (LoadPng(device, name, path.wstring())) {
                loaded++;
            }
        }
        Log("Loaded textures: " + std::to_string(loaded) + " / 13");
        return loaded > 0;
    }

    void AssetManager::ReleaseAllTextures()
    {
        for (auto& [k, t] : textures_) {
            if (t.srv) {
                t.srv->Release();
                t.srv = nullptr;
            }
        }
        textures_.clear();
    }

    const Texture* AssetManager::GetTexture(const std::string& name) const
    {
        auto it = textures_.find(name);
        if (it == textures_.end() || !it->second.srv) return nullptr;
        return &it->second;
    }

    void AssetManager::PlaySfx(const std::string& name) const
    {
        if (!config.soundEnabled) return;
        std::wstring wname(name.begin(), name.end());
        fs::path path = fs::path(assetsDir_) / L"sounds" / (wname + L".wav");
        if (fs::exists(path)) {
            PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        }
    }

    void AssetManager::LoadConfigAndProfile()
    {
        fs::path iniPath = fs::path(pluginDir_) / L"TavernGamesVisual.ini";
        config.toggleKey = GetPrivateProfileIntW(L"General", L"ToggleKey", 0x77, iniPath.c_str());
        config.soundEnabled = GetPrivateProfileIntW(L"General", L"SoundEnabled", 1, iniPath.c_str()) != 0;

        fs::path profPath = fs::path(assetsDir_) / L"profile.ini";
        profile.septims       = GetPrivateProfileIntW(L"Profile", L"Septims", 500, profPath.c_str());
        profile.wins          = GetPrivateProfileIntW(L"Profile", L"Wins", 0, profPath.c_str());
        profile.losses        = GetPrivateProfileIntW(L"Profile", L"Losses", 0, profPath.c_str());
        profile.draws         = GetPrivateProfileIntW(L"Profile", L"Draws", 0, profPath.c_str());
        profile.netProfit     = GetPrivateProfileIntW(L"Profile", L"NetProfit", 0, profPath.c_str());
        profile.bestDartRound = GetPrivateProfileIntW(L"Profile", L"BestDartRound", 0, profPath.c_str());
        profile.totalGames    = GetPrivateProfileIntW(L"Profile", L"TotalGames", 0, profPath.c_str());
        if (profile.septims < 0) profile.septims = 0;
    }

    void AssetManager::SaveConfig() const
    {
        fs::path iniPath = fs::path(pluginDir_) / L"TavernGamesVisual.ini";
        WritePrivateProfileStringW(L"General", L"ToggleKey", std::to_wstring(config.toggleKey).c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"General", L"SoundEnabled", config.soundEnabled ? L"1" : L"0", iniPath.c_str());
    }

    void AssetManager::SaveProfile() const
    {
        std::error_code ec;
        fs::create_directories(fs::path(assetsDir_), ec);
        fs::path profPath = fs::path(assetsDir_) / L"profile.ini";
        WritePrivateProfileStringW(L"Profile", L"Septims", std::to_wstring(profile.septims).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"Wins", std::to_wstring(profile.wins).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"Losses", std::to_wstring(profile.losses).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"Draws", std::to_wstring(profile.draws).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"NetProfit", std::to_wstring(profile.netProfit).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"BestDartRound", std::to_wstring(profile.bestDartRound).c_str(), profPath.c_str());
        WritePrivateProfileStringW(L"Profile", L"TotalGames", std::to_wstring(profile.totalGames).c_str(), profPath.c_str());
    }
}
