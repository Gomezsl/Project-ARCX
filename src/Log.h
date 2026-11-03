#pragma once
#include <string>
namespace Log {
void Init(const std::string& path);
void Info(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void Error(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void Shutdown();
}
