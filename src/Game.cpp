#include "Game.h"
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include "Log.h"
#include "SkinLoader.h"

namespace {
constexpr uint32_t kPlayerModelId = 0;  // CJ ("player"); ver docs/REVERSING.md para otras estrategias

RwApi g_rw;
GameApi g_api;
HookTargets g_targets;
ModuleInfo g_mod;
Game::HookFn g_hook;
bool g_init = false;
bool g_tickHooked = false;
void (*g_tickCb)() = nullptr;
Game::TouchFn g_touchCb = nullptr;
void (*g_origProcess)() = nullptr;
int (*g_origTouch)(int, int, int, int, int) = nullptr;

// Estado del reemplazo
void* g_origRwObject = nullptr;  // clump original del model info (para RemoveSkin)
bool g_haveOrig = false;
bool g_skinInstalled = false;

void Hooked_Process() {
    if (g_origProcess) g_origProcess();
    if (g_tickCb) g_tickCb();
}
// Paso transparente: se reenvian 5 enteros en registros; la interpretacion es TODO (ver README).
int Hooked_Touch(int a, int b, int c, int d, int e) {
    if (g_touchCb) g_touchCb(a, b, c, d);
    return g_origTouch ? g_origTouch(a, b, c, d, e) : 0;
}

void* ModelInfoForPlayer() {
    if (!g_api.modelInfoPtrs || !g_mod.Contains(g_api.modelInfoPtrs)) return nullptr;
    void* mi = g_api.modelInfoPtrs[kPlayerModelId];
    if (!mi || ((uintptr_t)mi & 7)) return nullptr;
    return mi;
}
}  // namespace

namespace Game {

bool Initialize(const ModuleInfo& mod, const GtaSA210Arm64::SymbolFn& sym, HookFn hook) {
    g_mod = mod;
    g_hook = std::move(hook);
    if (!mod.base) { Log::Error("libGTASA.so base not found"); return false; }
    GtaSA210Arm64::Resolve(mod, sym, g_rw, g_api, g_targets);
    g_init = true;
    Log::Info("Game detected (base=0x%zx size=0x%zx)", (size_t)mod.base, mod.size);
    Log::Info("Capabilities: loadModel=%d loadImages=%d applySkin=%d", g_rw.CanLoadModel(), g_rw.CanLoadImages(), g_api.CanApplySkin());
    return true;
}
bool IsInitialized() { return g_init; }
const RwApi& Rw() { return g_rw; }
const GameApi& Api() { return g_api; }

bool IsGameReady() {
    if (!g_init || !g_api.CanApplySkin() || !g_rw.CanLoadModel()) return false;
    return g_api.FindPlayerPed(-1) != nullptr;
}

void SetTickCallback(void (*cb)()) { g_tickCb = cb; }
void SetTouchCallback(TouchFn cb) { g_touchCb = cb; }
bool IsTickHooked() { return g_tickHooked; }

void HookFunctions() {
    if (!g_init || !g_hook) return;
    if (g_targets.gameProcess) {
        g_tickHooked = g_hook(g_targets.gameProcess, (void*)&Hooked_Process, (void**)&g_origProcess);
        if (!g_tickHooked) Log::Error("Failed to hook per-frame tick");
    } else {
        Log::Error("Game function unavailable: per-frame tick; skins se aplicaran desde el hilo de render (inseguro)");
    }
    if (g_targets.touchEvent) {
        if (!g_hook(g_targets.touchEvent, (void*)&Hooked_Touch, (void**)&g_origTouch))
            Log::Error("Failed to hook touch event");
    } else {
        Log::Error("Game function unavailable: touch event; el menu no recibira entrada tactil");
    }
}

bool RefreshPlayer() {
    if (!IsGameReady()) { Log::Error("Game function unavailable (not ready)"); return false; }
    void* ped = g_api.FindPlayerPed(-1);
    if (!ped) return false;
    g_api.Entity_DeleteRwObject(ped);
    g_api.Ped_SetModelIndex(ped, kPlayerModelId);
    return true;
}

bool ApplySkin(LoadedSkin& skin) {
    if (!IsGameReady()) { Log::Error("Game function unavailable / player not ready"); return false; }
    if (!skin.clump) { Log::Error("Failed to load model (null clump)"); return false; }
    void* ped = g_api.FindPlayerPed(-1);
    void* mi = ModelInfoForPlayer();
    if (!ped || !mi) { Log::Error("Invalid pointer: ped=%p modelInfo=%p", ped, mi); return false; }

    if (!g_haveOrig) {
        g_origRwObject = g_api.ModelInfo_GetRwObject(mi);
        g_haveOrig = true;
    }
    g_api.Entity_DeleteRwObject(ped);                    // 1) quitar la instancia actual del ped
    if (g_skinInstalled) g_api.ModelInfo_DeleteRwObject(mi);  // 2) destruir el clump de la skin anterior (lo posee el juego)
    g_api.ClumpModelInfo_SetClump(mi, skin.clump);       // 3) registrar el nuevo clump en el model info
    skin.clumpOwnedByGame = true;                        //    el juego es ahora responsable de destruirlo
    g_skinInstalled = true;
    g_api.Ped_SetModelIndex(ped, kPlayerModelId);        // 4) recrear la instancia del ped desde el model info
    return true;
}

bool RemoveSkin() {
    if (!g_skinInstalled) return true;
    if (!IsGameReady() || !g_haveOrig || !g_origRwObject) { Log::Error("Cannot restore original model"); return false; }
    void* ped = g_api.FindPlayerPed(-1);
    void* mi = ModelInfoForPlayer();
    if (!ped || !mi) return false;
    g_api.Entity_DeleteRwObject(ped);
    g_api.ModelInfo_DeleteRwObject(mi);
    // LIMITACION: si el juego destruyo/recargo el clump original entre medias, este puntero queda colgante.
    g_api.ClumpModelInfo_SetClump(mi, reinterpret_cast<RpClump*>(g_origRwObject));
    g_api.Ped_SetModelIndex(ped, kPlayerModelId);
    g_skinInstalled = false;
    return true;
}
}  // namespace Game
