// ============================================================================
//  GTA SA Android 2.10 ARM64 (libGTASA.so, arm64-v8a)  --  UNICO archivo con datos de version
// ============================================================================
//  Binario analizado: libGTASA.so 2.10 (aarch64, NO strippeada, exporta ~21k simbolos dinamicos)
//      SHA-256: 92b3714534742cf8e31eba35dbfd5e83871ecb5b936f5519d9b6ef3562234f9e
//  Todos los simbolos de abajo EXISTEN en su tabla .dynsym (verificado con `nm -D`), por lo que
//  se resuelven por dlsym (aml->GetSym). Los offsets son RVA del MISMO binario y solo se usan como
//  respaldo si el tamano del modulo coincide con kExpectedModuleSize (evita offsets en otra version).
//  Offset = direccion virtual - base de carga (el primer PT_LOAD empieza en 0).
//
//  NO encontrado en el binario (sin simbolo): CBaseModelInfo::GetRwObject/DeleteRwObject
//  -> por eso RemoveSkin no esta soportado todavia (ver Game.cpp / docs/REVERSING.md).
// ============================================================================
#include "game/versions/gta_sa_2_10_arm64.h"
#include "Log.h"

namespace GtaSA210Arm64 {
constexpr size_t kExpectedModuleSize = 0x11f1b80;  // fin del ultimo PT_LOAD

namespace Offsets {
constexpr uintptr_t RwStreamOpen = 0x280e44;  // _Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv
constexpr uintptr_t RwStreamClose = 0x280d24;  // _Z13RwStreamCloseP8RwStreamPv
constexpr uintptr_t RwStreamFindChunk = 0x27c9f4;  // _Z17RwStreamFindChunkP8RwStreamjPjS1_
constexpr uintptr_t RpClumpStreamRead = 0x2bba10;  // _Z17RpClumpStreamReadP8RwStream
constexpr uintptr_t RpClumpDestroy = 0x2baab0;  // _Z14RpClumpDestroyP7RpClump
constexpr uintptr_t RwTexDictionaryStreamRead = 0x26cb98;  // _Z25RwTexDictionaryStreamReadP8RwStream
constexpr uintptr_t RwTexDictionaryCreate = 0x273c78;  // _Z21RwTexDictionaryCreatev
constexpr uintptr_t RwTexDictionaryDestroy = 0x273d10;  // _Z22RwTexDictionaryDestroyP15RwTexDictionary
constexpr uintptr_t RwTexDictionaryAddTexture = 0x274120;  // _Z25RwTexDictionaryAddTextureP15RwTexDictionaryP9RwTexture
constexpr uintptr_t RwTexDictionaryGetCurrent = 0x27427c;  // _Z25RwTexDictionaryGetCurrentv
constexpr uintptr_t RwTexDictionarySetCurrent = 0x27425c;  // _Z25RwTexDictionarySetCurrentP15RwTexDictionary
constexpr uintptr_t RwImageCreate = 0x2709d8;  // _Z13RwImageCreateiii
constexpr uintptr_t RwImageAllocatePixels = 0x270b1c;  // _Z21RwImageAllocatePixelsP7RwImage
constexpr uintptr_t RwImageDestroy = 0x270a50;  // _Z14RwImageDestroyP7RwImage
constexpr uintptr_t RwRasterCreate = 0x272e34;  // _Z14RwRasterCreateiiii
constexpr uintptr_t RwRasterSetFromImage = 0x2725a4;  // _Z20RwRasterSetFromImageP8RwRasterP7RwImage
constexpr uintptr_t RwTextureCreate = 0x273f74;  // _Z15RwTextureCreateP8RwRaster
constexpr uintptr_t RwTextureSetName = 0x274000;  // _Z16RwTextureSetNameP9RwTexturePKc
constexpr uintptr_t FindPlayerPed = 0x4efae0;  // _Z13FindPlayerPedi
constexpr uintptr_t ModelInfoPtrs = 0xb666e0;  // _ZN10CModelInfo16ms_modelInfoPtrsE
constexpr uintptr_t PedModelInfo_SetClump = 0x45d514;  // _ZN13CPedModelInfo8SetClumpEP7RpClump
constexpr uintptr_t Ped_DeleteRwObject = 0x4cb698;  // _ZN7CEntity14DeleteRwObjectEv (la que usa Skins-selector)
constexpr uintptr_t Ped_SetModelIndex = 0x595998;  // _ZN4CPed13SetModelIndexEj
constexpr uintptr_t GameProcess = 0x4d62d4;  // _ZN5CGame7ProcessEv
constexpr uintptr_t TouchEvent = 0x31ec0c;  // _Z14AND_TouchEventiiii
constexpr uintptr_t SwapBuffers = 0x31e320;  // _Z21NVEventEGLSwapBuffersv
constexpr uintptr_t RequestSpecialChar = 0x39b2a0;       // _ZN10CStreaming18RequestSpecialCharEiPKci
constexpr uintptr_t LoadAllRequestedModels = 0x396b28;   // _ZN10CStreaming22LoadAllRequestedModelsEb
constexpr uintptr_t HasSpecialCharLoaded = 0x39b2a8;     // _ZN10CStreaming20HasSpecialCharLoadedEi
constexpr uintptr_t SetMissionDoesntRequireSpecialChar = 0x39b390;  // _ZN10CStreaming34SetMissionDoesntRequireSpecialCharEi
constexpr uintptr_t GetModelInfoByName = 0x45c6cc;       // _ZN10CModelInfo12GetModelInfoEPKcPi
constexpr uintptr_t FindPlayerVehicle = 0x4efdec;        // _Z17FindPlayerVehicleib
constexpr uintptr_t RebuildPlayer = 0x540cdc;            // _ZN8CClothes13RebuildPlayerEP10CPlayerPedb
constexpr uintptr_t TimerUserPause = 0xbdc594;           // _ZN6CTimer11m_UserPauseE (dato, .bss)
constexpr uintptr_t TimerCodePause = 0xbdc595;           // _ZN6CTimer11m_CodePauseE (dato, .bss)
constexpr uintptr_t LoadClumpFile = 0x4d054c;            // _ZN11CFileLoader13LoadClumpFileEP8RwStreamj
constexpr uintptr_t ClumpModelInfo_DeleteRwObject = 0x45b74c;  // _ZN15CClumpModelInfo14DeleteRwObjectEv
constexpr uintptr_t ClumpModelInfo_SetClump = 0x45b7f8;  // _ZN15CClumpModelInfo8SetClumpEP7RpClump
constexpr uintptr_t RtPNGImageRead = 0x2ad8c8;           // _Z14RtPNGImageReadPKc
constexpr uintptr_t RwImageFindRasterFormat = 0x2725e0;  // _Z23RwImageFindRasterFormatP7RwImageiPiS1_S1_S1_
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
        if (mod.size != kExpectedModuleSize) {
            Log::Error("Game function unavailable: %s (symbol missing and module size 0x%zx != expected 0x%zx)",
                       label, mod.size, (size_t)kExpectedModuleSize);
            return nullptr;
        }
        void* p = reinterpret_cast<void*>(mod.base + offset);
        if (mod.Contains(p)) { Log::Info("Resolved %s via offset 0x%zx", label, (size_t)offset); return p; }
        Log::Error("Offset for %s outside module range", label);
        return nullptr;
    }
    Log::Error("Game function unavailable: %s", label);
    return nullptr;
}
}  // namespace

#define BIND(dst, label, off, ...) \
    dst = reinterpret_cast<decltype(dst)>(Lookup(mod, sym, label, {__VA_ARGS__}, off))

void Resolve(const ModuleInfo& mod, const SymbolFn& sym, RwApi& rw, GameApi& g, HookTargets& h) {
    // RenderWare esta compilado como C++ en este binario: los nombres estan mangled.
    BIND(rw.RwStreamOpen, "RwStreamOpen", Offsets::RwStreamOpen, "_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv");
    BIND(rw.RwStreamClose, "RwStreamClose", Offsets::RwStreamClose, "_Z13RwStreamCloseP8RwStreamPv");
    BIND(rw.RwStreamFindChunk, "RwStreamFindChunk", Offsets::RwStreamFindChunk, "_Z17RwStreamFindChunkP8RwStreamjPjS1_");
    BIND(rw.RpClumpStreamRead, "RpClumpStreamRead", Offsets::RpClumpStreamRead, "_Z17RpClumpStreamReadP8RwStream");
    BIND(rw.RpClumpDestroy, "RpClumpDestroy", Offsets::RpClumpDestroy, "_Z14RpClumpDestroyP7RpClump");
    BIND(rw.RwTexDictionaryStreamRead, "RwTexDictionaryStreamRead", Offsets::RwTexDictionaryStreamRead, "_Z25RwTexDictionaryStreamReadP8RwStream");
    BIND(rw.RwTexDictionaryCreate, "RwTexDictionaryCreate", Offsets::RwTexDictionaryCreate, "_Z21RwTexDictionaryCreatev");
    BIND(rw.RwTexDictionaryDestroy, "RwTexDictionaryDestroy", Offsets::RwTexDictionaryDestroy, "_Z22RwTexDictionaryDestroyP15RwTexDictionary");
    BIND(rw.RwTexDictionaryAddTexture, "RwTexDictionaryAddTexture", Offsets::RwTexDictionaryAddTexture, "_Z25RwTexDictionaryAddTextureP15RwTexDictionaryP9RwTexture");
    BIND(rw.RwTexDictionaryGetCurrent, "RwTexDictionaryGetCurrent", Offsets::RwTexDictionaryGetCurrent, "_Z25RwTexDictionaryGetCurrentv");
    BIND(rw.RwTexDictionarySetCurrent, "RwTexDictionarySetCurrent", Offsets::RwTexDictionarySetCurrent, "_Z25RwTexDictionarySetCurrentP15RwTexDictionary");
    BIND(rw.RwImageCreate, "RwImageCreate", Offsets::RwImageCreate, "_Z13RwImageCreateiii");
    BIND(rw.RwImageAllocatePixels, "RwImageAllocatePixels", Offsets::RwImageAllocatePixels, "_Z21RwImageAllocatePixelsP7RwImage");
    BIND(rw.RwImageDestroy, "RwImageDestroy", Offsets::RwImageDestroy, "_Z14RwImageDestroyP7RwImage");
    BIND(rw.RwRasterCreate, "RwRasterCreate", Offsets::RwRasterCreate, "_Z14RwRasterCreateiiii");
    BIND(rw.RwRasterSetFromImage, "RwRasterSetFromImage", Offsets::RwRasterSetFromImage, "_Z20RwRasterSetFromImageP8RwRasterP7RwImage");
    BIND(rw.RwTextureCreate, "RwTextureCreate", Offsets::RwTextureCreate, "_Z15RwTextureCreateP8RwRaster");
    BIND(rw.RwTextureSetName, "RwTextureSetName", Offsets::RwTextureSetName, "_Z16RwTextureSetNameP9RwTexturePKc");

    BIND(g.FindPlayerPed, "FindPlayerPed", Offsets::FindPlayerPed, "_Z13FindPlayerPedi");
    g.modelInfoPtrs = reinterpret_cast<void**>(Lookup(mod, sym, "CModelInfo::ms_modelInfoPtrs",
        {"_ZN10CModelInfo16ms_modelInfoPtrsE"}, Offsets::ModelInfoPtrs));
    BIND(g.ClumpModelInfo_SetClump, "CPedModelInfo::SetClump", Offsets::PedModelInfo_SetClump, "_ZN13CPedModelInfo8SetClumpEP7RpClump");
    BIND(g.Entity_DeleteRwObject, "CEntity::DeleteRwObject", Offsets::Ped_DeleteRwObject, "_ZN7CEntity14DeleteRwObjectEv");
    BIND(g.Ped_SetModelIndex, "CPed::SetModelIndex", Offsets::Ped_SetModelIndex, "_ZN4CPed13SetModelIndexEj");

    BIND(g.Streaming_RequestSpecialChar, "CStreaming::RequestSpecialChar", Offsets::RequestSpecialChar, "_ZN10CStreaming18RequestSpecialCharEiPKci");
    BIND(g.Streaming_LoadAllRequestedModels, "CStreaming::LoadAllRequestedModels", Offsets::LoadAllRequestedModels, "_ZN10CStreaming22LoadAllRequestedModelsEb");
    BIND(g.Streaming_HasSpecialCharLoaded, "CStreaming::HasSpecialCharLoaded", Offsets::HasSpecialCharLoaded, "_ZN10CStreaming20HasSpecialCharLoadedEi");
    BIND(g.Streaming_SetMissionDoesntRequireSpecialChar, "CStreaming::SetMissionDoesntRequireSpecialChar", Offsets::SetMissionDoesntRequireSpecialChar, "_ZN10CStreaming34SetMissionDoesntRequireSpecialCharEi");
    BIND(g.ModelInfo_GetByName, "CModelInfo::GetModelInfo(name)", Offsets::GetModelInfoByName, "_ZN10CModelInfo12GetModelInfoEPKcPi");
    BIND(g.FindPlayerVehicle, "FindPlayerVehicle", Offsets::FindPlayerVehicle, "_Z17FindPlayerVehicleib");
    BIND(g.Clothes_RebuildPlayer, "CClothes::RebuildPlayer", Offsets::RebuildPlayer, "_ZN8CClothes13RebuildPlayerEP10CPlayerPedb");
    g.timerUserPause = reinterpret_cast<const volatile uint8_t*>(Lookup(mod, sym, "CTimer::m_UserPause", {"_ZN6CTimer11m_UserPauseE"}, Offsets::TimerUserPause));
    g.timerCodePause = reinterpret_cast<const volatile uint8_t*>(Lookup(mod, sym, "CTimer::m_CodePause", {"_ZN6CTimer11m_CodePauseE"}, Offsets::TimerCodePause));

    BIND(rw.RtPNGImageRead, "RtPNGImageRead", Offsets::RtPNGImageRead, "_Z14RtPNGImageReadPKc");
    BIND(rw.RwImageFindRasterFormat, "RwImageFindRasterFormat", Offsets::RwImageFindRasterFormat, "_Z23RwImageFindRasterFormatP7RwImageiPiS1_S1_S1_");
    BIND(g.FileLoader_LoadClumpFile, "CFileLoader::LoadClumpFile", Offsets::LoadClumpFile, "_ZN11CFileLoader13LoadClumpFileEP8RwStreamj");
    BIND(g.ClumpModelInfo_DeleteRwObject, "CClumpModelInfo::DeleteRwObject", Offsets::ClumpModelInfo_DeleteRwObject, "_ZN15CClumpModelInfo14DeleteRwObjectEv");
    g.clumpModelInfoSetClump = Lookup(mod, sym, "CClumpModelInfo::SetClump", {"_ZN15CClumpModelInfo8SetClumpEP7RpClump"}, Offsets::ClumpModelInfo_SetClump);

    h.gameProcess = Lookup(mod, sym, "CGame::Process (tick)", {"_ZN5CGame7ProcessEv"}, Offsets::GameProcess);
    h.touchEvent = Lookup(mod, sym, "AND_TouchEvent", {"_Z14AND_TouchEventiiii"}, Offsets::TouchEvent);
    h.swapBuffers = Lookup(mod, sym, "NVEventEGLSwapBuffers", {"_Z21NVEventEGLSwapBuffersv"}, Offsets::SwapBuffers);
}
}  // namespace GtaSA210Arm64
