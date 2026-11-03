#pragma once
#include <string>

struct ArcxConfig {
    bool rememberSkin = true;
    bool applyOnStart = true;
    std::string lastSkin;       // nombre de carpeta de la ultima skin aplicada
    std::string modloaderPath;  // vacio = autodetectar
    int skinsPerPage = 4;       // 3..10
    int maxResidentMB = 64;     // limite de seguridad para recursos de una skin

    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
};

namespace Paths {
// Devuelve el directorio de datos externos del juego (con '/' final) o "" si no existe ninguno.
std::string DetectGameDataDir();
}
