#pragma once
// Unica capa que toca la API de AML. Si cambia entre versiones, se corrige solo aqui.
#include <cstdint>
#include <cstddef>

namespace Aml {
void* GetLibHandle(const char* name);             // aml->GetLibHandle
uintptr_t GetLibBase(const char* name);           // aml->GetLib
void* GetSym(void* handle, const char* symbol);   // aml->GetSym
bool Hook(void* target, void* replacement, void** original);  // aml->Hook (inline hook)
}
