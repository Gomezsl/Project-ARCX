#pragma once
#include <cstddef>
#include <string>
#include "SkinInfo.h"
#include "game/GameApi.h"

struct LoadedSkin {
    std::string name;
    RpClump* clump = nullptr;
    RwTexDictionary* txd = nullptr;
    std::string txdKey;            // clave de cache si el TXD es compartido
    bool clumpOwnedByGame = false; // true tras Game::ApplySkin: NO destruir desde aqui
};

namespace SkinLoader {
// Lee, valida y carga DFF + TXD/imagenes con las funciones RenderWare del juego.
bool Load(const SkinInfo& info, LoadedSkin& out, int maxResidentMB);
// Libera lo que sea nuestro (TXD con refcount; clump solo si el juego no lo posee).
void Release(LoadedSkin& skin);
size_t AvailableMemoryMB();  // MemAvailable de /proc/meminfo (0 si no se puede leer)
}
