// Example: Using libsurvive in your Visual Studio project
// To use this example:
// 1. Add libsurvive.props to your project (Right-click project → Add → Existing Property Sheet)
// 2. Add this .cpp to your project
// 3. Build and run

#include <libsurvive/survive_api.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    printf("Initializing libsurvive...\n");

    // Initialize libsurvive context
    SurviveSimpleContext* ctx = survive_simple_init(argc, argv);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize libsurvive!\n");
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  1. Lighthouse devices are plugged in\n");
        fprintf(stderr, "  2. SteamVR is closed\n");
        fprintf(stderr, "  3. libsurvive.dll and plugins/ folder are in exe directory\n");
        return 1;
    }

    printf("libsurvive initialized successfully!\n");
    printf("Waiting for devices and tracking data...\n");
    printf("Press Ctrl+C to exit\n\n");

    // Main loop - wait for tracking updates
    int update_count = 0;
    while (survive_simple_wait_for_update(ctx)) {

        // Process all updated objects
        for (const SurviveSimpleObject* obj = survive_simple_get_next_updated(ctx);
             obj != NULL;
             obj = survive_simple_get_next_updated(ctx)) {

            // Get the latest pose for this object
            SurvivePose pose;
            uint32_t timecode = survive_simple_object_get_latest_pose(obj, &pose);

            // Get object name and serial number
            const char* name = survive_simple_object_name(obj);
            const char* serial = survive_simple_serial_number(obj);

            // Print tracking data
            printf("[%s] %s (time: %u)\n", serial, name, timecode);
            printf("  Position: x=%.3f y=%.3f z=%.3f\n",
                   pose.Pos[0], pose.Pos[1], pose.Pos[2]);
            printf("  Rotation: w=%.3f x=%.3f y=%.3f z=%.3f\n",
                   pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);

            // Get velocity if you need it
            SurviveVelocity velocity;
            survive_simple_object_get_latest_velocity(obj, &velocity);
            printf("  Velocity: x=%.3f y=%.3f z=%.3f\n",
                   velocity.Pos[0], velocity.Pos[1], velocity.Pos[2]);
            printf("\n");

            update_count++;
        }
    }

    printf("Received %d tracking updates\n", update_count);

    // Cleanup
    survive_simple_close(ctx);
    return 0;
}


// ALTERNATIVE: Using C++ style
#ifdef __cplusplus
#include <iostream>
#include <iomanip>

void cpp_example(int argc, char** argv) {
    auto ctx = survive_simple_init(argc, argv);
    if (!ctx) {
        std::cerr << "Failed to initialize libsurvive!" << std::endl;
        return;
    }

    std::cout << "libsurvive initialized (C++ example)" << std::endl;
    std::cout << std::fixed << std::setprecision(3);

    while (survive_simple_wait_for_update(ctx)) {
        for (auto obj = survive_simple_get_next_updated(ctx);
             obj != nullptr;
             obj = survive_simple_get_next_updated(ctx)) {

            SurvivePose pose;
            survive_simple_object_get_latest_pose(obj, &pose);

            std::cout << survive_simple_object_name(obj) << ": "
                      << "pos(" << pose.Pos[0] << ", "
                      << pose.Pos[1] << ", "
                      << pose.Pos[2] << ")" << std::endl;
        }
    }

    survive_simple_close(ctx);
}
#endif


/*
 * Notes:
 *
 * 1. Pose Format:
 *    - Position: (x, y, z) in meters
 *    - Rotation: Quaternion (w, x, y, z)
 *
 * 2. Common Use Cases:
 *    - VR tracking
 *    - Motion capture
 *    - Robot localization
 *    - 6DoF controller tracking
 *
 * 3. Configuration:
 *    - First run will calibrate lighthouses (takes ~10 seconds)
 *    - Calibration saved to %APPDATA%\libsurvive\config.json
 *    - Use --force-calibrate to recalibrate
 *    - Use --v 100 for verbose output
 *
 * 4. Command Line Options:
 *    Pass argv to survive_simple_init() to use options like:
 *    --v 100             - Verbose logging
 *    --force-calibrate   - Force recalibration
 *    --lighthouse-gen 2  - Force lighthouse generation
 *    --playback file.rec - Playback recorded data
 *
 * 5. Thread Safety:
 *    - Simple API is thread-safe
 *    - Your code runs in its own thread
 *    - Can't starve libsurvive of data
 */
