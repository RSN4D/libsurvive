# Windows Build Guide for libsurvive

## Prerequisites

### 1. Install Build Tools
- [x] CMake 3.12+ (you have 3.31.0 ✓)
- [x] Git (you have it ✓)
- [x] Python 3.8+ (you have 3.12.2 ✓)
- [x] Visual Studio 2019/2022 with C++ Desktop Development workload
- [x] NuGet (you have it ✓)

### 2. Install Python Dependencies

Open PowerShell or Command Prompt and run:

```powershell
# Required for code generation
pip install symengine sympy

# Optional but recommended for plotting/visualization
pip install numpy scipy matplotlib
```

### 3. Clean Previous Build (if needed)

If you have build errors, clean the build directory:

```powershell
# Remove the build directory to clear CMake cache
Remove-Item -Recurse -Force build-win
```

## Building libsurvive

### Quick Build (Recommended)

```powershell
# Run the build script
./make.ps1
```

This will:
1. Create a `build-win` directory
2. Download and configure Eigen automatically
3. Configure CMake with `-DDOWNLOAD_EIGEN=On -DUSE_EIGEN=On`
4. Build in Release mode

### Manual Build

If you prefer manual control:

```powershell
# Create build directory
mkdir build-win -ErrorAction SilentlyContinue
cd build-win

# Configure
cmake -DDOWNLOAD_EIGEN=On -DUSE_EIGEN=On ..

# Build
cmake --build . --config Release

# Go back to root
cd ..
```

### Troubleshooting

#### Issue: "file failed to open for reading" (Eigen error)

**Problem**: CMake cache has stale Eigen path from previous build attempt

**Solution**:
```powershell
Remove-Item build-win\CMakeCache.txt
cd build-win
cmake -DDOWNLOAD_EIGEN=On -DUSE_EIGEN=On ..
cmake --build . --config Release
```

#### Issue: "No module named 'symengine'"

**Problem**: Python dependencies not installed

**Solution**:
```powershell
pip install symengine sympy
```

#### Issue: NuGet restore fails

**Problem**: NuGet can't restore packages (libusb, lapacke, etc.)

**Solution**:
```powershell
# Manually restore NuGet packages
cd build-win\src
nuget restore packages.config -PackagesDirectory ..\packages
cd ..\..
```

#### Issue: Could not find sciplot

**Problem**: Optional dependency not installed

**Solution**: This is safe to ignore. Sciplot is optional and only used for plotting during development. The build will succeed without it.

## Running the Software

After successful build, executables are in `build-win\Release\`:

```powershell
# Run the main CLI tool
.\build-win\Release\survive-cli.exe

# With visualization (requires websocketd in PATH)
.\build-win\Release\survive-websocketd.ps1
# Then open tools/viz/index.html in a browser
```

## Common Build Options

Edit the cmake command to add these options:

```powershell
cmake -DDOWNLOAD_EIGEN=On `
      -DUSE_EIGEN=On `
      -DENABLE_TESTS=On `
      -DBUILD_STATIC=Off `
      -DCMAKE_BUILD_TYPE=Release `
      ..
```

Available options:
- `-DBUILD_STATIC=ON` - Build static library instead of DLL
- `-DENABLE_TESTS=ON` - Build test suite
- `-DBUILD_APPLICATIONS=OFF` - Skip building CLI tools
- `-DUSE_SINGLE_PRECISION=ON` - Use float instead of double
- `-DUSE_ASAN=ON` - Enable address sanitizer (for debugging)

## NuGet Build (for creating packages)

```powershell
./make.ps1 --nuget
```

This builds both x86 and x64 versions and creates NuGet packages for distribution.

## Verifying the Build

Check that these files exist after successful build:

```powershell
# Core library
build-win\Release\libsurvive.dll

# Executables
build-win\Release\survive-cli.exe
build-win\Release\api_example.exe
build-win\Release\sensors-readout.exe

# Generated code
build-win\src\generated\*.gen.h
```

## Dependencies Installed by NuGet

The build automatically fetches via NuGet:
- libusb 1.0.21
- OpenBLAS (for linear algebra)
- LAPACKE (linear algebra package)

These are downloaded to `build-win\packages\` automatically during configuration.
