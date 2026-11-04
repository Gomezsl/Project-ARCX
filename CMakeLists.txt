cmake_minimum_required(VERSION 3.22)
project(ARCX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# NO ocultar simbolos: AML busca OnModLoad/__GetModInfo con dlsym y necesitan visibilidad por defecto.

set(TP ${CMAKE_SOURCE_DIR}/third_party)
set(IMGUI ${TP}/imgui)
set(AML ${TP}/AndroidModLoader)

foreach(d IN ITEMS ${IMGUI}/imgui.cpp ${AML} ${TP}/stb/stb_image.h)
  if(NOT EXISTS ${d})
    message(FATAL_ERROR "Falta ${d}. Ejecuta scripts/fetch_deps.sh")
  endif()
endforeach()

add_library(ARCX SHARED
  src/main.cpp
  src/AmlCompat.cpp
  src/Log.cpp
  src/Config.cpp
  src/FileScanner.cpp
  src/SkinManager.cpp
  src/SkinLoader.cpp
  src/Game.cpp
  src/ImGuiMenu.cpp
  src/game/versions/gta_sa_2_10_arm64.cpp
  ${IMGUI}/imgui.cpp
  ${IMGUI}/imgui_draw.cpp
  ${IMGUI}/imgui_tables.cpp
  ${IMGUI}/imgui_widgets.cpp
  ${IMGUI}/backends/imgui_impl_opengl3.cpp
)

# ImGui con simbolos ocultos: otros mods (CheatMenu, etc.) traen su propio ImGui y, si el linker dinamico
# enlazara nuestras llamadas con las suyas, compartiriamos GImGui y se romperia el dibujo.
set_source_files_properties(
  ${IMGUI}/imgui.cpp ${IMGUI}/imgui_draw.cpp ${IMGUI}/imgui_tables.cpp ${IMGUI}/imgui_widgets.cpp
  ${IMGUI}/backends/imgui_impl_opengl3.cpp
  PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden;-fvisibility-inlines-hidden")

target_include_directories(ARCX PRIVATE
  src ${IMGUI} ${IMGUI}/backends ${TP}/stb
  ${AML} ${AML}/include   # amlmod.h se incluye como <mod/amlmod.h>; verifica el layout de tu revision de AML
)
target_compile_definitions(ARCX PRIVATE IMGUI_IMPL_OPENGL_ES2 IMGUI_DISABLE_DEMO_WINDOWS)
target_compile_options(ARCX PRIVATE -Wall -Wextra -Wno-unused-parameter -fno-rtti)
target_link_libraries(ARCX PRIVATE log android EGL GLESv2 dl)
target_link_options(ARCX PRIVATE -Wl,--gc-sections)
# AML (64 bits) carga solo mods cuyo nombre sigue la convencion libNombre64.so (como libCheatMenu64.so)
set_target_properties(ARCX PROPERTIES OUTPUT_NAME "ARCX64")
