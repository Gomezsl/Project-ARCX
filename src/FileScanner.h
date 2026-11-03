#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "SkinInfo.h"

namespace FileScanner {
constexpr uint32_t kChunkClump = 0x10;         // rwID_CLUMP (DFF)
constexpr uint32_t kChunkTexDict = 0x16;       // rwID_TEXDICTIONARY (TXD)
constexpr uint64_t kMaxFileBytes = 64ull << 20;

// Valida existencia, archivo regular, tamano y cabecera RenderWare (tipo de chunk + tamano coherente).
bool IsValidRwFile(const std::string& path, uint32_t expectedChunk, std::string* err = nullptr);

// Escanea subcarpetas de `root` (no recursivo, ignora symlinks y carpetas sin DFF valido).
std::vector<SkinInfo> ScanSkins(const std::string& root);
}
