#pragma once
#include <mutex>
#include <string>
#include <vector>
#include "Config.h"
#include "SkinInfo.h"
#include "SkinLoader.h"

class SkinManager {
public:
    struct View {
        std::vector<std::string> names;
        int selected = -1;
        int active = -1;
        std::string status;
        bool gameReady = false;
        std::string modloaderPath;
    };

    static SkinManager& Get();
    void Init(const std::string& configPath, const std::string& defaultModloader);
    void Rescan();
    View Snapshot();
    void Select(int index);
    void RequestApply();
    void RequestRemove();
    void OnGameTick();  // hilo del juego (o render como ultimo recurso)
    Config& Cfg() { return m_cfg; }
    void SaveConfig();

private:
    std::string ModloaderDir() const;

    std::mutex m_mx;
    Config m_cfg;
    std::string m_cfgPath, m_defaultRoot;
    std::vector<SkinInfo> m_skins;
    int m_selected = -1, m_active = -1;
    bool m_pendingApply = false, m_pendingRemove = false, m_startupDone = false;
    int m_readyFrames = 0;
    LoadedSkin m_current;
    bool m_hasCurrent = false;
    std::string m_status;
};
