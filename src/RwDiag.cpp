#include "RwDiag.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>

namespace {
struct Hdr { uint32_t type, size, ver; };

bool ReadHdr(const uint8_t* d, size_t len, size_t pos, Hdr& h) {
    if (pos + 12 > len) return false;
    memcpy(&h, d + pos, 12);
    return pos + 12 + (size_t)h.size <= len;
}

std::string CStr(const uint8_t* p, size_t n) {
    size_t l = 0;
    while (l < n && p[l]) ++l;
    return std::string((const char*)p, l);
}

void WalkDff(const uint8_t* d, size_t len, size_t pos, size_t end, int depth, std::set<std::string>& out) {
    if (depth > 12) return;
    while (pos + 12 <= end) {
        Hdr h;
        if (!ReadHdr(d, len, pos, h)) return;
        size_t body = pos + 12, bend = body + h.size;
        if (bend > end) return;
        switch (h.type) {
            case 0x10: case 0x1A: case 0x0F: case 0x08: case 0x07:   // clump, geometry list, geometry, material list, material
                WalkDff(d, len, body, bend, depth + 1, out);
                break;
            case 0x06: {                                              // texture: struct + string(nombre) + string(mascara) + ext
                size_t q = body;
                while (q + 12 <= bend) {
                    Hdr c;
                    if (!ReadHdr(d, len, q, c)) break;
                    if (c.type == 0x02) { std::string n = CStr(d + q + 12, c.size); if (!n.empty()) out.insert(n); break; }  // primer string = nombre
                    q += 12 + c.size;
                }
                break;
            }
            default: break;
        }
        pos = bend;
    }
}
bool FindPedChunk(const uint8_t* d, size_t len, size_t pos, size_t end, int depth) {
    if (depth > 14) return false;
    while (pos + 12 <= end) {
        Hdr h;
        if (!ReadHdr(d, len, pos, h)) return false;
        size_t body = pos + 12, bend = body + h.size;
        if (bend > end) return false;
        if (h.type == 0x116 || h.type == 0x11E) return true;
        switch (h.type) {   // contenedores: clump, geometry list, geometry, material list, material, frame list, extension, atomic
            case 0x10: case 0x1A: case 0x0F: case 0x08: case 0x07: case 0x0E: case 0x03: case 0x14:
                if (FindPedChunk(d, len, body, bend, depth + 1)) return true;
                break;
            default: break;
        }
        pos = bend;
    }
    return false;
}
}  // namespace

namespace RwDiag {
bool DffLooksLikePed(const std::vector<uint8_t>& dff) { return FindPedChunk(dff.data(), dff.size(), 0, dff.size(), 0); }

bool FileLooksLikePed(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12 || sz > (64l << 20)) { fclose(f); return false; }
    std::vector<uint8_t> d((size_t)sz);
    size_t n = fread(d.data(), 1, d.size(), f);
    fclose(f);
    return n == d.size() && DffLooksLikePed(d);
}

std::string Lower(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }

std::vector<std::string> DffTextureNames(const std::vector<uint8_t>& dff) {
    std::set<std::string> raw, low;
    WalkDff(dff.data(), dff.size(), 0, dff.size(), 0, raw);
    for (auto& n : raw) low.insert(Lower(n));
    return std::vector<std::string>(low.begin(), low.end());
}

std::vector<std::string> TxdTextureNames(const std::string& path) {
    std::vector<std::string> names;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return names;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12 || sz > (64l << 20)) { fclose(f); return names; }
    std::vector<uint8_t> d((size_t)sz);
    size_t n = fread(d.data(), 1, d.size(), f);
    fclose(f);
    if (n != d.size()) return names;
    Hdr top;
    if (!ReadHdr(d.data(), d.size(), 0, top) || top.type != 0x16) return names;
    size_t pos = 12, end = 12 + top.size;
    while (pos + 12 <= end) {
        Hdr h;
        if (!ReadHdr(d.data(), d.size(), pos, h)) break;
        if (h.type == 0x15) {   // TextureNative: struct { u32 plataforma; u32 filtro/direccionado; char name[32]; char mask[32]; ... }
            Hdr s;
            if (ReadHdr(d.data(), d.size(), pos + 12, s) && s.type == 0x01 && s.size >= 8 + 32) {
                std::string nm = CStr(d.data() + pos + 12 + 12 + 8, 32);
                bool printable = !nm.empty() && std::all_of(nm.begin(), nm.end(), [](unsigned char c) { return c >= 32 && c < 127; });
                if (printable) names.push_back(Lower(nm));
            }
        }
        pos += 12 + h.size;
    }
    return names;
}
}  // namespace RwDiag
