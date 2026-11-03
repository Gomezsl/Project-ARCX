#pragma once
// Tipos RenderWare (SDK 3.x publico) y tabla de punteros a funciones del motor.
// NINGUNA direccion vive aqui: se resuelven en game/versions/<version>.cpp
#include <cstddef>
#include <cstdint>

struct RwStream;
struct RpClump;
struct RwTexDictionary;
struct RwTexture;
struct RwRaster;

struct RwMemory { uint8_t* start; uint32_t length; };

// RwImage (layout LP64). VERIFICAR contra el binario 2.10 antes de confiar en el.
struct RwImage {
    int32_t flags, width, height, depth, stride;
    uint8_t* cpPixels;
    void* palette;
};
#if INTPTR_MAX == INT64_MAX
static_assert(offsetof(RwImage, cpPixels) == 24, "RwImage layout LP64");
#endif

enum : uint32_t {
    rwSTREAMMEMORY = 3, rwSTREAMREAD = 1,
    rwRASTERTYPETEXTURE = 0x04, rwRASTERFORMAT8888 = 0x0500,
};

struct RwApi {
    RwStream* (*RwStreamOpen)(uint32_t type, uint32_t access, const void* data) = nullptr;
    int (*RwStreamClose)(RwStream*, void* data) = nullptr;
    int (*RwStreamFindChunk)(RwStream*, uint32_t type, uint32_t* len, uint32_t* ver) = nullptr;
    RpClump* (*RpClumpStreamRead)(RwStream*) = nullptr;
    int (*RpClumpDestroy)(RpClump*) = nullptr;
    RwTexDictionary* (*RwTexDictionaryStreamRead)(RwStream*) = nullptr;
    RwTexDictionary* (*RwTexDictionaryCreate)() = nullptr;
    int (*RwTexDictionaryDestroy)(RwTexDictionary*) = nullptr;
    RwTexture* (*RwTexDictionaryAddTexture)(RwTexDictionary*, RwTexture*) = nullptr;
    RwTexDictionary* (*RwTexDictionaryGetCurrent)() = nullptr;
    RwTexDictionary* (*RwTexDictionarySetCurrent)(RwTexDictionary*) = nullptr;
    RwImage* (*RwImageCreate)(int w, int h, int depth) = nullptr;
    RwImage* (*RwImageAllocatePixels)(RwImage*) = nullptr;
    int (*RwImageDestroy)(RwImage*) = nullptr;
    RwRaster* (*RwRasterCreate)(int w, int h, int depth, uint32_t flags) = nullptr;
    RwRaster* (*RwRasterSetFromImage)(RwRaster*, RwImage*) = nullptr;
    RwTexture* (*RwTextureCreate)(RwRaster*) = nullptr;
    RwTexture* (*RwTextureSetName)(RwTexture*, const char*) = nullptr;

    bool CanLoadModel() const {
        return RwStreamOpen && RwStreamClose && RwStreamFindChunk && RpClumpStreamRead && RpClumpDestroy &&
               RwTexDictionaryStreamRead && RwTexDictionaryCreate && RwTexDictionaryDestroy &&
               RwTexDictionaryGetCurrent && RwTexDictionarySetCurrent;
    }
    bool CanLoadImages() const {
        return RwTexDictionaryAddTexture && RwImageCreate && RwImageAllocatePixels && RwImageDestroy &&
               RwRasterCreate && RwRasterSetFromImage && RwTextureCreate && RwTextureSetName;
    }
};

// Funciones del juego (miembros llamados como funcion libre con `this` en x0, ABI AAPCS64).
// Cada una debe apuntar al override MAS DERIVADO (CPed / CPedModelInfo), no a la base.
struct GameApi {
    void* (*FindPlayerPed)(int playerId) = nullptr;                    // FindPlayerPed(-1)
    void** modelInfoPtrs = nullptr;                                    // CModelInfo::ms_modelInfoPtrs (dato)
    void* (*ModelInfo_GetRwObject)(void* modelInfo) = nullptr;         // CBaseModelInfo::GetRwObject
    void (*ModelInfo_DeleteRwObject)(void* modelInfo) = nullptr;       // CBaseModelInfo::DeleteRwObject
    void (*ClumpModelInfo_SetClump)(void* modelInfo, RpClump*) = nullptr;  // CPedModelInfo::SetClump
    void (*Entity_DeleteRwObject)(void* entity) = nullptr;             // CPed::DeleteRwObject
    void (*Ped_SetModelIndex)(void* ped, uint32_t modelId) = nullptr;  // CPed::SetModelIndex

    bool CanApplySkin() const {
        return FindPlayerPed && modelInfoPtrs && ModelInfo_GetRwObject && ModelInfo_DeleteRwObject &&
               ClumpModelInfo_SetClump && Entity_DeleteRwObject && Ped_SetModelIndex;
    }
};

// Funciones usadas solo para hooks.
struct HookTargets {
    void* gameProcess = nullptr;  // funcion llamada una vez por frame en el hilo del juego (void(*)())
    void* touchEvent = nullptr;   // callback de toque del juego (ver README, seccion Input)
};

struct ModuleInfo {
    uintptr_t base = 0;
    size_t size = 0;
    bool Contains(const void* p) const {
        uintptr_t a = (uintptr_t)p;
        return base && a >= base && a < base + size;
    }
};
