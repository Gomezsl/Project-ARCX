# Project ARCX — Skin Selector / Loader para GTA SA Android (AML, ARM64)

Plugin nativo `libARCX.so` (arm64-v8a) para **Android Mod Loader (AML)** con menú **Dear ImGui**.

## Estado real del proyecto (léelo primero)

| Parte | Estado |
|---|---|
| Escaneo de `modloader/`, validación de DFF/TXD, detección PNG/JPG | ✅ Implementado y probado en host |
| Config (`arcx_config.ini`), log (`arcx.log`), paginación, menú ImGui | ✅ Implementado (menú no probado en dispositivo) |
| Carga DFF/TXD vía RenderWare (`SkinLoader`) | ⚠️ Código escrito, **inactivo hasta resolver las funciones RW** de 2.10 ARM64 |
| Aplicar el modelo al jugador (`Game::ApplySkin`) | ⚠️ Código escrito (estrategia en `docs/REVERSING.md`), **inactivo hasta resolver funciones del juego** |
| Offsets/símbolos GTA SA 2.10 ARM64 | ❌ **Todos en 0 (TODO)**. No se han inventado. |
| Entrada táctil para el menú | ❌ Hook de toque sin verificar (símbolo/prototipo desconocidos) |
| Compilación en CI / ejecución en dispositivo | ⚠️ **No he podido compilarlo con el NDK ni probarlo en Android.** Solo se comprobó sintaxis de los módulos portables en host. Espera ajustar errores en el primer build. |

Mientras falten las direcciones, el plugin arranca, escanea, muestra el menú y registra en `arcx.log`
`Game function unavailable: <función> (TODO: GTA SA 2.10 ARM64)`; **no cambia ninguna skin** (no hay falsos positivos).

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
