# Build Guide

Comprehensive step-by-step instructions for building the RealSense 3D Skeletal Tracking application on Windows.

> [!IMPORTANT]
> **Prerequisites First**: Complete [PREREQUISITES.md](PREREQUISITES.md) before proceeding. All dependencies must be installed.

---

## Build Process Overview

```
Prerequisites → Configure CMake → Generate VS Solution → Compile → Run
```

---

## Step 1: Verify Prerequisites

Before building, confirm all tools are accessible:

```powershell
# Open PowerShell and verify
cmake --version          # Should show 3.20+
cl                       # Should show MSVC compiler
nvcc --version          # Should show CUDA 12.3
nvidia-smi              # Should show RTX 4070

# Check environment variables
echo $env:CUDA_PATH
echo $env:TENSORRT_DIR
echo $env:OpenCV_DIR
```

---

## Step 2: Navigate to Project Directory

```powershell
cd C:\Users\basti\source\repos\RealsenseBodyPose
```

---

## Step 3: Create Build Directory

```powershell
# Create and enter build directory
mkdir build
cd build
```

> [!TIP]
> **Out-of-Source Build**: We build in a separate directory to keep source code clean. You can delete the build directory anytime without affecting source files.

---

## Step 4: Configure with CMake

### Option A: CMake GUI (Recommended for First-Time Users)

1. **Launch CMake GUI**:
   - Open "CMake (cmake-gui)" from Start Menu

2. **Set Paths**:
   - **Where is the source code**: `C:/Users/basti/source/repos/RealsenseBodyPose`
   - **Where to build the binaries**: `C:/Users/basti/source/repos/RealsenseBodyPose/build`

3. **Click "Configure"**:
   - Select generator: **Visual Studio 17 2022**
   - Select platform: **x64**
   - Click "Finish"

4. **Review Configuration**:
   - CMake will search for packages
   - Check the output log for:
     ```
     Found RealSense SDK: 2.54.x
     Found OpenCV: 4.8.x
     Found CUDA: 12.3
     Found TensorRT: ...
     ```

5. **Fix Errors (if any)**:
   - If TensorRT not found, set `TENSORRT_DIR` manually in CMake GUI
   - Click "Configure" again until no red entries remain

6. **Click "Generate"**:
   - This creates Visual Studio solution files

### Option B: Command Line (For Experienced Users)

```powershell
# From build directory
cmake .. -G "Visual Studio 17 2022" -A x64

# If TensorRT not found automatically, specify manually:
cmake .. -G "Visual Studio 17 2022" -A x64 -DTENSORRT_DIR="C:/TensorRT-8.6.1.6"
```

**Expected Output**:
```
-- Found RealSense SDK: 2.54.2
-- Found OpenCV: 4.8.1
✅ OpenCV CUDA support: 12.3
-- Found CUDA: 12.3
-- Found TensorRT:
  Include: C:/TensorRT-8.6.1.6/include
  Library: C:/TensorRT-8.6.1.6/lib/nvinfer.lib
  ONNX Parser: C:/TensorRT-8.6.1.6/lib/nvonnxparser.lib
...
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

---

## Step 5: Build the Project

### Option A: Visual Studio IDE

1. **Open Solution**:
   - Navigate to `build/RealsenseBodyPose.sln`
   - Double-click to open in Visual Studio 2022

2. **Set Build Configuration**:
   - At the top, select **Release** (not Debug)
   - Ensure platform is **x64**

3. **Build**:
   - Menu: Build → Build Solution (or press F7)
   - Wait for compilation (~2-5 minutes on first build)

4. **Check Output**:
   - Output window should show:
     ```
     ========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
     ```

### Option B: Command Line

```powershell
# From build directory
cmake --build . --config Release

# For faster compilation (use all CPU cores)
cmake --build . --config Release -j 8
```

**Expected Output**:
```
Scanning dependencies of target RealsenseBodyPose
[ 16%] Building CXX object CMakeFiles/RealsenseBodyPose.dir/src/main.cpp.obj
[ 33%] Building CXX object CMakeFiles/RealsenseBodyPose.dir/src/RealSenseCamera.cpp.obj
[ 50%] Building CXX object CMakeFiles/RealsenseBodyPose.dir/src/PoseEstimator.cpp.obj
[ 66%] Building CXX object CMakeFiles/RealsenseBodyPose.dir/src/SkeletonProjector.cpp.obj
[ 83%] Building CXX object CMakeFiles/RealsenseBodyPose.dir/src/Visualizer.cpp.obj
[100%] Linking CXX executable bin\Release\RealsenseBodyPose.exe
[100%] Built target RealsenseBodyPose
```

---

## Step 6: Verify Build Output

Check that the executable was created:

```powershell
dir build\bin\Release\RealsenseBodyPose.exe
```

You should also see required DLLs copied automatically:
- `realsense2.dll`
- `nvinfer.dll`, `nvonnxparser.dll`
- OpenCV DLLs (`opencv_world4xx.dll`, etc.)

---

## Step 7: Prepare Model

Before running, ensure you have the TensorRT engine file:

```powershell
# Check if model exists
dir models\yolov8n-pose.engine
```

If not, follow [MODEL_SETUP.md](MODEL_SETUP.md) to create it.

---

## Step 8: Test Run

```powershell
# Navigate to executable directory
cd build\bin\Release

# Connect RealSense D457 camera

# Run application
.\RealsenseBodyPose.exe --model ..\..\..\models\yolov8n-pose.engine
```

**Expected Startup**:
```
[INFO] === RealSense 3D Skeletal Tracking ===
[INFO] Starting initialization...
[INFO] 
[INFO] [1/4] Initializing RealSense Camera...
[INFO] Camera Intrinsics:
[INFO]   Resolution: 1280x720
[INFO]   Focal Length: fx=637.123 fy=637.456
[INFO] ✅ Camera started successfully:
[INFO] 
[INFO] [2/4] Initializing GPU Pose Estimator...
[TensorRT] ...
[INFO] ✅ TensorRT engine loaded successfully
[INFO] ✅ GPU Acceleration Enabled:
[INFO]   GPU: NVIDIA GeForce RTX 4070
[INFO] 
[INFO] [3/4] Initializing 3D Projector...
[INFO] ✅ 3D Projector initialized
[INFO] 
[INFO] [4/4] Initializing Visualizer...
[INFO] ✅ Visualizer initialized
[INFO] 
[INFO] ✅✅✅ All systems ready! ✅✅✅
[INFO] Press ESC to quit
```

Stand in front of the camera and verify skeleton tracking appears!

---

## Common Build Errors

### Error: "CMake could not find realsense2"

**Solution**:
```powershell
# Verify RealSense SDK is installed
dir "C:\Program Files (x86)\Intel RealSense SDK 2.0"

# Set environment variable
$env:realsense2_DIR="C:\Program Files (x86)\Intel RealSense SDK 2.0\lib\cmake\realsense2"

# Reconfigure CMake
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Error: "CMake could not find OpenCV"

**Solution**:
```powershell
# Set OpenCV_DIR to your build location
$env:OpenCV_DIR="C:\opencv_build\build\install"

# Reconfigure
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Error: "TENSORRT_DIR environment variable not set"

**Solution**:
```powershell
# Set environment variable (System-wide)
[System.Environment]::SetEnvironmentVariable("TENSORRT_DIR", "C:\TensorRT-8.6.1.6", "Machine")

# Or specify in CMake command
cmake .. -G "Visual Studio 17 2022" -A x64 -DTENSORRT_DIR="C:\TensorRT-8.6.1.6"
```

### Error: "Cannot open file 'NvInfer.h'"

**Solution**: TensorRT include directory not found
```powershell
# Verify TensorRT installation
dir C:\TensorRT-8.6.1.6\include\NvInfer.h

# Ensure TENSORRT_DIR is correct
echo $env:TENSORRT_DIR
```

### Error: Linking error "unresolved external symbol cudaFree"

**Solution**: CUDA libraries not linked
```powershell
# Verify CUDA installation
echo $env:CUDA_PATH
dir "$env:CUDA_PATH\lib\x64\cudart.lib"

# Restart computer to ensure environment variables are loaded
```

### Error: "opencv_world4xx.dll not found" at runtime

**Solution**: DLLs not in PATH
```powershell
# Option 1: Copy DLLs manually to executable directory
copy C:\opencv_build\build\install\x64\vc17\bin\*.dll build\bin\Release\

# Option 2: Add to PATH
$env:PATH += ";C:\opencv_build\build\install\x64\vc17\bin"
```

### Error: Build succeeds but "realsense2.dll not found" at runtime

**Solution**:
```powershell
# Copy RealSense DLL
copy "C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\x64\realsense2.dll" build\bin\Release\
```

---

## Clean Build

If you encounter persistent issues, try a clean build:

```powershell
# Delete build directory
cd ..
rmdir -Recurse -Force build

# Recreate and rebuild
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## Build Configuration Options

### Debug Build

For development with debugger support:
```powershell
cmake --build . --config Debug
```

The executable will be in `build\bin\Debug\RealsenseBodyPose.exe`

### Optimization Levels

Edit `CMakeLists.txt` to change optimization:
```cmake
# Maximum optimization (default)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")

# Debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
```

---

## Installation (Optional)

To install to system directory:

```powershell
# Build with install target
cmake --install . --config Release --prefix "C:\Program Files\RealsenseBodyPose"
```

---

## Next Steps

After successful build:

1. ✅ **Test with Camera**: Connect D457 and run application
2. ✅ **Verify GPU Usage**: Check Task Manager → GPU → CUDA usage while running
3. ✅ **Benchmark Performance**: Note FPS counter in visualization
4. ✅ **Try Different Models**: Build YOLOv8s-pose or YOLOv8m-pose for higher accuracy

---

## Build Summary Checklist

- [ ] All prerequisites installed
- [ ] Environment variables set (`TENSORRT_DIR`, `OpenCV_DIR`, `CUDA_PATH`)
- [ ] CMake configuration successful (all packages found)
- [ ] Build completed with no errors
- [ ] Executable created in `build\bin\Release\`
- [ ] TensorRT engine file prepared in `models/`
- [ ] RealSense D457 camera connected
- [ ] Test run successful with skeleton tracking

---

**Congratulations!** If all steps completed successfully, you now have a working real-time 3D skeletal tracking system! 🎉

For usage examples and integration, see [README.md](README.md).
