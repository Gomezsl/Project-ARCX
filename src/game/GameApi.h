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
    RpClump* (*RpClumpStreamRead)(RwStream*) = nullptr;   // ya no se usa: el cargador usa CFileLoader::LoadClumpFile
    int (*RpClumpDestroy)(RpClump*) = nullptr;
    RwImage* (*RtPNGImageRead)(const char* path) = nullptr;           // decodificador PNG del propio juego
    int (*RwImageFindRasterFormat)(RwImage*, int rasterType, int* w, int* h, int* depth, int* format) = nullptr;
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
        return RwStreamOpen && RwStreamClose && RwTexDictionaryStreamRead && RwStreamFindChunk &&
               RwTexDictionaryCreate && RwTexDictionaryDestroy && RwTexDictionaryGetCurrent && RwTexDictionarySetCurrent;
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
    void (*ClumpModelInfo_SetClump)(void* modelInfo, RpClump*) = nullptr;  // CPedModelInfo::SetClump (sin uso)
    // --- Cargador propio (tecnica observada en libCustomCharacter64.so, reimplementada) ---
    void (*FileLoader_LoadClumpFile)(RwStream*, uint32_t modelId) = nullptr;  // CFileLoader::LoadClumpFile
    void (*ClumpModelInfo_DeleteRwObject)(void* modelInfo) = nullptr;         // CClumpModelInfo::DeleteRwObject
    void* clumpModelInfoSetClump = nullptr;                                   // CClumpModelInfo::SetClump (solo para hook de verificacion)
    void (*Entity_DeleteRwObject)(void* entity) = nullptr;             // CPed::DeleteRwObject
    void (*Ped_SetModelIndex)(void* ped, uint32_t modelId) = nullptr;  // CPed::SetModelIndex

    // --- Via "personaje especial" (la que usa Skins-selector): el juego carga <nombre>.dff/.txd por nombre ---
    void (*Streaming_RequestSpecialChar)(int slot, const char* name, int flags) = nullptr;
    void (*Streaming_LoadAllRequestedModels)(bool) = nullptr;
    bool (*Streaming_HasSpecialCharLoaded)(int slot) = nullptr;
    void (*Streaming_SetMissionDoesntRequireSpecialChar)(int slot) = nullptr;
    void* (*ModelInfo_GetByName)(const char* name, int* outIndex) = nullptr;  // CModelInfo::GetModelInfo(name,&idx)
    void* (*FindPlayerVehicle)(int playerId, bool) = nullptr;
    void (*Clothes_RebuildPlayer)(void* playerPed, bool) = nullptr;           // volver a CJ
    const volatile uint8_t* timerUserPause = nullptr;                         // CTimer::m_UserPause
    const volatile uint8_t* timerCodePause = nullptr;                         // CTimer::m_CodePause

    bool CanApplySpecialChar() const {
        return FindPlayerPed && Entity_DeleteRwObject && Ped_SetModelIndex && Streaming_RequestSpecialChar &&
               Streaming_LoadAllRequestedModels && Streaming_HasSpecialCharLoaded && ModelInfo_GetByName;
    }
    bool CanApplySkin() const {   // cargador propio
        return FindPlayerPed && modelInfoPtrs && FileLoader_LoadClumpFile && ClumpModelInfo_DeleteRwObject &&
               Entity_DeleteRwObject && Ped_SetModelIndex;
    }
};

// Funciones usadas solo para hooks.
struct HookTargets {
    void* gameProcess = nullptr;  // funcion llamada una vez por frame en el hilo del juego (void(*)())
    void* touchEvent = nullptr;   // AND_TouchEvent(int,int,int,int)
    void* swapBuffers = nullptr;  // NVEventEGLSwapBuffers(): hilo de render, contexto GL activo
};

struct ModuleInfo {
    uintptr_t base = 0;
    size_t size = 0;
    bool Contains(const void* p) const {
        uintptr_t a = (uintptr_t)p;
        return base && a >= base && a < base + size;
    }
};
