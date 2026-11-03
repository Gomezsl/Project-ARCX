#include "AmlCompat.h"
#include <mod/amlmod.h>  // RusJJ/AndroidModLoader: declara `aml` (IAML*). VERIFICAR nombres contra tu revision.

namespace Aml {
void* GetLibHandle(const char* name) { return aml->GetLibHandle(name); }
uintptr_t GetLibBase(const char* name) { return aml->GetLib(name); }
void* GetSym(void* handle, const char* symbol) { return handle ? aml->GetSym(handle, symbol) : nullptr; }
bool Hook(void* target, void* replacement, void** original) {
    if (!target || !replacement) return false;
    aml->Hook(target, replacement, original);
    return original && *original;
}
}  // namespace Aml
