#include "FileScanner.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "Log.h"
#include "RwDiag.h"

namespace fs = std::filesystem;

namespace {
// Extensible: agrega aqui nuevos formatos de imagen (el decodificador vive en SkinLoader).
const char* const kImageExts[] = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};

std::string Lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
bool IsImageExt(const std::string& ext) {
    for (const char* e : kImageExts) if (ext == e) return true;
    return false;
}
// Prioridad: stem == nombre de carpeta, luego "skin", luego primero alfabetico.
std::string PickPreferred(std::vector<std::string>& files, const std::string& folderName) {
    if (files.empty()) return "";
    std::sort(files.begin(), files.end());
    std::string fl = Lower(folderName);
    for (auto& f : files) if (Lower(fs::path(f).stem().string()) == fl) return f;
    for (auto& f : files) if (Lower(fs::path(f).stem().string()) == "skin") return f;
    return files.front();
}
}  // namespace

namespace FileScanner {

bool IsValidRwFile(const std::string& path, uint32_t expectedChunk, std::string* err) {
    auto fail = [&](const char* m) { if (err) *err = m; return false; };
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) return fail("no es un archivo regular");
    uint64_t sz = fs::file_size(path, ec);
    if (ec) return fail("no se pudo leer el tamano");
    if (sz < 12 || sz > kMaxFileBytes) return fail("tamano fuera de rango");
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return fail("archivo no legible");
    uint32_t hdr[3] = {0, 0, 0};
    size_t n = fread(hdr, 1, sizeof(hdr), f);
    fclose(f);
    if (n != sizeof(hdr)) return fail("cabecera incompleta");
    if (hdr[0] != expectedChunk) return fail("tipo de chunk RenderWare inesperado");
    if ((uint64_t)hdr[1] + 12 > sz) return fail("archivo truncado");
    return true;
}

std::vector<SkinInfo> ScanSkins(const std::string& root, bool pedFilter) {
    std::vector<SkinInfo> out;
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        Log::Error("Skins folder not found: %s", root.c_str());
        return out;
    }
    constexpr int kMaxDepth = 3;
    struct Item { std::string path; int depth; };
    for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        auto st = it->symlink_status(e2);
        if (e2 || fs::is_symlink(st) || !fs::is_directory(st)) continue;
        std::string name = it->path().filename().string();
        if (name.empty() || name[0] == '.') continue;

        std::vector<Item> dffs, txds, imgs;
        std::vector<std::string> others;
        for (fs::recursive_directory_iterator fi(it->path(), fs::directory_options::skip_permission_denied, e2), fe;
             !e2 && fi != fe; fi.increment(e2)) {
            std::error_code e3;
            auto fst = fi->symlink_status(e3);
            if (e3 || fs::is_symlink(fst)) { fi.disable_recursion_pending(); continue; }
            if (fs::is_directory(fst)) { if (fi.depth() + 1 >= kMaxDepth) fi.disable_recursion_pending(); continue; }
            if (!fs::is_regular_file(fst)) continue;
            std::string ext = Lower(fi->path().extension().string());
            std::string p = fi->path().string();
            int depth = fi.depth();
            if (ext == ".dff") dffs.push_back({p, depth});
            else if (ext == ".txd") txds.push_back({p, depth});
            else if (IsImageExt(ext)) imgs.push_back({p, depth});
            else if (others.size() < 60) others.push_back(fs::relative(fi->path(), it->path(), e3).string());
        }

        // Solo DFF/TXD con cabecera valida participan; los DFF sin datos de ped (vehiculos/objetos) se descartan si pedFilter.
        std::vector<Item> vdff;
        for (auto& d : dffs) {
            std::string err;
            if (!IsValidRwFile(d.path, kChunkClump, &err)) { Log::Error("Invalid DFF (%s): %s", err.c_str(), d.path.c_str()); continue; }
            if (pedFilter && !RwDiag::FileLooksLikePed(d.path)) {
                Log::Info("Ignoring non-ped model (no skin/HAnim data): %s", d.path.c_str());
                continue;
            }
            vdff.push_back(d);
        }
        if (vdff.empty()) continue;  // carpeta sin modelo valido -> ignorada

        int minDepth = 99;
        for (auto& d : vdff) minDepth = std::min(minDepth, d.depth);
        std::vector<std::string> dffPaths;
        for (auto& d : vdff) if (d.depth == minDepth) dffPaths.push_back(d.path);

        std::vector<std::string> txdPaths;
        for (auto& t : txds) {
            std::string err;
            if (IsValidRwFile(t.path, kChunkTexDict, &err)) txdPaths.push_back(t.path);
            else Log::Error("Invalid TXD (%s): %s", err.c_str(), t.path.c_str());
        }

        SkinInfo s;
        s.name = name;
        s.folder = it->path().string();
        s.dffPath = PickPreferred(dffPaths, name);
        s.modelName = fs::path(s.dffPath).stem().string();
        if (dffPaths.size() > 1) Log::Info("Folder %s has %zu DFFs, using %s", name.c_str(), dffPaths.size(), s.dffPath.c_str());
        s.txdPath = PickPreferred(txdPaths, name);
        for (auto& i : imgs) s.imagePaths.push_back(i.path);
        std::sort(s.imagePaths.begin(), s.imagePaths.end());
        s.otherFiles = std::move(others);
        std::error_code e4;
        uint64_t total = fs::file_size(s.dffPath, e4);
        if (!s.txdPath.empty()) total += fs::file_size(s.txdPath, e4);
        for (auto& i : s.imagePaths) total += fs::file_size(i, e4);
        s.totalBytes = total;
        out.push_back(std::move(s));
    }
    std::sort(out.begin(), out.end(), [](const SkinInfo& a, const SkinInfo& b) { return Lower(a.name) < Lower(b.name); });
    return out;
}
}  // namespace FileScanner
