#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Diagnostico de texturas SOLO con lectura de archivos (sin tocar el motor): permite avisar en arcx.log
// que texturas pide un DFF y cuales no estan en la carpeta de la skin (TXD o imagenes).
namespace RwDiag {
// Nombres (minusculas, sin duplicados) de las texturas referenciadas por los materiales del DFF.
std::vector<std::string> DffTextureNames(const std::vector<uint8_t>& dff);
// Nombres de las texturas de un TXD (vacio si el formato nativo no se reconoce).
std::vector<std::string> TxdTextureNames(const std::string& path);
std::string Lower(std::string s);
}
