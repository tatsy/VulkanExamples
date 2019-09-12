cmake_minimum_required(VERSION 2.8.0)
project(VulkanExamples)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Macros.cmake)

# ------------------------------------------------------------------------------
# Set compiler options
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
if (NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

# ------------------------------------------------------------------------------
# Import graphics libraries
# ------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(Vulkan REQUIRED)

if (APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework IOKit -framework CoreVideo -framework IOSurface -framework Metal -framework QuartzCore")
endif()

# ------------------------------------------------------------------------------
# Set output path
# ------------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
# Set output path
# ------------------------------------------------------------------------------
add_subdirectory(sources)
