// Scrcpy Control Center — Main Entry Point
// Win32 + DirectX 9 + Dear ImGui
// Target: Windows 8.1 32-bit (x86)

#include "app.h"
#include "utils/logger.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // Initialize application subsystems
    if (!App::instance().init(hInstance)) {
        MessageBoxA(nullptr, "Failed to initialize application.\n"
                    "Check logs for details.", "Scrcpy Control Center",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    // Run the main loop
    int result = App::instance().run();

    // Cleanup
    App::instance().shutdown();

    return result;
}
