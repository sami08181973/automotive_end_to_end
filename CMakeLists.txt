cmake_minimum_required(VERSION 3.14)
project(AutomotiveSDV_EndToEnd VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SDV_ENABLE_CUDA "Compile NVIDIA CUDA HPC kernels" OFF)

if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SDV_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(SDV_MODULE_SOURCES
    modules/infotainment/infotainment.cpp
    modules/connectivity/connectivity.cpp
    modules/adas/adas.cpp
    modules/body_comfort/body_comfort.cpp
    modules/powertrain/powertrain.cpp
    modules/chassis_safety/chassis_safety.cpp
    modules/ee_architecture/ee_architecture.cpp
    modules/security/security.cpp
    modules/functional/functional.cpp
    modules/software_platform/software_platform.cpp
    modules/vehicle_data/vehicle_data.cpp
    modules/virtual_ecu/virtual_ecu.cpp
    modules/hpc/hpc.cpp
    modules/hpc/hpc_compute.cpp
    modules/hpc/hpc_kernels_cpu.cpp
)

add_library(sdv_architectures STATIC ${SDV_MODULE_SOURCES})
target_include_directories(sdv_architectures PUBLIC
    ${SDV_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/modules
)

# CUDA vs stub
set(SDV_CUDA_READY FALSE)
if(SDV_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(CMAKE_CUDA_STANDARD 17)
        target_sources(sdv_architectures PRIVATE modules/hpc/cuda/hpc_kernels.cu)
        target_compile_definitions(sdv_architectures PUBLIC SDV_HAS_CUDA=1)
        set_target_properties(sdv_architectures PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
        set(SDV_CUDA_READY TRUE)
        message(STATUS "CUDA HPC kernels ENABLED (${CMAKE_CUDA_COMPILER})")
    else()
        message(WARNING "SDV_ENABLE_CUDA=ON but nvcc not found – using CPU + stub")
    endif()
endif()

if(NOT SDV_CUDA_READY)
    target_sources(sdv_architectures PRIVATE modules/hpc/cuda/hpc_cuda_stub.cpp)
    message(STATUS "CUDA HPC kernels: stub (CPU-optimized path)")
endif()

add_executable(sdv_demo src/main.cpp)
target_link_libraries(sdv_demo PRIVATE sdv_architectures)

add_executable(sdv_usecases usecases/usecases_runner.cpp)
target_link_libraries(sdv_usecases PRIVATE sdv_architectures)

add_executable(sdv_hpc usecases/hpc_usecases_runner.cpp)
target_link_libraries(sdv_hpc PRIVATE sdv_architectures)

install(TARGETS sdv_demo sdv_usecases sdv_hpc RUNTIME DESTINATION bin)
install(DIRECTORY docs/ DESTINATION share/sdv_docs)
install(DIRECTORY cloud/ DESTINATION share/sdv_cloud)

message(STATUS "Targets: sdv_demo | sdv_usecases | sdv_hpc")
message(STATUS "Docs:    docs/23-hpc-cuda-colab.html")
