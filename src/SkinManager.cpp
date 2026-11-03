#include "SkinManager.h"
#include "FileScanner.h"
#include "Game.h"
#include "Log.h"

SkinManager& SkinManager::Get() { static SkinManager i; return i; }

void SkinManager::Init(const std::string& configPath, const std::string& defaultModloader) {
    m_cfgPath = configPath;
    m_defaultRoot = defaultModloader;
    m_cfg.Load(configPath);
    Rescan();
}

std::string SkinManager::ModloaderDir() const {
    return m_cfg.modloaderPath.empty() ? m_defaultRoot : m_cfg.modloaderPath;
}

void SkinManager::SaveConfig() {
    std::lock_guard<std::mutex> lk(m_mx);
    if (!m_cfg.Save(m_cfgPath)) Log::Error("Could not save config: %s", m_cfgPath.c_str());
}

void SkinManager::Rescan() {
    Log::Info("Scanning skins...");
    std::string root;
    { std::lock_guard<std::mutex> lk(m_mx); root = ModloaderDir(); }
    auto found = FileScanner::ScanSkins(root);  // IO fuera del lock
    std::lock_guard<std::mutex> lk(m_mx);
    std::string activeName = (m_active >= 0 && m_active < (int)m_skins.size()) ? m_skins[m_active].name : "";
    std::string selName = (m_selected >= 0 && m_selected < (int)m_skins.size()) ? m_skins[m_selected].name : m_cfg.lastSkin;
    m_skins = std::move(found);
    m_active = m_selected = -1;
    for (int i = 0; i < (int)m_skins.size(); ++i) {
        auto& s = m_skins[i];
        Log::Info("Found skin: %s", s.name.c_str());
        Log::Info("DFF: %s", s.dffPath.c_str());
        if (!s.txdPath.empty()) Log::Info("TXD: %s", s.txdPath.c_str());
        if (s.name == activeName) { m_active = i; s.loaded = m_hasCurrent; }
        if (s.name == selName) m_selected = i;
    }
    m_status = std::to_string(m_skins.size()) + " skin(s)";
}

SkinManager::View SkinManager::Snapshot() {
    std::lock_guard<std::mutex> lk(m_mx);
    View v;
    for (auto& s : m_skins) v.names.push_back(s.name);
    v.selected = m_selected;
    v.active = m_active;
    v.status = m_status;
    v.modloaderPath = ModloaderDir();
    v.gameReady = Game::IsInitialized() && Game::Api().CanApplySkin() && Game::Rw().CanLoadModel();
    return v;
}

void SkinManager::Select(int index) {
    std::lock_guard<std::mutex> lk(m_mx);
    if (index >= 0 && index < (int)m_skins.size()) m_selected = index;
}
void SkinManager::RequestApply() { std::lock_guard<std::mutex> lk(m_mx); if (m_selected >= 0) m_pendingApply = true; }
void SkinManager::RequestRemove() { std::lock_guard<std::mutex> lk(m_mx); m_pendingRemove = true; }

void SkinManager::OnGameTick() {
    if (!Game::IsInitialized()) return;
    bool apply = false, remove = false;
    SkinInfo info;
    int idx = -1;
    {
        std::lock_guard<std::mutex> lk(m_mx);
        // Aplicar al iniciar: esperar ~3 s de frames con jugador valido.
        if (!m_startupDone) {
            if (Game::IsGameReady()) {
                if (++m_readyFrames > 180) {
                    m_startupDone = true;
                    if (m_cfg.rememberSkin && m_cfg.applyOnStart && !m_cfg.lastSkin.empty()) {
                        for (int i = 0; i < (int)m_skins.size(); ++i)
                            if (m_skins[i].name == m_cfg.lastSkin) { m_selected = i; m_pendingApply = true; }
                    }
                }
            } else m_readyFrames = 0;
        }
        apply = m_pendingApply; remove = m_pendingRemove;
        m_pendingApply = m_pendingRemove = false;
        if (apply && m_selected >= 0 && m_selected < (int)m_skins.size()) { idx = m_selected; info = m_skins[idx]; }
        else apply = false;
    }
    if (!apply && !remove) return;

    if (remove) {
        bool ok = Game::RemoveSkin();
        std::lock_guard<std::mutex> lk(m_mx);
        if (ok) {
            if (m_hasCurrent) { SkinLoader::Release(m_current); m_hasCurrent = false; }
            for (auto& s : m_skins) s.loaded = false;
            m_active = -1;
        }
        m_status = ok ? "Skin removida" : "Error al quitar la skin (ver arcx.log)";
        return;
    }

    Log::Info("Applying %s...", info.name.c_str());
    std::string status;
    if (m_hasCurrent && m_current.name == info.name) {
        // Ya residente: reutilizar recurso, solo reconstruir el ped.
        Log::Info("%s already loaded, reusing resource", info.name.c_str());
        status = Game::RefreshPlayer() ? "Skin aplicada: " + info.name : "Error al refrescar (ver arcx.log)";
    } else {
        LoadedSkin ls;
        if (!SkinLoader::Load(info, ls, m_cfg.maxResidentMB)) {
            status = "Error cargando " + info.name + " (ver arcx.log)";
        } else if (!Game::ApplySkin(ls)) {
            SkinLoader::Release(ls);
            status = "Error aplicando " + info.name + " (ver arcx.log)";
        } else {
            std::lock_guard<std::mutex> lk(m_mx);
            if (m_hasCurrent) SkinLoader::Release(m_current);  // solo TXD: el clump anterior lo destruyo el juego
            m_current = ls; m_hasCurrent = true;
            for (auto& s : m_skins) s.loaded = false;
            if (idx < (int)m_skins.size()) m_skins[idx].loaded = true;
            m_active = idx;
            if (m_cfg.rememberSkin) { m_cfg.lastSkin = info.name; m_cfg.Save(m_cfgPath); }
            status = "Skin aplicada: " + info.name;
        }
    }
    std::lock_guard<std::mutex> lk(m_mx);
    m_status = status;
}
