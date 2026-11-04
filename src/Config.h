#pragma once
#include <string>

struct ArcxConfig {
    bool rememberSkin = true;
    bool applyOnStart = true;
    bool pedFilter = true;      // ocultar DFF que no son peds (vehiculos/objetos)
    std::string lastSkin;       // nombre de carpeta de la ultima skin aplicada
    std::string modloaderPath;  // metodo "special": vacio = <datos>/files/modloader
    std::string arcxLoaderPath; // metodo "builtin": vacio = <datos>/files/ARCXLoader
    std::string loadMethod = "special";  // "special" = el juego/ModLoader carga <modelo>.dff por nombre; "builtin" = RenderWare propio (experimental)
    int skinsPerPage = 4;       // 3..10
    int maxResidentMB = 64;     // limite de seguridad para recursos de una skin

    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
};

namespace Paths {
// Devuelve el directorio de datos externos del juego (con '/' final) o "" si no existe ninguno.
std::string DetectGameDataDir();
}
