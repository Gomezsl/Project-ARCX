#!/usr/bin/env bash
# Descarga dependencias de terceros en third_party/ (ImGui, AML headers, stb_image).
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p third_party && cd third_party
[ -d imgui ] || git clone --depth 1 --branch v1.90.9 https://github.com/ocornut/imgui.git imgui
[ -d AndroidModLoader ] || git clone --depth 1 https://github.com/RusJJ/AndroidModLoader.git AndroidModLoader
[ -d stb ] || git clone --depth 1 https://github.com/nothings/stb.git stb
echo "[ARCX] third_party listo"
