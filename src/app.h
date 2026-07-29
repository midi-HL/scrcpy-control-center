#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>

// Main application class. Orchestrates initialization, the render loop,
// and shutdown of all subsystems.
class App {
public:
    static App& instance();

    // Initialize all subsystems (call once at startup)
    bool init(HINSTANCE hInstance);

    // Run the main message/render loop
    int run();

    // Shutdown and cleanup
    void shutdown();

    // Theme management
    void applyDarkTheme();
    void applyLightTheme();

    // Window access
    HWND getHwnd() const { return m_hwnd; }

private:
    App() = default;
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool initWindow(HINSTANCE hInstance);
    bool initDirectX();
    bool initImGui();
    bool resetDevice();

    HWND m_hwnd = nullptr;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    bool m_running = true;
    volatile bool m_needsResize = false;

    // DirectX 9 objects
    LPDIRECT3D9 m_d3d = nullptr;
    LPDIRECT3DDEVICE9 m_d3dDevice = nullptr;
    D3DPRESENT_PARAMETERS m_d3dpp = {};

    // Forward declare WndProc
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
