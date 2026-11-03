#include "SkinLoader.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <filesystem>
#include "FileScanner.h"
#include "Game.h"
#include "Log.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"

namespace fs = std::filesystem;

namespace {
constexpr uint64_t kMaxImageBytes = 16ull << 20;
constexpr int kMaxImageDim = 2048;

struct TxdEntry { RwTexDictionary* dict; int refs; };
std::map<std::string, TxdEntry> g_txdCache;  // evita duplicar TXD identicos

bool ReadFile(const std::string& path, uint64_t maxBytes, std::vector<uint8_t>& out) {
    std::error_code ec;
    uint64_t sz = fs::file_size(path, ec);
    if (ec || sz == 0 || sz > maxBytes) return false;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.resize((size_t)sz);
    size_t n = fread(out.data(), 1, out.size(), f);
    fclose(f);
    return n == out.size();
}

bool IsPow2(int v) { return v > 0 && (v & (v - 1)) == 0; }

RwTexDictionary* LoadTxdFromFile(const RwApi& rw, const std::string& path) {
    std::vector<uint8_t> buf;
    if (!FileScanner::IsValidRwFile(path, FileScanner::kChunkTexDict) ||
        !ReadFile(path, FileScanner::kMaxFileBytes, buf)) {
        Log::Error("Invalid TXD: %s", path.c_str());
        return nullptr;
    }
    RwMemory mem{buf.data(), (uint32_t)buf.size()};
    RwStream* s = rw.RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);
    if (!s) { Log::Error("Failed to open memory stream for TXD"); return nullptr; }
    RwTexDictionary* d = nullptr;
    if (rw.RwStreamFindChunk(s, FileScanner::kChunkTexDict, nullptr, nullptr)) d = rw.RwTexDictionaryStreamRead(s);
    rw.RwStreamClose(s, &mem);
    if (!d) Log::Error("Invalid TXD (stream read failed): %s", path.c_str());
    return d;
}

// Convierte png/jpg a RwTexture y la anade al diccionario. EXPERIMENTAL en movil: el motor puede esperar
// formatos de raster concretos; requiere dimensiones potencia de 2 (no se reescala).
bool AddImageTexture(const RwApi& rw, RwTexDictionary* dict, const std::string& path) {
    std::vector<uint8_t> buf;
    if (!ReadFile(path, kMaxImageBytes, buf)) { Log::Error("Image unreadable: %s", path.c_str()); return false; }
    int w = 0, h = 0, n = 0;
    uint8_t* px = stbi_load_from_memory(buf.data(), (int)buf.size(), &w, &h, &n, 4);
    if (!px) { Log::Error("Invalid image: %s", path.c_str()); return false; }
    bool ok = false;
    if (w > kMaxImageDim || h > kMaxImageDim || !IsPow2(w) || !IsPow2(h)) {
        Log::Error("Image must be power-of-two and <= %d: %s (%dx%d)", kMaxImageDim, path.c_str(), w, h);
    } else if (RwImage* img = rw.RwImageCreate(w, h, 32)) {
        if (rw.RwImageAllocatePixels(img) && img->cpPixels && img->stride >= w * 4) {
            for (int y = 0; y < h; ++y) memcpy(img->cpPixels + (size_t)y * img->stride, px + (size_t)y * w * 4, (size_t)w * 4);
            if (RwRaster* r = rw.RwRasterCreate(w, h, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888)) {
                if (rw.RwRasterSetFromImage(r, img)) {
                    if (RwTexture* t = rw.RwTextureCreate(r)) {
                        std::string stem = fs::path(path).stem().string().substr(0, 31);
                        rw.RwTextureSetName(t, stem.c_str());
                        ok = rw.RwTexDictionaryAddTexture(dict, t) != nullptr;
                    }
                }
            }
        }
        rw.RwImageDestroy(img);
    }
    stbi_image_free(px);
    if (!ok) Log::Error("Failed to convert image to texture: %s", path.c_str());
    return ok;
}
}  // namespace

namespace SkinLoader {

size_t AvailableMemoryMB() {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[256];
    size_t kb = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemAvailable: %zu kB", &kb) == 1) break;
    }
    fclose(f);
    return kb / 1024;
}

bool Load(const SkinInfo& info, LoadedSkin& out, int maxResidentMB) {
    const RwApi& rw = Game::Rw();
    if (!rw.CanLoadModel()) { Log::Error("Game function unavailable: RenderWare stream API"); return false; }

    uint64_t mb = info.totalBytes >> 20;
    if ((int)mb > maxResidentMB) { Log::Error("Skin too large (%llu MB > %d MB): %s", (unsigned long long)mb, maxResidentMB, info.name.c_str()); return false; }
    size_t avail = AvailableMemoryMB();
    if (avail && avail < mb * 4 + 128) { Log::Error("Not enough free memory (%zu MB available)", avail); return false; }

    Log::Info("Loading %s...", info.name.c_str());
    std::vector<uint8_t> dff;
    if (!FileScanner::IsValidRwFile(info.dffPath, FileScanner::kChunkClump) ||
        !ReadFile(info.dffPath, FileScanner::kMaxFileBytes, dff)) {
        Log::Error("DFF not found or invalid: %s", info.dffPath.c_str());
        return false;
    }

    LoadedSkin ls;
    ls.name = info.name;
    bool hasImages = !info.imagePaths.empty() && rw.CanLoadImages();
    if (!info.imagePaths.empty() && !rw.CanLoadImages())
        Log::Error("Game function unavailable: image->texture conversion; imagenes ignoradas");

    // TXD (compartido si no se mezclan imagenes)
    if (!info.txdPath.empty() && !hasImages) {
        auto it = g_txdCache.find(info.txdPath);
        if (it != g_txdCache.end()) { it->second.refs++; ls.txd = it->second.dict; ls.txdKey = info.txdPath; Log::Info("Reusing TXD %s", info.txdPath.c_str()); }
        else if ((ls.txd = LoadTxdFromFile(rw, info.txdPath))) { g_txdCache[info.txdPath] = {ls.txd, 1}; ls.txdKey = info.txdPath; }
        else return false;
    } else if (!info.txdPath.empty() || hasImages) {
        ls.txd = !info.txdPath.empty() ? LoadTxdFromFile(rw, info.txdPath) : rw.RwTexDictionaryCreate();
        if (!ls.txd) return false;
        if (hasImages) for (auto& p : info.imagePaths) AddImageTexture(rw, ls.txd, p);
    }

    // DFF con el TXD como diccionario actual para que los materiales enlacen sus texturas.
    RwTexDictionary* prev = rw.RwTexDictionaryGetCurrent();
    if (ls.txd) rw.RwTexDictionarySetCurrent(ls.txd);
    RwMemory mem{dff.data(), (uint32_t)dff.size()};
    RwStream* s = rw.RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);
    if (s) {
        if (rw.RwStreamFindChunk(s, FileScanner::kChunkClump, nullptr, nullptr)) ls.clump = rw.RpClumpStreamRead(s);
        rw.RwStreamClose(s, &mem);
    }
    rw.RwTexDictionarySetCurrent(prev);

    if (!ls.clump) {
        Log::Error("Failed to load model: %s", info.dffPath.c_str());
        Release(ls);
        return false;
    }
    out = ls;
    Log::Info("%s loaded successfully", info.name.c_str());
    return true;
}

void Release(LoadedSkin& skin) {
    const RwApi& rw = Game::Rw();
    if (skin.clump && !skin.clumpOwnedByGame && rw.RpClumpDestroy) rw.RpClumpDestroy(skin.clump);
    skin.clump = nullptr;
    if (skin.txd) {
        if (!skin.txdKey.empty()) {
            auto it = g_txdCache.find(skin.txdKey);
            if (it != g_txdCache.end() && --it->second.refs <= 0) {
                if (rw.RwTexDictionaryDestroy) rw.RwTexDictionaryDestroy(it->second.dict);
                g_txdCache.erase(it);
            }
        } else if (rw.RwTexDictionaryDestroy) {
            rw.RwTexDictionaryDestroy(skin.txd);
        }
        skin.txd = nullptr;
    }
}
}  // namespace SkinLoader
