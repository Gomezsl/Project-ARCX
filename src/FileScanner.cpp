#include "FileScanner.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "Log.h"

namespace fs = std::filesystem;

namespace {
// Extensible: agrega aqui nuevos formatos de imagen (el decodificador vive en SkinLoader).
const char* const kImageExts[] = {".png", ".jpg", ".jpeg"};

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

std::vector<SkinInfo> ScanSkins(const std::string& root) {
    std::vector<SkinInfo> out;
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        Log::Error("Modloader folder not found: %s", root.c_str());
        return out;
    }
    for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        auto st = it->symlink_status(e2);
        if (e2 || fs::is_symlink(st) || !fs::is_directory(st)) continue;
        std::string name = it->path().filename().string();
        if (name.empty() || name[0] == '.') continue;

        std::vector<std::string> dffs, txds, imgs;
        uint64_t total = 0;
        for (fs::directory_iterator fi(it->path(), fs::directory_options::skip_permission_denied, e2), fe;
             !e2 && fi != fe; fi.increment(e2)) {
            std::error_code e3;
            auto fst = fi->symlink_status(e3);
            if (e3 || !fs::is_regular_file(fst)) continue;  // sin symlinks
            std::string ext = Lower(fi->path().extension().string());
            std::string p = fi->path().string();
            if (ext == ".dff") dffs.push_back(p);
            else if (ext == ".txd") txds.push_back(p);
            else if (IsImageExt(ext)) imgs.push_back(p);
        }

        // Solo DFF/TXD con cabecera valida participan en la eleccion.
        auto keepValid = [&](std::vector<std::string>& v, uint32_t chunk, const char* kind) {
            std::vector<std::string> ok;
            for (auto& p : v) {
                std::string err;
                if (IsValidRwFile(p, chunk, &err)) ok.push_back(p);
                else Log::Error("Invalid %s (%s): %s", kind, err.c_str(), p.c_str());
            }
            v.swap(ok);
        };
        keepValid(dffs, kChunkClump, "DFF");
        keepValid(txds, kChunkTexDict, "TXD");

        if (dffs.empty()) continue;  // carpeta sin modelo valido -> ignorada

        SkinInfo s;
        s.name = name;
        s.folder = it->path().string();
        s.dffPath = PickPreferred(dffs, name);
        s.modelName = fs::path(s.dffPath).stem().string();
        if (dffs.size() > 1) Log::Info("Folder %s has %zu DFFs, using %s", name.c_str(), dffs.size(), s.dffPath.c_str());
        s.txdPath = PickPreferred(txds, name);
        std::sort(imgs.begin(), imgs.end());
        s.imagePaths = imgs;
        std::error_code e4;
        total += fs::file_size(s.dffPath, e4);
        if (!s.txdPath.empty()) total += fs::file_size(s.txdPath, e4);
        for (auto& i : imgs) total += fs::file_size(i, e4);
        s.totalBytes = total;
        out.push_back(std::move(s));
    }
    std::sort(out.begin(), out.end(), [](const SkinInfo& a, const SkinInfo& b) { return Lower(a.name) < Lower(b.name); });
    return out;
}
}  // namespace FileScanner
