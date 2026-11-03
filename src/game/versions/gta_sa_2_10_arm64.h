#pragma once
#include <functional>
#include "game/GameApi.h"

namespace GtaSA210Arm64 {
using SymbolFn = std::function<void*(const char*)>;  // dlsym-like sobre libGTASA.so

// Resuelve cada funcion por (1) simbolo exportado, (2) offset manual. Nunca inventa direcciones:
// lo que no se resuelve queda en nullptr y se registra "[ARCX][ERROR] Game function unavailable".
void Resolve(const ModuleInfo& mod, const SymbolFn& sym, RwApi& rw, GameApi& game, HookTargets& hooks);
}
