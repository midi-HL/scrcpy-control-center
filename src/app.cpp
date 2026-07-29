#include "app.h"
#include "ui/sidebar.h"
#include "ui/content_panel.h"
#include "core/feature_parser.h"
#include "core/state_manager.h"
#include "core/config_manager.h"
#include "core/device_manager.h"
#include "core/process_manager.h"
#include "core/icon_manager.h"
#include "utils/logger.h"
#include "utils/i18n.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include <d3d9.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

App& App::instance() {
    static App s_instance;
    return s_instance;
}

// ── Win32 Window Procedure ───────────────────────────────────────────────────

LRESULT WINAPI App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                auto& app = App::instance();
                app.m_windowWidth = LOWORD(lParam);
                app.m_windowHeight = HIWORD(lParam);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ── Window Creation ──────────────────────────────────────────────────────────

bool App::initWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ScrcpyControlCenter";

    RegisterClassEx(&wc);

    m_hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Scrcpy Control Center",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        m_windowWidth, m_windowHeight,
        nullptr, nullptr,
        hInstance, nullptr
    );

    if (!m_hwnd) {
        LOG_ERROR("Failed to create window");
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    return true;
}

// ── DirectX 9 Initialization ────────────────────────────────────────────────

bool App::initDirectX() {
    m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_d3d) {
        LOG_ERROR("Direct3DCreate9 failed");
        return false;
    }

    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
    m_d3dpp.Windowed = TRUE;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    m_d3dpp.EnableAutoDepthStencil = TRUE;
    m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // VSync

    HRESULT hr = m_d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        m_hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &m_d3dpp,
        &m_d3dDevice
    );

    if (FAILED(hr)) {
        // Fallback to software vertex processing
        hr = m_d3d->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            m_hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &m_d3dpp,
            &m_d3dDevice
        );
    }

    if (FAILED(hr)) {
        LOG_ERROR("DirectX 9 device creation failed: 0x%08lx", hr);
        return false;
    }

    LOG_INFO("DirectX 9 initialized successfully");
    return true;
}

// ── ImGui Initialization ────────────────────────────────────────────────────

bool App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Disable ImGui.ini file (we manage our own config)
    io.IniFilename = nullptr;

    // Setup ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;    // No rounded corners (flat design)
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    // Platform/Renderer backends
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX9_Init(m_d3dDevice);

    LOG_INFO("ImGui initialized (v%s)", ImGui::GetVersion());
    return true;
}

// ── Theme Presets ────────────────────────────────────────────────────────────

void App::applyDarkTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
}

void App::applyLightTheme() {
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
}

// ── Full Initialization ─────────────────────────────────────────────────────

bool App::init(HINSTANCE hInstance) {
    // 0. DPI awareness for multi-resolution support (Win8.1 compatible)
    typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
    auto pSetDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(
        GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
    if (pSetDPIAware) {
        pSetDPIAware();
        LOG_INFO("DPI awareness enabled");
    }

    // 1. Load feature mapping (must be first — everything depends on this)
    if (!FeatureParser::instance().loadFromFile("config/scrcpy_features.json")) {
        LOG_ERROR("Failed to load feature mapping — UI will be empty");
    }

    // 2. Initialize state from feature defaults
    StateManager::instance().initFromDefaults();

    // 3. Initialize config manager (sets default paths to app directory)
    ConfigManager::instance().init("config");

    // 4. Load saved state if exists
    StateManager::instance().loadFromJson("config/last_state.json");

    // 5. Create Win32 window
    if (!initWindow(hInstance)) return false;

    // 6. Initialize DirectX 9
    if (!initDirectX()) return false;

    // 7. Initialize ImGui
    if (!initImGui()) return false;

    // 8. Initialize icon manager
    IconManager::instance().init("cache/icons");

    // 9. Start device polling (interval from settings, default 20s)
    auto& cfg = ConfigManager::instance().getSettings();
    int pollIntervalMs = cfg.devicePollingInterval * 1000;
    DeviceManager::instance().startPolling(cfg.adbPath, pollIntervalMs);

    // 10. Wire up ProcessManager output → Logger
    ProcessManager::instance().setOutputCallback([](const std::string& line) {
        std::string trimmed = line;
        while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r')) {
            trimmed.pop_back();
        }
        if (!trimmed.empty()) {
            LOG_INFO("[scrcpy] %s", trimmed.c_str());
        }
    });

    // 11. Apply theme
    if (cfg.theme == "light") {
        applyLightTheme();
    }

    LOG_INFO("Application initialized successfully (polling: %ds, scrcpy: %s, adb: %s)",
             cfg.devicePollingInterval, cfg.scrcpyPath.c_str(), cfg.adbPath.c_str());
    return true;
}

// ── Main Loop ───────────────────────────────────────────────────────────────

int App::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (m_running) {
        // Process Windows messages
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                m_running = false;
            }
        }
        if (!m_running) break;

        // Begin ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ── Full-window ImGui layout ─────────────────────────────────────────
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("##MainWindow", nullptr, windowFlags);

        // Left sidebar
        auto selected = Sidebar::instance().render();

        ImGui::SameLine();

        // Right content panel
        ContentPanel::instance().render(selected);

        ImGui::End();

        // ── Render ───────────────────────────────────────────────────────────
        ImGui::EndFrame();

        m_d3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_d3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        D3DCOLOR clearCol = D3DCOLOR_XRGB(25, 25, 35);
        m_d3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearCol, 1.0f, 0);

        if (m_d3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            m_d3dDevice->EndScene();
        }

        HRESULT hr = m_d3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle device lost
        if (hr == D3DERR_DEVICELOST) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            m_d3dDevice->Reset(&m_d3dpp);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    }

    return 0;
}

// ── Shutdown ────────────────────────────────────────────────────────────────

void App::shutdown() {
    // Save current state
    StateManager::instance().saveToJson("config/last_state.json");

    // Stop device polling
    DeviceManager::instance().stopPolling();

    // Kill all child processes (zombie prevention)
    ProcessManager::instance().cleanup();

    // Shutdown ImGui
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Release DirectX
    if (m_d3dDevice) { m_d3dDevice->Release(); m_d3dDevice = nullptr; }
    if (m_d3d) { m_d3d->Release(); m_d3d = nullptr; }

    // Destroy window
    DestroyWindow(m_hwnd);

    LOG_INFO("Application shut down cleanly");
}
