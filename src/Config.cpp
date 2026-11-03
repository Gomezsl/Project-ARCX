#include "Config.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static bool ToBool(const std::string& v) { return v == "1" || v == "true" || v == "on"; }

bool Config::Load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = Trim(line.substr(0, eq)), v = Trim(line.substr(eq + 1));
        if (k == "remember_skin") rememberSkin = ToBool(v);
        else if (k == "apply_on_start") applyOnStart = ToBool(v);
        else if (k == "last_skin") lastSkin = v;
        else if (k == "modloader_path") modloaderPath = v;
        else if (k == "skins_per_page") skinsPerPage = std::clamp(atoi(v.c_str()), 3, 10);
        else if (k == "max_resident_mb") maxResidentMB = std::clamp(atoi(v.c_str()), 8, 512);
    }
    return true;
}

bool Config::Save(const std::string& path) const {
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) return false;
        out << "# Project ARCX\n"
            << "remember_skin=" << (rememberSkin ? 1 : 0) << "\n"
            << "apply_on_start=" << (applyOnStart ? 1 : 0) << "\n"
            << "last_skin=" << lastSkin << "\n"
            << "modloader_path=" << modloaderPath << "\n"
            << "skins_per_page=" << skinsPerPage << "\n"
            << "max_resident_mb=" << maxResidentMB << "\n";
        if (!out.good()) return false;
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    return !ec;
}

namespace Paths {
std::string DetectGameDataDir() {
    // Candidatos; el primero que exista gana. Se puede forzar con modloader_path en el .ini.
    static const char* kCandidates[] = {
        "/storage/emulated/0/Android/data/com.rockstargames.gtasa/",
        "/sdcard/Android/data/com.rockstargames.gtasa/",
        "/storage/emulated/0/Android_unprotected/data/com.rockstargames.gtasa/",
        "/sdcard/Android_unprotected/data/com.rockstargames.gtasa/",
    };
    for (const char* c : kCandidates) {
        std::error_code ec;
        if (fs::is_directory(c, ec)) return c;
    }
    return "";
}
}  // namespace Paths
