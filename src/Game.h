#pragma once
#include <functional>
#include "game/GameApi.h"
#include "game/versions/gta_sa_2_10_arm64.h"

struct LoadedSkin;

namespace Game {
using HookFn = std::function<bool(void* target, void* replacement, void** original)>;

bool Initialize(const ModuleInfo& mod, const GtaSA210Arm64::SymbolFn& sym, HookFn hook);
bool IsInitialized();
bool IsGameReady();               // funciones resueltas + jugador existente
const RwApi& Rw();
const GameApi& Api();

void HookFunctions();             // instala hook de tick/toque si las direcciones estan resueltas
bool IsTickHooked();
void SetTickCallback(void (*cb)());
using TouchFn = bool (*)(int a, int b, int c, int d);  // true = evento consumido (no llega al juego)
void SetTouchCallback(TouchFn cb);
using SwapFn = void (*)();
void SetSwapCallback(SwapFn cb);  // se llama en cada swap del juego, hilo de render, contexto GL activo
bool IsSwapHooked();

bool ApplySkin(LoadedSkin& skin); // sustituye el clump del modelo del jugador. Solo hilo del juego.
bool RemoveSkin();
bool RefreshPlayer();             // reconstruye el RwObject del jugador (reuso de recurso ya cargado)
}
