#include <EGL/egl.h>
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <filesystem>
#include <string>
#include <mod/amlmod.h>
#include "AmlCompat.h"
#include "Config.h"
#include "Game.h"
#include "ImGuiMenu.h"
#include "Log.h"
#include "SkinManager.h"

// MYMODCFG enlaza AML::Config (implementada en mod/config.cpp, que no forma parte de la libreria):
// da "undefined symbol: Config::Config(char const*)". MYMOD no la necesita.
#ifdef MYMOD
MYMOD(com.arcx.skinloader, Project ARCX, 0.1.0, ARCX)
#else
#error "Tu revision de AML no define MYMOD: revisa third_party/AndroidModLoader/mod/amlmod.h"
#endif

namespace {
EGLBoolean (*g_origSwap)(EGLDisplay, EGLSurface) = nullptr;

void RenderOverlay(EGLDisplay d, EGLSurface s) {
    EGLint w = 0, h = 0;
    EGLBoolean okW = eglQuerySurface(d, s, EGL_WIDTH, &w);
    EGLBoolean okH = eglQuerySurface(d, s, EGL_HEIGHT, &h);
    static int diag = 0;
    if (diag < 3) {
        ++diag;
        Log::Info("Swap #%d: display=%p surface=%p ctx=%p size=%dx%d query=%d/%d", diag, d, s, eglGetCurrentContext(), w, h, (int)okW, (int)okH);
        if (!eglGetCurrentContext()) Log::Error("No EGL context current in the swap hook thread: nothing can be drawn here");
    }
    if (w > 0 && h > 0) {
        if (!Game::IsTickHooked()) SkinManager::Get().OnGameTick();  // ultimo recurso (hilo de render, inseguro)
        Menu::Render(w, h);
    }
}
// Via principal: swap del propio juego (NVEventEGLSwapBuffers), contexto GL del juego ya activo.
void OnGameSwap() { RenderOverlay(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW)); }
// Respaldo: swap generico de libEGL.
EGLBoolean Hooked_eglSwapBuffers(EGLDisplay d, EGLSurface s) {
    RenderOverlay(d, s);
    return g_origSwap(d, s);
}

void GameTick() { SkinManager::Get().OnGameTick(); }

bool OnTouchRaw(int a, int b, int c, int d) { return Menu::OnTouchRaw(a, b, c, d); }

ModuleInfo FindModule(const char* soName) {
    struct Ctx { const char* name; ModuleInfo mi; } ctx{soName, {}};
    dl_iterate_phdr([](dl_phdr_info* info, size_t, void* p) -> int {
        auto* c = (Ctx*)p;
        if (!info->dlpi_name || !strstr(info->dlpi_name, c->name)) return 0;
        uintptr_t lo = ~(uintptr_t)0, hi = 0;
        for (int i = 0; i < info->dlpi_phnum; ++i) {
            const ElfW(Phdr)& ph = info->dlpi_phdr[i];
            if (ph.p_type != PT_LOAD) continue;
            lo = std::min<uintptr_t>(lo, ph.p_vaddr);
            hi = std::max<uintptr_t>(hi, ph.p_vaddr + ph.p_memsz);
        }
        if (hi > lo) { c->mi.base = info->dlpi_addr + lo; c->mi.size = hi - lo; }
        return 1;
    }, &ctx);
    return ctx.mi;
}
}  // namespace

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    namespace fs = std::filesystem;
    std::string dataDir = Paths::DetectGameDataDir();
    std::string arcxDir = dataDir.empty() ? "/sdcard/" : dataDir + "arcx/";
    std::error_code ec;
    fs::create_directories(arcxDir, ec);
    Log::Init(arcxDir + "arcx.log");
    Log::Info("Plugin initialized");
    if (dataDir.empty()) Log::Error("Game data dir not found; configure modloader_path in arcx_config.ini");

    // ZyZ ModLoader: .../files/modloader   |   Cargador propio de ARCX: .../files/ARCXLoader
    const std::string zyzRoot = dataDir + "files/modloader/";
    const std::string arcxRoot = dataDir + "files/ARCXLoader/";
    if (!dataDir.empty()) fs::create_directories(arcxRoot, ec);   // para que el usuario vea donde dejar las skins
    SkinManager::Get().Init(arcxDir + "arcx_config.ini", zyzRoot, arcxRoot);

    ModuleInfo mod = FindModule("libGTASA.so");
    void* handle = Aml::GetLibHandle("libGTASA.so");
    if (!mod.base) mod.base = Aml::GetLibBase("libGTASA.so");  // sin tamano: Contains() sera falso -> nada se resuelve
    Game::Initialize(mod, [handle](const char* n) { return Aml::GetSym(handle, n); }, Aml::Hook);
    Game::SetTickCallback(GameTick);
    Game::SetTouchCallback(OnTouchRaw);
    Game::SetSwapCallback(OnGameSwap);

    // Via principal: eglSwapBuffers (la misma que usa Skins-selector, comprobada en dispositivo).
    // Respaldo: NVEventEGLSwapBuffers del propio juego. Nunca las dos a la vez (se dibujaria doble).
    void* swapFn = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    bool eglOk = swapFn && Aml::Hook(swapFn, (void*)&Hooked_eglSwapBuffers, (void**)&g_origSwap);
    Log::Info("Menu hook eglSwapBuffers (%p): %s", swapFn, eglOk ? "ok" : "FAILED");
    Game::HookFunctions(!eglOk);
    if (!eglOk) Log::Info("Menu hook fallback: NVEventEGLSwapBuffers %s", Game::IsSwapHooked() ? "ok" : "FAILED");
}
