cmake_minimum_required(VERSION 3.22)

project(genalgo CXX CUDA)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES native)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(SFML 2.6 COMPONENTS window system graphics REQUIRED)
find_package(OpenMP REQUIRED)

add_library(genalgoIncludes INTERFACE)
target_compile_features(genalgoIncludes INTERFACE cxx_std_17)
target_include_directories(genalgoIncludes INTERFACE src/)

add_library(cudaFitnessEngine STATIC src/CudaFitnessEngine.cu)
target_compile_options(cudaFitnessEngine PRIVATE
  "-lineinfo"
)
target_link_libraries(cudaFitnessEngine
  PUBLIC genalgoIncludes
)

add_library(cpuFitnessEngine STATIC src/STFitnessEngine.cpp src/MTFitnessEngine.cpp)
target_link_libraries(cpuFitnessEngine
  PRIVATE OpenMP::OpenMP_CXX
  PUBLIC genalgoIncludes
)

add_executable(genalgo
  src/main.cpp
  src/Image.cpp
  src/globalRNG.cpp
  src/Triangle.cpp
  src/Individual.cpp
  src/Population.cpp
  src/FitnessEngine.cpp
  src/SFMLRenderer.cpp
  src/PoorProfiler.cpp
  src/SignalHandler.cpp
  src/JSONSerializer.cpp
  src/JSONDeserializer.cpp
  src/GlobalConfig.cpp
)

target_compile_features(genalgo PRIVATE cxx_std_20)
target_include_directories(genalgo PRIVATE src/)

target_link_libraries(genalgo
  sfml-window
  sfml-system
  sfml-graphics
  cudaFitnessEngine
  cpuFitnessEngine
  genalgoIncludes
)

# vim: et ts=8 sts=2 sw=2