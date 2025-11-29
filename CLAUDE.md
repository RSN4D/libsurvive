# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

libsurvive is an open-source 6DoF tracking library for HTC Vive and Lighthouse-based VR systems. It supports both SteamVR 1.0 and 2.0 generation devices and provides tracking capabilities independent of SteamVR.

## Build Commands

### Linux/Unix
```bash
# Quick build using Makefile
make

# Or manually with CMake
mkdir -p bin
cd bin
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . -j 4

# Install
make install
# or: cd bin && cmake --build . --target install

# Run tests (if enabled)
cmake -DENABLE_TESTS=ON ..
cmake --build .
ctest
```

### Windows
```bash
# Quick build using PowerShell script
./make.ps1

# Or manually with CMake GUI
# Open CMakeLists.txt in CMake GUI, configure, generate, then build in Visual Studio
```

### Common CMake Options
- `-DBUILD_STATIC=ON` - Build as static library instead of shared
- `-DUSE_SINGLE_PRECISION=ON` - Use float instead of double
- `-DENABLE_TESTS=ON` - Enable build and execution of tests
- `-DBUILD_APPLICATIONS=OFF` - Don't build CLI tools (for library-only builds)
- `-DUSE_HIDAPI=ON` - Use HIDAPI instead of libusb (default on Windows)
- `-DUSE_ASAN=ON` - Enable address sanitizer
- `-DBUILD_GATT_SUPPORT=ON` - Include GATT support for basestations (requires gattlib)

### Python Bindings
```bash
# Install from PyPI
pip install pysurvive

# Build from source
python setup.py install
```

### C# Bindings
```bash
cd bindings/cs
dotnet build -c Release
```

## Testing

```bash
# Run all tests
cd bin
ctest

# Run specific test
./test_cases/test_name

# Run with data recording for debugging
./survive-cli --record debug.rec.gz
./survive-cli --playback debug.rec.gz

# USB monitoring (Linux only, requires libpcap and usbmon)
sudo modprobe usbmon
sudo setfacl -m u:$USER:r /dev/usbmon*
./survive-cli --usbmon-record capture.pcap.gz --htcvive
./survive-cli --usbmon-playback capture.pcap.gz
```

## Architecture

### Data Flow Pipeline

1. **Drivers** (`src/driver_*.c`) - Provide raw data from devices
   - `driver_vive.c` - Main USB driver for Vive hardware
   - `driver_simulator.c` - Simulates device for testing
   - `driver_playback.c` - Replays recorded data
   - `driver_usbmon.c` - Can run concurrent with SteamVR

2. **Data Processing** (`src/survive_process*.c`)
   - `survive_process_gen1.c` - Lighthouse 1.0 protocol
   - `survive_process_gen2.c` - Lighthouse 2.0 protocol
   - Disambiguates angle data (lighthouse → sensor → axis)
   - Collects OOTX calibration data from lighthouses
   - Filters stray light and reflections

3. **Posers** (`src/poser_*.c`) - Solve for position/orientation
   - `poser_mpfit.c` - MPFit solver (default, uses Levenberg-Marquardt)
   - `poser_barycentric_svd.c` - SVD-based seed poser (initial solve)
   - `poser_epnp.c` - Efficient PnP algorithm variant
   - Solve for lighthouse positions (initial calibration)
   - Provide pose estimates to Kalman filter

4. **Kalman Filter** (`src/survive_kalman_tracker.c`)
   - 21-dimensional state space per object:
     - Pose (Vec3 + Quaternion)
     - Velocity (Vec3 + Axis-angle)
     - Acceleration, Acceleration Scale
     - IMU rotational correction
   - Iterative Extended Kalman Filter (IEKF)
   - Integrates IMU data, light data, and poser output
   - Outputs final pose when covariance is low enough

5. **Output** - User-registerable callbacks
   - Low-level API: Direct hooks (see `include/libsurvive/survive_hooks.h`)
   - High-level API: Simple API (see `include/libsurvive/survive_api.h`)

### Key Components

**Math Infrastructure** (`tools/generate_math_functions/`)
- Uses symengine to generate C code from Python implementations
- Automatically generates analytical Jacobians for non-linear least squares
- Sanity-checks against numerical Jacobians
- Generated code in `src/generated/*.gen.h`

**Calibration**
- Stored in `config.json` in `XDG_CONFIG_HOME/libsurvive`
- Continuously integrates data from stationary objects
- Use `--force-calibrate` to recalibrate with existing OOTX data
- Delete `config.json` to force full recalibration

**Plugin System** (`src/survive_plugins.c`)
- Drivers compile to `driver_<name>.so` in plugins folder
- Must have `REGISTER_LINKTIME(DriverRegExample)` registration
- Can be poll-based or threaded
- Disabled with `-DBUILD_STATIC=ON` (sets `SURVIVE_DISABLE_PLUGINS`)

### Code Organization

```
src/
├── driver_*.c          # Device drivers (USB, playback, simulator, etc.)
├── poser_*.c           # Position solvers (MPFit, SVD, EPnP, etc.)
├── survive.c           # Core library initialization and management
├── survive_api.c       # High-level Simple API
├── survive_kalman_tracker.c  # Kalman filter implementation
├── survive_process*.c  # Gen1/Gen2 lighthouse data processing
├── survive_reproject*.c # Reprojection models for both generations
├── survive_config.c    # Configuration management
├── ootx_decoder.c      # OOTX lighthouse calibration data decoder
└── generated/          # Auto-generated math functions

include/libsurvive/
├── survive.h           # Core API
├── survive_api.h       # High-level Simple API
├── survive_hooks.h     # Callback hooks for raw data
├── survive_types.h     # Data structures and types
└── poser.h            # Poser interface

redist/                 # Third-party/redistributed code
├── mpfit/             # Levenberg-Marquardt optimization
├── linmath.*          # Linear algebra utilities
└── json_helpers.*     # JSON parsing (using jsmn)

tools/
├── generate_math_functions/  # Python → C code generation
└── viz/                      # THREE.js visualization (needs websocketd)

libs/
├── cnkalman/          # Kalman filter library
└── cnmatrix/          # Matrix operations library
```

## Development Patterns

### Adding a New Driver

1. Create `src/driver_<name>.c`
2. Implement registration function:
   ```c
   int DriverRegExample(SurviveContext *ctx) {
       if(...error...) return SURVIVE_DRIVER_ERROR;
       return SURVIVE_DRIVER_NORMAL;
   }
   REGISTER_LINKTIME(DriverRegExample)
   ```
3. Register poll or threaded driver:
   - Poll: `survive_add_driver(ctx, user_ptr, poll_fn, close_fn)`
   - Thread: `survive_add_threaded_driver(ctx, data, name, routine, close_fn)`
4. For threaded drivers, lock context access:
   ```c
   survive_get_ctx_lock(ctx);
   // access SurviveContext or SurviveObject members
   survive_release_ctx_lock(ctx);
   ```

### Adding a New Poser

1. Create `src/poser_<name>.c`
2. Implement handler function with "Poser" prefix:
   ```c
   int PoserMyPoser(SurviveObject *so, PoserFnData *pd) {
       switch (pd->pt) {
       case POSERDATA_LIGHT: /* handle light data */ return 0;
       case POSERDATA_IMU: /* handle IMU data */ return 0;
       case POSERDATA_FULL_SCENE: /* handle full scene */ return 0;
       }
       return -1;  // not handled
   }
   REGISTER_LINKTIME(PoserMyPoser);
   ```
3. Output poses via `PoserData_poser_raw_pose_func()`
4. Set lighthouse poses via `PoserData_lighthouse_pose_func()`
5. See `docs/writing_a_poser.md` for details

### Modifying Math Functions

1. Edit Python implementations in `src/generated/*.py`
2. Run code generation (happens automatically during build):
   ```bash
   cnkalman_generate_code(./generated/<file>.py)
   ```
3. Generated `.gen.h` files are auto-included in build
4. Sanity checks compare analytical vs numerical Jacobians

### Using Hooks (Low-Level API)

```c
// Install custom hook
light_process_func old_fn = survive_install_light_fn(ctx, my_light_fn);

// In your callback, optionally call previous handler
static void my_light_fn(SurviveObject *so, int sensor_id, ...) {
    // Your code here
    if (old_fn) old_fn(so, sensor_id, ...);  // Call previous handler
}

// Use user_ptr for state (avoid globals)
ctx->user_ptr = my_data;
so->user_ptr = my_object_data;
```

## Common Development Tasks

### Debugging Tracking Issues

```bash
# Increase verbosity
./survive-cli --v 100   # Common tracking info
./survive-cli --v 150   # Per-pose info
./survive-cli --v 250   # Per-light-event info

# Visualize
./survive-websocketd & xdg-open ./tools/viz/index.html

# Record problematic session
./survive-cli --record issue.rec.gz --v 100

# Force specific lighthouse generation
./survive-cli --lighthouse-gen 1  # or 2
```

### Working with Recordings

```bash
# Record session
./survive-cli --record session.rec.gz

# Playback
./survive-cli --playback session.rec.gz

# Playback with speed control
./survive-cli --playback session.rec.gz --playback-factor 0  # fast as possible
./survive-cli --playback session.rec.gz --playback-factor 2  # half speed
```

### Configuration Files

- Default location: `$XDG_CONFIG_HOME/libsurvive/config.json`
- Recording-specific: `<recording>.json` (e.g., `debug.rec.json`)
- Generated code uses same format as runtime config

## Coding Standards

- **C Standard**: C11 (C99 for most code, C11 for atomics)
- **C++ Standard**: C++20
- **Formatting**: Use `.clang-format` in repository root
- **Error Handling**: Return error codes, use `SV_ERROR`/`SV_WARN`/`SV_INFO` logging
- **Thread Safety**: Lock `SurviveContext` when accessing from threads
- **Warnings**: Treated as errors in CI (`-DENABLE_WARNINGS_AS_ERRORS=ON`)

## Language Bindings

**Python** (`bindings/python/`)
- Available via `pip install pysurvive`
- Uses Simple API (high-level)
- Example: `bindings/python/` directory

**C#** (`bindings/cs/`)
- NuGet package: `libsurvive.net`
- Simple API wrapper (`libsurvive.SurviveAPI`)
- Works on Linux and Windows
- Unity integration example included

## Platform-Specific Notes

**Linux**
- Requires udev rules: `sudo cp useful_files/81-vive.rules /etc/udev/rules.d/`
- Uses libusb by default
- GATT support requires gattlib

**Windows**
- Uses HIDAPI by default (not libusb)
- NuGet automatically fetches dependencies (LAPACKE, OpenBLAS)
- Release binaries available on GitHub

**Android**
- Core build only: `-DDO_CORE_BUILD=ON`
- Uses libusb

## Important Files

- `survive-cli.c` - Main CLI example
- `api_example.c` - Simple API usage example
- `sensors-readout.c` - Raw sensor data display
- `simple_pose_test.c` - Pose testing utility
- `docs/architecture.md` - Detailed architecture documentation
- `docs/writing_a_poser.md` - Poser development guide
- `survive_autocomplete.sh` - Bash completion (install to `/etc/bash_completion.d/`)

## References

- Discord: https://discordapp.com/invite/7QbCAGS
- GitHub: https://github.com/cntools/libsurvive
- PyPI: https://pypi.org/project/pysurvive/
- NuGet: https://www.nuget.org/packages/libsurvive.net/
