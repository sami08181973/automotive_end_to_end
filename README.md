# Automotive Architectures – End-to-End Software-Defined Vehicle (SDV)

**Author:** Muhammad Samiullah (Founder & CTO)  
**Company:** Embedded AI Design Labs Pvt Ltd  
**Email:** muhammadsami@embedailabs.com  

**Copyright © 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.**  
**Watermark:** SDV End-to-End Architectures – Embedded AI Design Labs Pvt Ltd  

Public viewing of this repository is permitted. Redistribution or bulk republication
without written permission is prohibited. See [COPYRIGHT.md](COPYRIGHT.md).

---

Holistic C++ reference implementation and multi-page HTML presentation covering the key architectures of a modern Software-Defined Vehicle, including diagrams, IBM DOORS-format requirements, ASIL/ASPICE, health monitoring, CRC, CAN/SOME-IP, Adaptive AUTOSAR, hypervisor, and QNX.

## Build & Run (Windows)

```powershell
cd e:\Sami\automotive_end_to_end
mkdir build -Force
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build . -j 8
.\sdv_demo.exe
```

## Presentation

Open `docs/index.html` — includes author card, watermark, copyright, architecture pages, and:

| Page | Content |
|------|---------|
| `14-system-architecture.html` | System architecture SVG |
| `15-sequence-flows.html` | Sequence / sequential flows |
| `16-data-control-flow.html` | Data flow + control flow |
| `17-behaviour-diagrams.html` | State/behaviour diagrams |
| `18-enterprise-architecture.html` | EA-tool style layers |
| `19-doors-requirements.html` | IBM DOORS FR + NFR |
| `20-safety-aspice.html` | ASIL QM/A–D + ASPICE |
| `21-platform-tech.html` | Hypervisor, QNX, CAN, SOME/IP, CRC, health |

## HPC / CUDA realtime module

| Item | Path |
|------|------|
| Module | `modules/hpc/` (CPU optimized + `cuda/hpc_kernels.cu`) |
| Runner | `sdv_hpc` / `sdv_hpc.exe` |
| Colab | `cloud/colab/sdv_hpc_cuda.ipynb` |
| Guide | [docs/23-hpc-cuda-colab.html](docs/23-hpc-cuda-colab.html) |

```powershell
.\build\sdv_hpc.exe
# Colab / local NVIDIA:
cmake -S . -B build -DSDV_ENABLE_CUDA=ON
```

## Virtual ECU + Cloud (no local hardware)

| Path | How |
|------|-----|
| Local all demos | `powershell -File .\scripts\install_and_run_all.ps1` |
| Use-cases only | `.\build\sdv_usecases.exe` |
| Docker | `docker build -f cloud/docker/Dockerfile -t embedailabs/sdv-virtual-ecu:latest .` |
| Colab free GPU | Upload `cloud/colab/sdv_virtual_ecu.ipynb` |
| Kubernetes | `kubectl apply -f cloud/kubernetes/sdv-jobs.yaml` |
| Terraform | `cloud/terraform/{aws,gcp,azure}` |
| Step-by-step | [docs/22-cloud-hw-guide.html](docs/22-cloud-hw-guide.html) |

Optional real CUDA: `cmake -DSDV_ENABLE_CUDA=ON ...`

## Note on ASIL

ISO 26262 defines **ASIL QM, A, B, C, D** only (no ASIL-E). See `docs/20-safety-aspice.html`.
