cmake_minimum_required(VERSION 3.20)
project(RealsenseBodyPose LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")
endif()

# ============================================
# Find Required Packages
# ============================================

# 1. RealSense SDK 2.0
find_package(realsense2 REQUIRED)
message(STATUS "Found RealSense SDK: ${realsense2_VERSION}")

# 2. OpenCV with CUDA support
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV: ${OpenCV_VERSION}")

# Check if OpenCV has CUDA support (optional but recommended)
if(OpenCV_CUDA_VERSION)
    message(STATUS "✅ OpenCV CUDA support: ${OpenCV_CUDA_VERSION}")
else()
    message(WARNING "⚠️  OpenCV built without CUDA support")
endif()

# 3. CUDA Toolkit
find_package(CUDA REQUIRED)
message(STATUS "Found CUDA: ${CUDA_VERSION}")

# 4. TensorRT
# TensorRT doesn't provide CMake config, so we find it manually
set(TENSORRT_DIR $ENV{TENSORRT_DIR} CACHE PATH "TensorRT installation directory")

if(NOT TENSORRT_DIR)
    message(FATAL_ERROR "TENSORRT_DIR environment variable not set. Please set it to TensorRT installation path (e.g., C:/TensorRT-8.6.1.6)")
endif()

find_path(TENSORRT_INCLUDE_DIR NvInfer.h
    HINTS ${TENSORRT_DIR}
    PATH_SUFFIXES include
    REQUIRED
)

find_library(TENSORRT_LIBRARY nvinfer
    HINTS ${TENSORRT_DIR}
    PATH_SUFFIXES lib
    REQUIRED
)

find_library(TENSORRT_ONNX_PARSER nvonnxparser
    HINTS ${TENSORRT_DIR}
    PATH_SUFFIXES lib
    REQUIRED
)

message(STATUS "Found TensorRT:")
message(STATUS "  Include: ${TENSORRT_INCLUDE_DIR}")
message(STATUS "  Library: ${TENSORRT_LIBRARY}")
message(STATUS "  ONNX Parser: ${TENSORRT_ONNX_PARSER}")

# ============================================
# Include Directories
# ============================================

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${realsense2_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${TENSORRT_INCLUDE_DIR}
)

# ============================================
# Source Files
# ============================================

set(SOURCES
    src/main.cpp
    src/RealSenseCamera.cpp
    src/PoseEstimator.cpp
    src/SkeletonProjector.cpp
    src/Visualizer.cpp
)

set(HEADERS
    src/Utils.h
    src/RealSenseCamera.h
    src/PoseEstimator.h
    src/SkeletonProjector.h
    src/Visualizer.h
)

# ============================================
# Executable Target
# ============================================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ============================================
# Link Libraries
# ============================================

target_link_libraries(${PROJECT_NAME}
    ${realsense2_LIBRARY}
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${CUDA_CUDA_LIBRARY}
    ${CUDA_cudart_static_LIBRARY}
    ${TENSORRT_LIBRARY}
    ${TENSORRT_ONNX_PARSER}
)

# ============================================
# Windows-Specific Configuration
# ============================================

if(WIN32)
    # Copy DLLs to output directory for runtime
    # RealSense DLLs
    if(EXISTS "${realsense2_DIR}/../../../bin/realsense2.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${realsense2_DIR}/../../../bin/realsense2.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()
    
    # TensorRT DLLs
    file(GLOB TENSORRT_DLLS "${TENSORRT_DIR}/bin/*.dll")
    foreach(DLL ${TENSORRT_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
    
    # OpenCV DLLs (if needed)
    get_target_property(OPENCV_DLL_DIR opencv_core LOCATION)
    get_filename_component(OPENCV_BIN_DIR ${OPENCV_DLL_DIR} DIRECTORY)
    file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
    foreach(DLL ${OPENCV_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
endif()

# ============================================
# Installation (Optional)
# ============================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# ============================================
# Summary
# ============================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "  RealSense 3D Skeletal Tracking")
message(STATUS "==============================================")
message(STATUS "Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  CUDA Standard: ${CMAKE_CUDA_STANDARD}")
message(STATUS "  RealSense: ${realsense2_VERSION}")
message(STATUS "  OpenCV: ${OpenCV_VERSION}")
message(STATUS "  CUDA: ${CUDA_VERSION}")
message(STATUS "  TensorRT: ${TENSORRT_DIR}")
message(STATUS "==============================================")
message(STATUS "")
