# Cómo completar la Parte B (GTA SA 2.10 ARM64)

Solo análisis legítimo de **tu propia copia** del juego instalada en tu dispositivo.

## 1. ¿Hay símbolos?
```bash
llvm-nm -D --defined-only libGTASA.so | grep -E 'RwStream|RpClump|RwTexDict|RwImage|RwRaster|RwTexture'
llvm-nm -D --defined-only libGTASA.so | llvm-cxxfilt | grep -E 'CPed::|ModelInfo|FindPlayerPed|CGame'
```
Lo que aparezca: añade el nombre mangled a la lista de candidatos en `gta_sa_2_10_arm64.cpp` (se resuelve con `dlsym` y se valida dentro del módulo). Los nombres que ya figuran son **hipótesis sin verificar**.

## 2. Si está strippeada: offsets
1. Abre `libGTASA.so` (arm64-v8a, la de la versión 2.10 exacta) en Ghidra/IDA. Anota el **SHA-256** del binario en el comentario del archivo de versión.
2. RenderWare: busca cadenas/patrones del SDK 3.x (p. ej. mensajes de `RwStreamOpen`, cabeceras de chunk `0x10`/`0x16`, `0x1803FFFF`). Contrasta con el código de RW 3.6 y con proyectos de reversing de GTA SA (gta-reversed) para identificar firmas.
3. Juego: parte de `CModelInfo::ms_modelInfoPtrs` (array indexado por id de modelo, accedido desde `CStreaming` y `CEntity::SetModelIndex`), `FindPlayerPed` (acceso a la tabla de jugadores) y la vtable de `CPedModelInfo` (`SetClump`, `GetRwObject`, `DeleteRwObject`).
4. Offset = dirección − base de carga del módulo (RVA). Escríbelo en `Offsets::*`.
5. Verifica en dispositivo: `arcx.log` debe mostrar `Resolved <fn> via offset 0x...` y `Capabilities: ... applySkin=1`.

## 3. Estrategia implementada en `Game::ApplySkin`
Reemplazo del clump del model info del jugador (id 0):
`Entity::DeleteRwObject(ped)` → (`ModelInfo::DeleteRwObject` si había skin previa) → `PedModelInfo::SetClump(mi, clump)` → `Ped::SetModelIndex(ped, 0)`.

Riesgos conocidos (comprobar en pruebas):
* El sistema de ropa de CJ (`CClothes::RebuildPlayer`) puede reconstruir el modelo y pisar la skin.
* El streaming puede destruir el clump del model info y dejar colgante el puntero original usado por `RemoveSkin`.
* La DFF debe usar la jerarquía de huesos de un ped (skin de CJ/ped estándar).
* Estrategia alternativa (no implementada): cargar el modelo en un slot de ped especial/libre y cambiar el `modelIndex` del jugador a ese id; requiere conocer las funciones de `CStreaming` (`RequestSpecialModel`, etc.) — `// TODO: GTA SA 2.10 ARM64`.

## 4. Tick por frame y entrada táctil
* `GameProcess`: función llamada una vez por frame en el hilo del juego (prototipo asumido `void()`). Sin ella, ARCX aplica desde el hilo de render (inseguro) y lo registra.
* `TouchEvent`: el hook reenvía 5 enteros sin modificar; **interpretación pendiente** en `Menu::OnTouchRaw`. Verifica prototipo y coordenadas (¿pantalla o vista?) antes de fiarte.

## 5. Resultado del análisis de `libGTASA.so` 2.10 (aarch64)
* El binario **no está strippeado** y exporta unos 21 000 símbolos dinámicos: `llvm-nm -D` sirve; no hacen falta offsets.
* RenderWare está compilado como **C++**: los nombres son mangled, p. ej. `_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv`.
* El juego solo importa `eglGetDisplay/eglGetProcAddress/eglQueryString` y enlaza `libGLESv2.so` → contexto **GLES2**.
* Pendiente (requiere desensamblar): semántica de `CPedModelInfo::SetClump` (¿incrementa refs del TXD? ¿asume jerarquía/animaciones?), propiedad del clump al hacer streaming, layout de `CBaseModelInfo` para restaurar el clump original, y el significado de los 4 enteros de `AND_TouchEvent`.
