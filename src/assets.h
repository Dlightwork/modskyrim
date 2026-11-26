#pragma once
#include <d3d11.h>
#include <string>
#include <unordered_map>

namespace tg
{
    struct Texture
    {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;
    };

    struct PlayerProfile
    {
        int septims = 500;
        int wins = 0;
        int losses = 0;
        int draws = 0;
        int netProfit = 0;
        int bestDartRound = 0;
        int totalGames = 0;
    };

    struct Config
    {
        int toggleKey = 0x77; // VK_F8
        bool soundEnabled = true;
    };

    class AssetManager
    {
    public:
        static AssetManager& Get();

        void InitBasePaths(HMODULE hModule);
        bool LoadAllTextures(ID3D11Device* device);
        void ReleaseAllTextures();

        const Texture* GetTexture(const std::string& name) const;
        void PlaySfx(const std::string& name) const;

        void LoadConfigAndProfile();
        void SaveProfile() const;
        void SaveConfig() const;

        void Log(const std::string& msg) const;

        PlayerProfile profile;
        Config config;

        const std::wstring& GetPluginDir() const { return pluginDir_; }

    private:
        bool LoadPng(ID3D11Device* device, const std::string& key, const std::wstring& fullPath);

        std::wstring pluginDir_;
        std::wstring assetsDir_;
        std::wstring logPath_;
        std::unordered_map<std::string, Texture> textures_;
    };
}
