#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "SkinInfo.h"
#include "game/GameApi.h"

// Recursos preparados por el cargador propio. El clump NO se crea aqui: lo carga el propio juego con
// CFileLoader::LoadClumpFile (registra el modelo, TXD y jerarquia igual que player.dff del IMG).
struct LoadedSkin {
    std::string name;
    std::vector<uint8_t> dff;       // bytes del DFF validado (se liberan tras aplicar)
    RwTexDictionary* txd = nullptr; // textos del TXD + PNG convertidos; debe vivir mientras la skin este activa
    std::string txdKey;             // clave de cache si el TXD es compartido
};

namespace SkinLoader {
bool Load(const SkinInfo& info, LoadedSkin& out, int maxResidentMB);
void Release(LoadedSkin& skin);
size_t AvailableMemoryMB();
}
