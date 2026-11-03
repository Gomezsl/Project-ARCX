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

EGLBoolean Hooked_eglSwapBuffers(EGLDisplay d, EGLSurface s) {
    EGLint w = 0, h = 0;
    eglQuerySurface(d, s, EGL_WIDTH, &w);
    eglQuerySurface(d, s, EGL_HEIGHT, &h);
    if (w > 0 && h > 0) {
        if (!Game::IsTickHooked()) SkinManager::Get().OnGameTick();  // ultimo recurso (hilo de render, inseguro)
        Menu::Render(w, h);
    }
    return g_origSwap(d, s);
}

void GameTick() { SkinManager::Get().OnGameTick(); }

// TODO: GTA SA 2.10 ARM64 -- interpretar (a,b,c,d) del callback de toque una vez verificado su prototipo.
void OnTouchRaw(int a, int b, int c, int d) { Menu::OnTouchRaw(a, b, c, d); }

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

extern "C" void OnModLoad() {
    namespace fs = std::filesystem;
    std::string dataDir = Paths::DetectGameDataDir();
    std::string arcxDir = dataDir.empty() ? "/sdcard/" : dataDir + "arcx/";
    std::error_code ec;
    fs::create_directories(arcxDir, ec);
    Log::Init(arcxDir + "arcx.log");
    Log::Info("Plugin initialized");
    if (dataDir.empty()) Log::Error("Game data dir not found; configure modloader_path in arcx_config.ini");

    SkinManager::Get().Init(arcxDir + "arcx_config.ini", dataDir + "modloader/");

    ModuleInfo mod = FindModule("libGTASA.so");
    void* handle = Aml::GetLibHandle("libGTASA.so");
    if (!mod.base) mod.base = Aml::GetLibBase("libGTASA.so");  // sin tamano: Contains() sera falso -> nada se resuelve
    Game::Initialize(mod, [handle](const char* n) { return Aml::GetSym(handle, n); }, Aml::Hook);
    Game::SetTickCallback(GameTick);
    Game::SetTouchCallback(OnTouchRaw);
    Game::HookFunctions();

    void* egl = dlopen("libEGL.so", RTLD_NOW);
    void* swap = egl ? dlsym(egl, "eglSwapBuffers") : nullptr;
    if (!Aml::Hook(swap, (void*)&Hooked_eglSwapBuffers, (void**)&g_origSwap))
        Log::Error("Failed to hook eglSwapBuffers: el menu ImGui no se dibujara");
}
