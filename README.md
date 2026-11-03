# Project ARCX — Skin Selector / Loader para GTA SA Android (AML, ARM64)

Plugin nativo `libARCX.so` (arm64-v8a) para **Android Mod Loader (AML)** con menú **Dear ImGui**.

## Estado real del proyecto (léelo primero)

| Parte | Estado |
|---|---|
| Escaneo de `modloader/`, validación de DFF/TXD, detección PNG/JPG | ✅ Implementado y probado en host |
| Config (`arcx_config.ini`), log (`arcx.log`), paginación, menú ImGui | ✅ Implementado (menú no probado en dispositivo) |
| Carga DFF/TXD vía RenderWare (`SkinLoader`) | ⚠️ Código escrito, **inactivo hasta resolver las funciones RW** de 2.10 ARM64 |
| Aplicar el modelo al jugador (`Game::ApplySkin`) | ⚠️ Código escrito (estrategia en `docs/REVERSING.md`), **inactivo hasta resolver funciones del juego** |
| Símbolos GTA SA 2.10 ARM64 | ✅ Verificados con `nm -D` sobre `libGTASA.so` 2.10 (SHA-256 en `gta_sa_2_10_arm64.cpp`): RenderWare, `FindPlayerPed`, `CPed::SetModelIndex`, `CPedModelInfo::SetClump`, `CGame::Process`, `AND_TouchEvent`, `NVEventEGLSwapBuffers`. Offsets de respaldo solo si el tamaño del módulo coincide. |
| Quitar skin / restaurar el modelo original | ❌ No hay símbolo para `CBaseModelInfo::GetRwObject`; `RemoveSkin` devuelve error (TODO) |
| Entrada táctil para el menú | ⚠️ `AND_TouchEvent(int,int,int,int)` hookeado (prototipo verificado); el significado de los 4 argumentos está sin verificar: los primeros eventos se registran en `arcx.log` (`Touch raw: ...`) |
| Dibujo del menú | ⚠️ Se engancha `NVEventEGLSwapBuffers` (el juego usa GLES2 → ImGui con backend ES2, `#version 100`) |
| Compilación en CI / ejecución en dispositivo | ⚠️ **No he podido compilarlo con el NDK ni probarlo en Android.** Solo se comprobó sintaxis de los módulos portables en host. Espera ajustar errores en el primer build. |

El plugin registra en `arcx.log` cada función resuelta (`Resolved ... via symbol`) o faltante (`Game function unavailable`). Aplicar una skin sigue **sin estar probado en dispositivo**: el reemplazo del clump del modelo del jugador es una estrategia no verificada (ver `docs/REVERSING.md`).

## A) Parte fácil vs B) Parte crítica

**A (hecha):** `FileScanner` (carpetas, DFF/TXD/imágenes, elección del modelo por nombre de carpeta → `skin.*` → alfabético), `Config`, `Log`, `ImGuiMenu`, `SkinManager`.

**B (depende del binario):**

| Qué falta | Por qué | Cómo conectarlo | Archivo |
|---|---|---|---|
| Funciones RenderWare (`RwStreamOpen`, `RpClumpStreamRead`, `RwTexDictionaryStreamRead`, …) | No conozco sus RVA en 2.10 ARM64; los símbolos podrían no exportarse | `nm`/Ghidra, rellenar `Offsets::*` | `src/game/versions/gta_sa_2_10_arm64.cpp` |
| `FindPlayerPed`, `CModelInfo::ms_modelInfoPtrs`, `CPedModelInfo::SetClump`, `CPed::SetModelIndex`, `CPed::DeleteRwObject`, `CBaseModelInfo::{Get,Delete}RwObject` | Ídem; además deben ser los *overrides* más derivados | Ídem | mismo archivo |
| Tick por frame en el hilo del juego | Aplicar la skin desde el hilo de render causa condiciones de carrera | Hookear una función llamada cada frame | mismo archivo (`GameProcess`) |
| Entrada táctil | ImGui necesita eventos de toque; el prototipo del callback es desconocido | Verificar prototipo; ajustar `Menu::OnTouchRaw` | `gta_sa_2_10_arm64.cpp` + `ImGuiMenu.cpp` |
| Layout de `RwImage`/`RwRaster` 2.10 y formato de raster móvil | La conversión PNG/JPG→textura es **experimental** | Verificar en el binario | `game/GameApi.h`, `SkinLoader.cpp` |

Guía de ingeniería inversa: [`docs/REVERSING.md`](docs/REVERSING.md).

## Arquitectura

```
FileScanner → SkinManager → SkinLoader → RenderWare (RwApi) → Game (GameApi) → hilo del juego
                                              ▲                    ▲
                                  game/versions/gta_sa_2_10_arm64.cpp  (ÚNICO sitio con direcciones)
AmlCompat  → única capa que toca la API de AML
```

* Carga bajo demanda: solo **una** skin residente; se libera la anterior al aplicar otra; límite `max_resident_mb`; comprobación de `MemAvailable`; TXD con caché/refcount; si la skin ya está cargada se reutiliza (`RefreshPlayer`).
* Seguridad: sin symlinks, tamaños máximos, cabecera RW validada, direcciones validadas dentro del rango de `libGTASA.so`, nunca se ejecuta código de las carpetas de skins.
* Propiedad: tras `ApplySkin` el **juego** posee el clump; ARCX no lo destruye (evita doble free).

## Instalación

1. Compila con GitHub Actions (push → artefacto `libARCX-arm64-v8a` → `libs/arm64-v8a/libARCX.so`) o localmente (abajo).
2. Copia `libARCX.so` a la carpeta de mods de AML (`.../AML/mods/` según tu instalación de AML).
3. Skins (la ruta se autodetecta; se puede forzar con `modloader_path` en `arcx_config.ini`):

```
Android_unprotected/data/com.rockstargames.gtasa/modloader/
├── ghost/   ghost.dff  ghost.txd
├── venom/   venom_model.dff  venom_textures.txd   (nombres libres)
└── soldier/ soldier.dff  soldier.txd  extra.png
```
Imágenes PNG/JPG: potencia de 2, ≤2048 px (no se reescalan).
**Aviso:** toda subcarpeta con un `.dff` válido aparece como skin (también modelos de vehículos de otros mods); usa `modloader_path` apuntando a una carpeta dedicada si hace falta.

Archivos generados en `.../com.rockstargames.gtasa/arcx/`: `arcx.log`, `arcx_config.ini`.

## Uso

1. Inicia GTA SA. 2. Pulsa el botón **ARCX** (esquina superior izquierda). 3. Pestaña **SKINS**. 4. Elige una skin. 5. **APLICAR SKIN**.
(Hasta completar la sección B, el paso 5 registrará el error en `arcx.log`.)

## Compilación local

```bash
bash scripts/fetch_deps.sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release
cmake --build build   # → build/libARCX.so
```

## AML: puntos a verificar contra tu revisión

`src/AmlCompat.cpp` y `main.cpp` asumen la API de RusJJ/AndroidModLoader: `#include <mod/amlmod.h>`, macro `MYMODCFG(guid, nombre, versión, autor)`, `extern "C" void OnModLoad()`, y `aml->GetLibHandle / GetLib / GetSym / Hook`. Confírmalo en los headers clonados (`third_party/AndroidModLoader`); AML no integra ImGui por sí mismo, por eso se engancha `eglSwapBuffers` y se usa el backend OpenGL ES 3. Si tu CMake no encuentra `mod/amlmod.h`, ajusta `${AML}` en `CMakeLists.txt`.

## Diferencias respecto a la estructura pedida

Se omitieron `Android.mk`, `Application.mk` y `build.gradle` (innecesarios con CMake + NDK). Se añadió `AmlCompat` (capa AML) y `game/` (capa de versión).
