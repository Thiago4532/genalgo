cmake_minimum_required(VERSION 3.22)

project(genalgo CXX CUDA)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(SFML 2.6 COMPONENTS window system graphics REQUIRED)
# find_package(glfw3 3.3 REQUIRED)

add_subdirectory(glad/)
add_subdirectory(tsl/)

add_library(CudaGLHelper OBJECT src/CudaGLHelper.cu)
target_compile_features(CudaGLHelper PUBLIC cxx_std_20)
target_link_libraries(CudaGLHelper PRIVATE
  tsl
)

add_executable(genalgo
  src/main.cpp
  src/Shader.cpp
  src/resource.cpp
  src/GLStateManager.cpp
  src/Image.cpp
  src/globalRNG.cpp
  src/Triangle.cpp
  src/Individual.cpp
  src/Population.cpp
  src/STFitnessEngine.cpp
  src/GLFitnessEngine.cpp
  src/SFMLRenderer.cpp
  src/DrawableTexture.cpp
  src/PoorProfiler.cpp
)

target_compile_features(genalgo PRIVATE cxx_std_20)
target_include_directories(genalgo PRIVATE src/)

target_link_libraries(genalgo PRIVATE
  sfml-window
  sfml-system
  sfml-graphics
  tsl
  glad
  CudaGLHelper
  # glfw
)

# vim: et ts=8 sts=2 sw=2
