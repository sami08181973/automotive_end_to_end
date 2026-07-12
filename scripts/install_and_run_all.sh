#!/usr/bin/env bash
# Install prerequisites and run all SDV demos (Linux / macOS / WSL / Colab)
# Author: Muhammad Samiullah – Embedded AI Design Labs Pvt Ltd
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
echo "=== SDV Install & Run All ==="
echo "Root: $ROOT"

need_apt=0
command -v cmake >/dev/null || need_apt=1
command -v g++ >/dev/null || need_apt=1
if [[ "$need_apt" -eq 1 ]]; then
  echo "Installing cmake g++ via apt (sudo)..."
  sudo apt-get update -qq
  sudo apt-get install -y -qq build-essential cmake g++
fi

mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu || echo 4)"

echo "=== sdv_demo ==="
./build/sdv_demo
echo "=== sdv_usecases ==="
./build/sdv_usecases
echo "=== sdv_hpc ==="
./build/sdv_hpc

if command -v docker >/dev/null 2>&1; then
  echo "=== docker build (optional) ==="
  docker build -f cloud/docker/Dockerfile -t embedailabs/sdv-virtual-ecu:latest . || true
fi

echo "Done. See docs/22-cloud-hw-guide.html"
echo "Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd"
