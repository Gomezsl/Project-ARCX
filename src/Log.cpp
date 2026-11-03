#include "Log.h"
#include <cstdarg>
#include <cstdio>
#include <mutex>
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace {
std::mutex g_mutex;
FILE* g_file = nullptr;

void Write(bool error, const char* fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    const char* prefix = error ? "[ARCX][ERROR] " : "[ARCX] ";
    std::lock_guard<std::mutex> lk(g_mutex);
#ifdef __ANDROID__
    __android_log_print(error ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "ARCX", "%s%s", prefix, buf);
#endif
    if (g_file) {
        fprintf(g_file, "%s%s\n", prefix, buf);
        fflush(g_file);
    }
}
}  // namespace

namespace Log {
void Init(const std::string& path) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_file) fclose(g_file);
    g_file = fopen(path.c_str(), "w");  // se trunca en cada arranque
}
void Info(const char* fmt, ...) { va_list ap; va_start(ap, fmt); Write(false, fmt, ap); va_end(ap); }
void Error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); Write(true, fmt, ap); va_end(ap); }
void Shutdown() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_file) { fclose(g_file); g_file = nullptr; }
}
}  // namespace Log
