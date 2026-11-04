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
uintptr_t (*g_origTouch)(int, int, int, int) = nullptr;
void (*g_origSwap)() = nullptr;
Game::SwapFn g_swapCb = nullptr;
bool g_swapHooked = false;

// Estado del reemplazo
bool g_skinInstalled = false;

void (*g_origSetClump)(void*, void*) = nullptr;
int g_setClumpCalls = 0;       // solo hilo del juego
bool g_setClumpHooked = false;
void Hooked_SetClump(void* self, void* clump) {
    if (clump) ++g_setClumpCalls;
    if (g_origSetClump) g_origSetClump(self, clump);
}

void Hooked_Process() {
    if (g_origProcess) g_origProcess();
    if (g_tickCb) g_tickCb();
}
// AND_TouchEvent(int,int,int,int): se reenvia sin modificar. Significado de los args: ver Menu::OnTouchRaw.
uintptr_t Hooked_Touch(int a, int b, int c, int d) {
    if (g_touchCb && g_touchCb(a, b, c, d)) return 0;  // el menu ARCX lo consumio
    return g_origTouch ? g_origTouch(a, b, c, d) : 0;
}
// NVEventEGLSwapBuffers(): hilo de render con el contexto GL del juego activo.
void Hooked_Swap() {
    if (g_swapCb) g_swapCb();
    if (g_origSwap) g_origSwap();
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
void SetSwapCallback(SwapFn cb) { g_swapCb = cb; }
bool IsSwapHooked() { return g_swapHooked; }
bool IsTickHooked() { return g_tickHooked; }

void HookFunctions(bool hookSwap) {
    if (!g_init || !g_hook) return;
    if (g_targets.gameProcess) {
        g_tickHooked = g_hook(g_targets.gameProcess, (void*)&Hooked_Process, (void**)&g_origProcess);
        if (!g_tickHooked) Log::Error("Failed to hook per-frame tick");
    } else {
        Log::Error("Game function unavailable: per-frame tick; skins se aplicaran desde el hilo de render (inseguro)");
    }
    if (g_api.clumpModelInfoSetClump) {
        g_setClumpHooked = g_hook(g_api.clumpModelInfoSetClump, (void*)&Hooked_SetClump, (void**)&g_origSetClump);
        Log::Info("SetClump verification hook: %s", g_setClumpHooked ? "ok" : "FAILED (carga no verificable)");
    }
    if (!hookSwap) {
        // el swap se engancha fuera (eglSwapBuffers)
    } else if (g_targets.swapBuffers) {
        g_swapHooked = g_hook(g_targets.swapBuffers, (void*)&Hooked_Swap, (void**)&g_origSwap);
        if (!g_swapHooked) Log::Error("Failed to hook NVEventEGLSwapBuffers");
    } else {
        Log::Error("Game function unavailable: NVEventEGLSwapBuffers");
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

// Cargador propio. Tecnica observada en libCustomCharacter64.so (simbolos que importa) y reimplementada:
//   CEntity::DeleteRwObject(ped) -> CClumpModelInfo::DeleteRwObject(modelInfo 0) ->
//   [TXD actual = el de la skin] CFileLoader::LoadClumpFile(stream_de_memoria, 0) -> CPed::SetModelIndex(ped, 0)
// LoadClumpFile hace lo mismo que el juego con player.dff: lee el clump, lo registra en el model info y gestiona el TXD.
bool ApplySkin(LoadedSkin& skin) {
    auto fail = [&](const char* m) { Log::Error("ApplySkin(%s): %s", skin.name.c_str(), m); return false; };
    if (!g_init || !g_api.CanApplySkin() || !g_rw.CanLoadModel()) return fail("Game function unavailable");
    if (skin.dff.size() < 12) return fail("DFF vacio");
    void* ped = g_api.FindPlayerPed(-1);
    void* mi = ModelInfoForPlayer();
    if (!ped || !mi) return fail("jugador o model info no disponible");
    if (PlayerInVehicle()) return fail("no se puede cambiar dentro de un vehiculo");

    g_api.Entity_DeleteRwObject(ped);
    g_api.ClumpModelInfo_DeleteRwObject(mi);

    RwTexDictionary* prev = g_rw.RwTexDictionaryGetCurrent();
    if (skin.txd) g_rw.RwTexDictionarySetCurrent(skin.txd);
    RwMemory mem{skin.dff.data(), (uint32_t)skin.dff.size()};
    RwStream* st = g_rw.RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);
    int before = g_setClumpCalls;
    if (st) {
        g_api.FileLoader_LoadClumpFile(st, kPlayerModelId);
        g_rw.RwStreamClose(st, &mem);
    }
    g_rw.RwTexDictionarySetCurrent(prev);

    bool loaded = st && (!g_setClumpHooked || g_setClumpCalls > before);
    if (!loaded) {
        Log::Error("LoadClumpFile did not register a clump (stream=%p setClumpCalls=%d->%d); restoring CJ", (void*)st, before, g_setClumpCalls);
        RestoreCJ(nullptr);
        return false;
    }
    g_api.Ped_SetModelIndex(ped, kPlayerModelId);
    g_skinInstalled = true;
    Log::Info("Built-in loader applied %s", skin.name.c_str());
    return true;
}

bool RemoveSkin() {
    std::string err;
    bool ok = RestoreCJ(&err);
    if (ok) g_skinInstalled = false;
    return ok;
}

bool PlayerPresent() { return g_init && g_api.FindPlayerPed && g_api.FindPlayerPed(-1) != nullptr; }
bool IsPaused() {
    return (g_api.timerUserPause && *g_api.timerUserPause) || (g_api.timerCodePause && *g_api.timerCodePause);
}
bool PlayerInVehicle() { return g_api.FindPlayerVehicle && g_api.FindPlayerVehicle(-1, false) != nullptr; }
bool CanUseSpecialChar() { return g_init && g_api.CanApplySpecialChar(); }

// Flujo tomado de Skins-selector (verificado por el autor en dispositivo): RequestSpecialChar -> LoadAll ->
// comprobar id del modelo (290..310) y que sea CPedModelInfo -> DeleteRwObject -> SetModelIndex.
bool ApplySpecialChar(const std::string& name, std::string* err) {
    auto fail = [&](const char* m) { if (err) *err = m; Log::Error("ApplySpecialChar(%s): %s", name.c_str(), m); return false; };
    if (!CanUseSpecialChar()) return fail("Game function unavailable (special char API)");
    if (name.empty() || name.size() > 23) return fail("nombre de modelo vacio o >23 caracteres");
    void* ped = g_api.FindPlayerPed(-1);
    if (!ped) return fail("jugador no disponible");
    if (PlayerInVehicle()) return fail("no se puede cambiar dentro de un vehiculo");

    const int slot = 9;  // igual que Skins-selector con "skin en misiones": evita chocar con los slots de misiones
    g_api.Streaming_RequestSpecialChar(slot, name.c_str(), 0);
    g_api.Streaming_LoadAllRequestedModels(false);
    if (!g_api.Streaming_HasSpecialCharLoaded(slot)) return fail("el juego no cargo el modelo (revisa el log de ZyZ ModLoader: nombre, DFF, TXD)");

    int idx = -1;
    void* mi = g_api.ModelInfo_GetByName(name.c_str(), &idx);
    if (!mi || idx < 290 || idx > 310) return fail("el juego no registro el modelo en el rango de personajes especiales (290-310)");
    int refIdx = -1;
    void* ref = g_api.ModelInfo_GetByName("male01", &refIdx);  // un CPedModelInfo conocido: comparar vtable evita crashear con modelos que no son ped
    if (ref && *(void**)mi != *(void**)ref) return fail("el modelo no es un ped (vtable distinta)");

    g_api.Entity_DeleteRwObject(ped);
    g_api.Ped_SetModelIndex(ped, (uint32_t)idx);
    if (g_api.Streaming_SetMissionDoesntRequireSpecialChar) g_api.Streaming_SetMissionDoesntRequireSpecialChar(slot);
    Log::Info("Special char applied: %s (model id %d)", name.c_str(), idx);
    return true;
}

bool RestoreCJ(std::string* err) {
    auto fail = [&](const char* m) { if (err) *err = m; Log::Error("RestoreCJ: %s", m); return false; };
    if (!g_init || !g_api.FindPlayerPed || !g_api.Entity_DeleteRwObject || !g_api.Ped_SetModelIndex) return fail("Game function unavailable");
    void* ped = g_api.FindPlayerPed(-1);
    if (!ped) return fail("jugador no disponible");
    if (PlayerInVehicle()) return fail("no se puede cambiar dentro de un vehiculo");
    g_api.Entity_DeleteRwObject(ped);
    g_api.Ped_SetModelIndex(ped, 0);
    if (g_api.Clothes_RebuildPlayer) g_api.Clothes_RebuildPlayer(ped, false);
    Log::Info("Player restored to CJ");
    return true;
}
}  // namespace Game
