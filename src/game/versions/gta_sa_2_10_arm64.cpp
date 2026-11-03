// ============================================================================
//  GTA SA Android 2.10 ARM64 (libGTASA.so, arm64-v8a)  --  UNICO archivo con datos de version
// ============================================================================
//  Estado: TODOS los offsets estan en 0 (= desconocido). No se han inventado.
//  Los nombres de simbolo son HIPOTESIS a verificar con:
//      llvm-nm -D --defined-only libGTASA.so | grep -i -E 'RwStream|RpClump|CPed|ModelInfo'
//  Si libGTASA.so 2.10 esta strippeada, usa los offsets (ver docs/REVERSING.md).
//  Offset = direccion virtual de la funcion - base de carga del modulo (RVA).
// ============================================================================
#include "game/versions/gta_sa_2_10_arm64.h"
#include "Log.h"

namespace GtaSA210Arm64 {
namespace Offsets {
// --- RenderWare ---
constexpr uintptr_t RwStreamOpen = 0;               // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwStreamClose = 0;              // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwStreamFindChunk = 0;          // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RpClumpStreamRead = 0;          // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RpClumpDestroy = 0;             // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionaryStreamRead = 0;  // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionaryCreate = 0;      // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionaryDestroy = 0;     // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionaryAddTexture = 0;  // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionaryGetCurrent = 0;  // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTexDictionarySetCurrent = 0;  // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwImageCreate = 0;              // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwImageAllocatePixels = 0;      // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwImageDestroy = 0;             // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwRasterCreate = 0;             // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwRasterSetFromImage = 0;       // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTextureCreate = 0;            // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t RwTextureSetName = 0;           // TODO: GTA SA 2.10 ARM64
// --- Juego ---
constexpr uintptr_t FindPlayerPed = 0;              // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t ModelInfoPtrs = 0;              // TODO: GTA SA 2.10 ARM64 (CModelInfo::ms_modelInfoPtrs, array de punteros)
constexpr uintptr_t ModelInfo_GetRwObject = 0;      // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t ModelInfo_DeleteRwObject = 0;   // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t PedModelInfo_SetClump = 0;      // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t Ped_DeleteRwObject = 0;         // TODO: GTA SA 2.10 ARM64
constexpr uintptr_t Ped_SetModelIndex = 0;          // TODO: GTA SA 2.10 ARM64
// --- Hooks ---
constexpr uintptr_t GameProcess = 0;                // TODO: GTA SA 2.10 ARM64 (tick por frame, hilo del juego)
constexpr uintptr_t TouchEvent = 0;                 // TODO: GTA SA 2.10 ARM64 (entrada tactil)
}  // namespace Offsets

namespace {
void* Lookup(const ModuleInfo& mod, const SymbolFn& sym, const char* label,
             std::initializer_list<const char*> names, uintptr_t offset) {
    if (sym) {
        for (const char* n : names) {
            void* p = sym(n);
            if (p && mod.Contains(p)) { Log::Info("Resolved %s via symbol %s", label, n); return p; }
        }
    }
    if (offset) {
        void* p = reinterpret_cast<void*>(mod.base + offset);
        if (mod.Contains(p)) { Log::Info("Resolved %s via offset 0x%zx", label, (size_t)offset); return p; }
        Log::Error("Offset for %s outside module range", label);
        return nullptr;
    }
    Log::Error("Game function unavailable: %s (TODO: GTA SA 2.10 ARM64)", label);
    return nullptr;
}
}  // namespace

#define BIND(dst, label, off, ...) \
    dst = reinterpret_cast<decltype(dst)>(Lookup(mod, sym, label, {__VA_ARGS__}, off))

void Resolve(const ModuleInfo& mod, const SymbolFn& sym, RwApi& rw, GameApi& g, HookTargets& h) {
    // RenderWare es C: los nombres coinciden con el SDK si el binario los exporta.
    BIND(rw.RwStreamOpen, "RwStreamOpen", Offsets::RwStreamOpen, "RwStreamOpen");
    BIND(rw.RwStreamClose, "RwStreamClose", Offsets::RwStreamClose, "RwStreamClose");
    BIND(rw.RwStreamFindChunk, "RwStreamFindChunk", Offsets::RwStreamFindChunk, "RwStreamFindChunk");
    BIND(rw.RpClumpStreamRead, "RpClumpStreamRead", Offsets::RpClumpStreamRead, "RpClumpStreamRead");
    BIND(rw.RpClumpDestroy, "RpClumpDestroy", Offsets::RpClumpDestroy, "RpClumpDestroy");
    BIND(rw.RwTexDictionaryStreamRead, "RwTexDictionaryStreamRead", Offsets::RwTexDictionaryStreamRead, "RwTexDictionaryStreamRead");
    BIND(rw.RwTexDictionaryCreate, "RwTexDictionaryCreate", Offsets::RwTexDictionaryCreate, "RwTexDictionaryCreate");
    BIND(rw.RwTexDictionaryDestroy, "RwTexDictionaryDestroy", Offsets::RwTexDictionaryDestroy, "RwTexDictionaryDestroy");
    BIND(rw.RwTexDictionaryAddTexture, "RwTexDictionaryAddTexture", Offsets::RwTexDictionaryAddTexture, "RwTexDictionaryAddTexture");
    BIND(rw.RwTexDictionaryGetCurrent, "RwTexDictionaryGetCurrent", Offsets::RwTexDictionaryGetCurrent, "RwTexDictionaryGetCurrent");
    BIND(rw.RwTexDictionarySetCurrent, "RwTexDictionarySetCurrent", Offsets::RwTexDictionarySetCurrent, "RwTexDictionarySetCurrent");
    BIND(rw.RwImageCreate, "RwImageCreate", Offsets::RwImageCreate, "RwImageCreate");
    BIND(rw.RwImageAllocatePixels, "RwImageAllocatePixels", Offsets::RwImageAllocatePixels, "RwImageAllocatePixels");
    BIND(rw.RwImageDestroy, "RwImageDestroy", Offsets::RwImageDestroy, "RwImageDestroy");
    BIND(rw.RwRasterCreate, "RwRasterCreate", Offsets::RwRasterCreate, "RwRasterCreate");
    BIND(rw.RwRasterSetFromImage, "RwRasterSetFromImage", Offsets::RwRasterSetFromImage, "RwRasterSetFromImage");
    BIND(rw.RwTextureCreate, "RwTextureCreate", Offsets::RwTextureCreate, "RwTextureCreate");
    BIND(rw.RwTextureSetName, "RwTextureSetName", Offsets::RwTextureSetName, "RwTextureSetName");

    // Candidatos C++ (Itanium mangling) = HIPOTESIS SIN VERIFICAR; Lookup solo acepta lo que dlsym devuelva.
    BIND(g.FindPlayerPed, "FindPlayerPed", Offsets::FindPlayerPed, "_Z13FindPlayerPedi");
    g.modelInfoPtrs = reinterpret_cast<void**>(
        Lookup(mod, sym, "CModelInfo::ms_modelInfoPtrs", {"_ZN10CModelInfo16ms_modelInfoPtrsE"}, Offsets::ModelInfoPtrs));
    BIND(g.ModelInfo_GetRwObject, "CBaseModelInfo::GetRwObject", Offsets::ModelInfo_GetRwObject, "_ZN14CBaseModelInfo11GetRwObjectEv");
    BIND(g.ModelInfo_DeleteRwObject, "CBaseModelInfo::DeleteRwObject", Offsets::ModelInfo_DeleteRwObject, "_ZN14CBaseModelInfo14DeleteRwObjectEv");
    BIND(g.ClumpModelInfo_SetClump, "CPedModelInfo::SetClump", Offsets::PedModelInfo_SetClump, "_ZN13CPedModelInfo8SetClumpEP7RpClump");
    BIND(g.Entity_DeleteRwObject, "CPed::DeleteRwObject", Offsets::Ped_DeleteRwObject, "_ZN4CPed14DeleteRwObjectEv");
    BIND(g.Ped_SetModelIndex, "CPed::SetModelIndex", Offsets::Ped_SetModelIndex, "_ZN4CPed13SetModelIndexEj");

    h.gameProcess = Lookup(mod, sym, "game per-frame tick", {"_ZN5CGame7ProcessEv"}, Offsets::GameProcess);
    h.touchEvent = Lookup(mod, sym, "touch event", {"_Z13AND_TouchEventiiiii"}, Offsets::TouchEvent);
}
}  // namespace GtaSA210Arm64
