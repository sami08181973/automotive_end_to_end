#Requires -Version 5.1
<#
.SYNOPSIS
  Install prerequisites and build/run all SDV demos (Windows).
.NOTES
  Author: Muhammad Samiullah (Founder & CTO)
  Company: Embedded AI Design Labs Pvt Ltd
  Email: muhammadsami@embedailabs.com
  Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
#>
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

Write-Host "=== SDV Install & Run All ===" -ForegroundColor Cyan
Write-Host "Root: $Root"

function Test-Cmd($name) {
  return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

Write-Host "`n[1/6] Checking tools..."
$need = @()
if (-not (Test-Cmd cmake)) { $need += "cmake" }
if (-not (Test-Cmd g++) -and -not (Test-Cmd cl)) { $need += "g++ (LLVM-MinGW or MSVC)" }
if (-not (Test-Cmd docker)) { Write-Host "  WARN: docker not found – skip container demos" -ForegroundColor Yellow }
if (-not (Test-Cmd kubectl)) { Write-Host "  WARN: kubectl not found – skip k8s apply" -ForegroundColor Yellow }
if (-not (Test-Cmd terraform)) { Write-Host "  WARN: terraform not found – skip cloud IaC" -ForegroundColor Yellow }

if ($need.Count -gt 0) {
  Write-Host "Missing: $($need -join ', ')" -ForegroundColor Red
  Write-Host "Install via winget:"
  Write-Host "  winget install Kitware.CMake"
  Write-Host "  winget install MartinStorsjo.LLVM-MinGW.UCRT"
  Write-Host "  winget install Docker.DockerDesktop"
  Write-Host "  winget install Hashicorp.Terraform"
  exit 1
}
Write-Host "  cmake/g++ OK"

Write-Host "`n[2/6] Configure CMake..."
New-Item -ItemType Directory -Force -Path build | Out-Null
Push-Location build
if (-not (Test-Path CMakeCache.txt)) {
  cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
}
Write-Host "`n[3/6] Build sdv_demo + sdv_usecases..."
cmake --build . -j 8
if ($LASTEXITCODE -ne 0) { Pop-Location; exit 1 }

Write-Host "`n[4/6] Run sdv_demo (full vehicle + Virtual ECU)..."
.\sdv_demo.exe
if ($LASTEXITCODE -ne 0) { Pop-Location; exit 1 }

Write-Host "`n[5/6] Run sdv_usecases (all HW backends)..."
.\sdv_usecases.exe
if ($LASTEXITCODE -ne 0) { Pop-Location; exit 1 }

Write-Host "`n[5b] Run sdv_hpc (realtime CUDA/CPU suite)..."
.\sdv_hpc.exe
if ($LASTEXITCODE -ne 0) { Pop-Location; exit 1 }
Pop-Location

Write-Host "`n[6/6] Optional Docker build..."
if (Test-Cmd docker) {
  try {
    docker version 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) {
      Write-Host "Building image embedailabs/sdv-virtual-ecu:latest ..."
      docker build -f cloud/docker/Dockerfile -t embedailabs/sdv-virtual-ecu:latest .
      Write-Host "Run: docker run --rm embedailabs/sdv-virtual-ecu:latest sdv_usecases"
    }
  } catch {
    Write-Host "Docker daemon not running – start Docker Desktop, then re-run." -ForegroundColor Yellow
  }
}

Write-Host "`nDone. Open docs/22-cloud-hw-guide.html for cloud step-by-step." -ForegroundColor Green
Write-Host "Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd"
