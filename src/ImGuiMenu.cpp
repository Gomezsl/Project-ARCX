#include "ImGuiMenu.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>
#include "Log.h"
#include "SkinManager.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

namespace {
struct TouchEv { int action; float x, y; };  // 0 down, 1 move, 2 up
std::mutex g_mx;
std::vector<TouchEv> g_events;
bool g_init = false;
bool g_open = false;
int g_page = 0;
// Rectangulos (px) del ultimo frame, para decidir en el hilo del juego si un toque es del menu.
std::atomic<int> g_btn[4], g_win[4];  // x0,y0,x1,y1
std::atomic<bool> g_captured{false};
bool In(const std::atomic<int>* r, int x, int y) {
    return x >= r[0] && x <= r[2] && y >= r[1] && y <= r[3];
}
}  // namespace

namespace Menu {

bool OnTouchRaw(int a, int b, int c, int d) {
    // Mapeo VERIFICADO con arcx.log de un Samsung A14: a=2 DOWN, a=3 MOVE, a=1 UP; b=dedo; c=x, d=y (pixeles).
    static int logged = 0;
    if (logged < 8) { ++logged; Log::Info("Touch raw: a=%d b=%d c=%d d=%d", a, b, c, d); }
    int act = (a == 2) ? 0 : (a == 3) ? 1 : (a == 1) ? 2 : -1;
    if (act < 0) return false;
    bool inside = In(g_btn, c, d) || In(g_win, c, d);
    bool consume;
    if (act == 0) { if (inside) g_captured = true; consume = inside; }
    else if (act == 2) { consume = g_captured.exchange(false); }
    else consume = g_captured;
    if (b == 0) {
        std::lock_guard<std::mutex> lk(g_mx);
        if (g_events.size() < 64) g_events.push_back({act, (float)c, (float)d});
    }
    return consume;
}

static void Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 12; st.FrameRounding = 8; st.TabRounding = 8; st.GrabRounding = 8;
    st.ItemSpacing = ImVec2(10, 12);
    ImGui_ImplOpenGL3_Init("#version 100");  // el juego enlaza libGLESv2 (contexto GLES2)
    g_init = true;
}

void Render(int w, int h) {
    if (!g_init) Init();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.FontGlobalScale = std::max(1.0f, h / 540.0f);

    {
        std::lock_guard<std::mutex> lk(g_mx);
        for (auto& e : g_events) {
            io.AddMousePosEvent(e.x, e.y);
            if (e.action == 0) io.AddMouseButtonEvent(0, true);
            if (e.action == 2) io.AddMouseButtonEvent(0, false);
        }
        g_events.clear();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    auto& sm = SkinManager::Get();
    const float fs = io.FontGlobalScale;

    // Boton flotante
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::Begin("##arcx_btn", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("ARCX")) g_open = !g_open;
    {
        ImVec2 p = ImGui::GetWindowPos(), z = ImGui::GetWindowSize();
        g_btn[0] = (int)p.x; g_btn[1] = (int)p.y; g_btn[2] = (int)(p.x + z.x); g_btn[3] = (int)(p.y + z.y);
    }
    ImGui::End();
    if (!g_open) { for (auto& v : g_win) v = -1; }

    if (g_open) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.85f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##arcx", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        float tw = ImGui::CalcTextSize("PROJECT ARCX").x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - tw) * 0.5f);
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "PROJECT ARCX");
        ImGui::Separator();

        auto view = sm.Snapshot();
        if (!view.gameReady)
            ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "Integracion con el juego incompleta (ver arcx.log)");

        if (ImGui::BeginTabBar("##tabs")) {
            if (ImGui::BeginTabItem("SKINS")) {
                const int per = std::max(3, sm.Cfg().skinsPerPage);
                const int total = (int)view.names.size();
                const int pages = std::max(1, (total + per - 1) / per);
                g_page = std::clamp(g_page, 0, pages - 1);
                for (int i = g_page * per; i < std::min(total, (g_page + 1) * per); ++i) {
                    std::string label = std::string(i == view.selected ? "> " : "  ") + view.names[i] + (i == view.active ? "   [ACTIVA]" : "");
                    if (ImGui::Selectable(label.c_str(), i == view.selected, 0, ImVec2(0, 28 * fs))) sm.Select(i);
                }
                if (total == 0) ImGui::TextDisabled("No hay skins en %s", view.modloaderPath.c_str());
                ImGui::Separator();
                if (ImGui::Button("<") && g_page > 0) --g_page;
                ImGui::SameLine(); ImGui::Text("%d / %d", g_page + 1, pages);
                ImGui::SameLine(); if (ImGui::Button(">") && g_page < pages - 1) ++g_page;
                ImGui::Spacing();
                ImGui::BeginDisabled(view.selected < 0);
                if (ImGui::Button("APLICAR SKIN")) sm.RequestApply();
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("QUITAR SKIN")) sm.RequestRemove();
                ImGui::TextWrapped("%s", view.status.c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("AJUSTES")) {
                ArcxConfig& c = sm.Cfg();
                bool changed = false;
                changed |= ImGui::Checkbox("Recordar skin", &c.rememberSkin);
                changed |= ImGui::Checkbox("Aplicar al iniciar", &c.applyOnStart);
                if (changed) sm.SaveConfig();
                if (ImGui::Button("REESCANEAR")) { sm.Rescan(); g_page = 0; }
                ImGui::TextWrapped("Ruta: %s", view.modloaderPath.c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        {
            ImVec2 p = ImGui::GetWindowPos(), z = ImGui::GetWindowSize();
            g_win[0] = (int)p.x; g_win[1] = (int)p.y; g_win[2] = (int)(p.x + z.x); g_win[3] = (int)(p.y + z.y);
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Shutdown() {
    if (!g_init) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    g_init = false;
}
}  // namespace Menu
