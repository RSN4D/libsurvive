# libsurvive SDK Package

This directory contains everything you need to use libsurvive in your Visual Studio C/C++ projects.

## Directory Structure

```
rsn_libsurvive/
├── bin/                    # DLLs and executables
│   ├── libsurvive.dll      # Main library DLL
│   ├── plugins/            # Plugin DLLs (drivers, posers)
│   └── *.exe               # Tools (survive-cli, api_example, etc.)
├── lib/                    # Import libraries (.lib)
│   ├── libsurvive.lib      # Main import library
│   ├── cnkalman.lib        # Kalman filter library
│   ├── cnmatrix.lib        # Matrix operations library
│   └── mpfit.lib           # Optimization library
└── include/                # Header files
    ├── libsurvive/         # Public API headers
    ├── cnkalman/           # Kalman filter headers
    ├── cnmatrix/           # Matrix library headers
    └── Eigen/              # Eigen math library headers
```

## Using in Visual Studio Projects

### Method 1: Manual Configuration

1. **Add Include Directories** (Project Properties → C/C++ → General → Additional Include Directories):
   ```
   $(SolutionDir)rsn_libsurvive\include
   ```

2. **Add Library Directory** (Project Properties → Linker → General → Additional Library Directories):
   ```
   $(SolutionDir)rsn_libsurvive\lib
   ```

3. **Link Libraries** (Project Properties → Linker → Input → Additional Dependencies):
   ```
   libsurvive.lib
   cnkalman.lib
   cnmatrix.lib
   mpfit.lib
   ```

4. **Copy DLL to Output** (Post-Build Event → Command Line):
   ```
   xcopy /Y /D "$(SolutionDir)rsn_libsurvive\bin\*.dll" "$(OutDir)"
   xcopy /Y /D /E "$(SolutionDir)rsn_libsurvive\bin\plugins" "$(OutDir)plugins\"
   ```

### Method 2: Using Property Sheet (Recommended)

Import the provided `libsurvive.props` file:

1. Right-click your project → Add → Existing Property Sheet
2. Select `rsn_libsurvive\libsurvive.props`

This automatically configures include paths, library paths, and post-build events.

## Quick Start Example

```cpp
#include <libsurvive/survive_api.h>
#include <iostream>

int main() {
    SurviveSimpleContext* ctx = survive_simple_init(0, nullptr);
    if (!ctx) {
        std::cerr << "Failed to initialize libsurvive" << std::endl;
        return 1;
    }

    std::cout << "Waiting for devices..." << std::endl;

    while (survive_simple_wait_for_update(ctx)) {
        for (const SurviveSimpleObject* it = survive_simple_get_next_updated(ctx);
             it != nullptr;
             it = survive_simple_get_next_updated(ctx)) {

            SurvivePose pose;
            survive_simple_object_get_latest_pose(it, &pose);

            std::cout << survive_simple_object_name(it) << ": "
                      << "pos(" << pose.Pos[0] << ", " << pose.Pos[1] << ", " << pose.Pos[2] << ") "
                      << "rot(" << pose.Rot[0] << ", " << pose.Rot[1] << ", "
                      << pose.Rot[2] << ", " << pose.Rot[3] << ")" << std::endl;
        }
    }

    survive_simple_close(ctx);
    return 0;
}
```

## Runtime Requirements

- **DLL Deployment**: Copy `libsurvive.dll` and the `plugins/` folder to your executable's directory
- **Windows 10/11**: x64 architecture
- **Visual C++ Runtime**: Ensure the MSVC runtime is installed (usually comes with Visual Studio)

## Available APIs

### High-Level Simple API (`survive_api.h`)
- Recommended for most applications
- Easy to use, thread-safe
- Provides position and velocity data
- See `bin/api_example.exe` source code

### Low-Level Hooks API (`survive.h`, `survive_hooks.h`)
- For advanced users
- Access to raw IMU data, light data, and events
- Requires careful threading considerations
- More control over the tracking pipeline

## Plugin System

The `plugins/` directory contains:
- **Drivers**: Hardware interface (driver_vive.dll for Vive/Lighthouse devices)
- **Posers**: Position solvers (poser_mpfit.dll - default solver)
- **Disambiguators**: Signal processing

All plugins must be in the same directory as the main DLL or in a `plugins/` subdirectory.

## Tools Included

- **survive-cli.exe** - Command-line interface for testing
- **api_example.exe** - Simple API usage example
- **sensors-readout.exe** - Display raw sensor data
- **survive-buttons.exe** - Button/controller event demo
- **survive-solver.exe** - Position solver utility

## Configuration

On first run, libsurvive will create a configuration file at:
```
%APPDATA%\libsurvive\config.json
```

This stores lighthouse calibration data. Delete this file to force recalibration.

## Troubleshooting

**DLL not found error**:
- Ensure `libsurvive.dll` is in the same directory as your .exe
- Check that the `plugins/` folder exists next to your .exe

**No devices detected**:
- Close SteamVR (it conflicts with libsurvive)
- Ensure devices are plugged in via USB
- Check Windows Device Manager for USB devices

**Build errors**:
- Make sure all paths are correct
- Verify you're building for x64 (not x86)
- Check that all .lib files are present

## Documentation

See the main repository for full documentation:
- https://github.com/cntools/libsurvive
- Header files contain detailed API documentation
- `CLAUDE.md` for development guide
- `WINDOWS_BUILD_GUIDE.md` for build instructions

## Version Info

Built with:
- CMake 3.31.0
- Visual Studio 2022 (MSVC 19.43)
- Eigen 3.4
- Python 3.12.2 (for code generation)
