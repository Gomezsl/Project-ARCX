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
#define STBIDEF static  // no exportar simbolos stb desde el .so
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

// Convierte una imagen a RwTexture y la anade al diccionario.
//  - .png: decodificador del propio juego (RtPNGImageRead) -> RwImageFindRasterFormat -> raster (camino del juego).
//  - .jpg/.jpeg (o PNG si RtPNG no esta): stb_image, RGBA8888. Requiere potencia de 2 y <=2048.
// EXPERIMENTAL en movil: el formato de raster puede necesitar ajustes.
bool AddTexture(const RwApi& rw, RwTexDictionary* dict, RwImage* img, const std::string& path, bool nativeFormat) {
    if (!img) return false;
    int w = img->width, h = img->height, depth = img->depth, flags = 0;
    if (nativeFormat && rw.RwImageFindRasterFormat) {
        if (!rw.RwImageFindRasterFormat(img, rwRASTERTYPETEXTURE, &w, &h, &depth, &flags)) return false;
    } else {
        depth = 32; flags = rwRASTERTYPETEXTURE | rwRASTERFORMAT8888;
    }
    RwRaster* r = rw.RwRasterCreate(w, h, depth, (uint32_t)flags);
    if (!r || !rw.RwRasterSetFromImage(r, img)) return false;
    RwTexture* t = rw.RwTextureCreate(r);
    if (!t) return false;
    std::string stem = fs::path(path).stem().string().substr(0, 31);
    rw.RwTextureSetName(t, stem.c_str());
    return rw.RwTexDictionaryAddTexture(dict, t) != nullptr;
}

bool AddImageTexture(const RwApi& rw, RwTexDictionary* dict, const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    std::error_code ec;
    uint64_t sz = fs::file_size(path, ec);
    if (ec || sz == 0 || sz > kMaxImageBytes) { Log::Error("Image unreadable or too large: %s", path.c_str()); return false; }

    if (ext == ".png" && rw.RtPNGImageRead && rw.RwImageDestroy) {
        RwImage* img = rw.RtPNGImageRead(path.c_str());
        bool ok = false;
        if (img) { ok = AddTexture(rw, dict, img, path, true); rw.RwImageDestroy(img); }
        if (ok) return true;
        Log::Error("Game PNG reader failed, trying stb: %s", path.c_str());
    }
    if (!rw.CanLoadImages()) { Log::Error("Game function unavailable: image->texture conversion"); return false; }
    std::vector<uint8_t> buf;
    if (!ReadFile(path, kMaxImageBytes, buf)) return false;
    int w = 0, h = 0, n = 0;
    uint8_t* px = stbi_load_from_memory(buf.data(), (int)buf.size(), &w, &h, &n, 4);
    if (!px) { Log::Error("Invalid image: %s", path.c_str()); return false; }
    bool ok = false;
    if (w > kMaxImageDim || h > kMaxImageDim || !IsPow2(w) || !IsPow2(h)) {
        Log::Error("Image must be power-of-two and <= %d: %s (%dx%d)", kMaxImageDim, path.c_str(), w, h);
    } else if (RwImage* img = rw.RwImageCreate(w, h, 32)) {
        if (rw.RwImageAllocatePixels(img) && img->cpPixels && img->stride >= w * 4) {
            for (int y = 0; y < h; ++y) memcpy(img->cpPixels + (size_t)y * img->stride, px + (size_t)y * w * 4, (size_t)w * 4);
            ok = AddTexture(rw, dict, img, path, false);
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
    LoadedSkin ls;
    ls.name = info.name;
    if (!FileScanner::IsValidRwFile(info.dffPath, FileScanner::kChunkClump) ||
        !ReadFile(info.dffPath, FileScanner::kMaxFileBytes, ls.dff)) {
        Log::Error("DFF not found or invalid: %s", info.dffPath.c_str());
        return false;
    }
    Log::Info("DFF: %s (%zu bytes)", info.dffPath.c_str(), ls.dff.size());

    bool hasImages = !info.imagePaths.empty();
    // TXD (compartido si no se mezclan imagenes)
    if (!info.txdPath.empty() && !hasImages) {
        auto it = g_txdCache.find(info.txdPath);
        if (it != g_txdCache.end()) { it->second.refs++; ls.txd = it->second.dict; ls.txdKey = info.txdPath; Log::Info("Reusing TXD %s", info.txdPath.c_str()); }
        else if ((ls.txd = LoadTxdFromFile(rw, info.txdPath))) { g_txdCache[info.txdPath] = {ls.txd, 1}; ls.txdKey = info.txdPath; }
        else return false;
    } else if (!info.txdPath.empty() || hasImages) {
        ls.txd = !info.txdPath.empty() ? LoadTxdFromFile(rw, info.txdPath) : rw.RwTexDictionaryCreate();
        if (!ls.txd) return false;
        if (hasImages) {
            int ok = 0;
            for (auto& p : info.imagePaths) ok += AddImageTexture(rw, ls.txd, p) ? 1 : 0;
            Log::Info("Images converted: %d/%zu", ok, info.imagePaths.size());
        }
    }
    out = std::move(ls);
    Log::Info("%s resources ready", info.name.c_str());
    return true;
}

void Release(LoadedSkin& skin) {
    const RwApi& rw = Game::Rw();
    skin.dff.clear();
    skin.dff.shrink_to_fit();
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
