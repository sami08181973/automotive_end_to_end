# Virtual ECU image ? runs SDV demos without local GPU/DSP/ARM silicon.
# Free-tier friendly: push to GHCR/Docker Hub; run on Colab, GKE Autopilot,
# EKS Fargate, AKS free tier, or local Docker Desktop.
FROM debian:bookworm-slim

LABEL org.opencontainers.image.title="SDV Virtual ECU"
LABEL org.opencontainers.image.vendor="Embedded AI Design Labs Pvt Ltd"
LABEL org.opencontainers.image.authors="Muhammad Samiullah <muhammadsami@embedailabs.com>"
LABEL org.opencontainers.image.licenses="Proprietary"

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake g++ ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/sdv
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j$(nproc) \
 && cp build/sdv_demo build/sdv_usecases build/sdv_hpc /usr/local/bin/

ENV SDV_CLOUD_PROVIDER=LocalDocker
WORKDIR /work
CMD ["sdv_demo"]
