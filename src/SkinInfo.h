#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct SkinInfo {
    std::string name;                    // nombre mostrado (= nombre de la carpeta)
    std::string folder;                  // ruta completa de la carpeta
    std::string modelName;               // stem del .dff: nombre que el juego usa en RequestSpecialChar (<=23 chars)
    std::string dffPath;                 // modelo validado
    std::string txdPath;                 // vacio si la skin solo usa imagenes
    std::vector<std::string> imagePaths; // png/jpg/jpeg (convertidas a RwTexture al cargar)
    std::vector<std::string> otherFiles; // otros archivos de la carpeta (diagnostico de texturas)
    uint64_t totalBytes = 0;
    bool loaded = false;                 // true solo mientras sus recursos estan residentes
};
