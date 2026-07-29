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
                app.m_needsResize = true;  // flag for main loop to reset device
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
        0, wc.lpszClassName, "Scrcpy Control Center",
        WS_OVERLAPPEDWINDOW,
        100, 100, m_windowWidth, m_windowHeight,
        nullptr, nullptr, hInstance, nullptr
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
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    m_d3dpp.BackBufferWidth = m_windowWidth;
    m_d3dpp.BackBufferHeight = m_windowHeight;

    HRESULT hr = m_d3d->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &m_d3dpp, &m_d3dDevice
    );

    if (FAILED(hr)) {
        hr = m_d3d->CreateDevice(
            D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &m_d3dpp, &m_d3dDevice
        );
    }

    if (FAILED(hr)) {
        LOG_ERROR("DirectX 9 device creation failed: 0x%08lx", hr);
        return false;
    }

    LOG_INFO("DirectX 9 initialized (%dx%d)", m_windowWidth, m_windowHeight);
    return true;
}

// ── Reset DX9 Device (called on window resize) ──────────────────────────────

bool App::resetDevice() {
    if (!m_d3dDevice) return false;

    ImGui_ImplDX9_InvalidateDeviceObjects();

    m_d3dpp.BackBufferWidth = m_windowWidth;
    m_d3dpp.BackBufferHeight = m_windowHeight;

    HRESULT hr = m_d3dDevice->Reset(&m_d3dpp);
    if (FAILED(hr)) {
        LOG_ERROR("Device reset failed: 0x%08lx", hr);
        return false;
    }

    ImGui_ImplDX9_CreateDeviceObjects();
    LOG_INFO("Device reset to %dx%d", m_windowWidth, m_windowHeight);
    return true;
}

// ── ImGui Initialization ────────────────────────────────────────────────────

bool App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // ── Load font with CJK (Chinese/Japanese/Korean) support ─────────────────
    // Try to load a CJK font from Windows Fonts directory
    const char* cjkFontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",     // Microsoft YaHei (Chinese Win)
        "C:\\Windows\\Fonts\\msyhbd.ttc",   // Microsoft YaHei Bold
        "C:\\Windows\\Fonts\\simhei.ttf",   // SimHei (older Chinese Win)
        "C:\\Windows\\Fonts\\simsun.ttc",   // SimSun
        "C:\\Windows\\Fonts\\arial.ttf",    // Arial (fallback, no CJK)
        nullptr
    };

    bool fontLoaded = false;
    for (const char* path : cjkFontPaths) {
        if (!path) break;

        // Check if file exists
        DWORD attr = GetFileAttributesA(path);
        if (attr == INVALID_FILE_ATTRIBUTES) continue;

        // Build font config with Chinese glyph ranges
        ImFontConfig fontCfg;
        fontCfg.OversampleH = 1;
        fontCfg.OversampleV = 1;

        // Add Chinese glyph ranges
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x2000, 0x206F, // General Punctuation
            0x3000, 0x30FF, // CJK Symbols, Hiragana, Katakana
            0x31F0, 0x31FF, // Katakana Extensions
            0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
            0x4E00, 0x9FFF, // CJK Unified Ideographs (Chinese characters)
            0x3400, 0x4DBF, // CJK Unified Ideographs Extension A
            0,
        };

        ImFont* font = io.Fonts->AddFontFromFileTTF(path, 16.0f, &fontCfg, ranges);
        if (font) {
            LOG_INFO("Loaded font: %s (with CJK glyphs)", path);
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        // Final fallback: default ImGui font (no CJK)
        io.Fonts->AddFontDefault();
        LOG_WARN("No CJK font found, Chinese text may not display correctly");
    }

    // Build font atlas
    io.Fonts->Build();

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

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
    // DPI awareness for multi-resolution support
    typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
    auto pSetDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(
        GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
    if (pSetDPIAware) {
        pSetDPIAware();
        LOG_INFO("DPI awareness enabled");
    }

    if (!FeatureParser::instance().loadFromFile("config/scrcpy_features.json")) {
        LOG_ERROR("Failed to load feature mapping");
    }

    StateManager::instance().initFromDefaults();
    ConfigManager::instance().init("config");
    StateManager::instance().loadFromJson("config/last_state.json");

    if (!initWindow(hInstance)) return false;
    if (!initDirectX()) return false;
    if (!initImGui()) return false;

    IconManager::instance().init("cache/icons");

    auto& cfg = ConfigManager::instance().getSettings();
    DeviceManager::instance().startPolling(cfg.adbPath, cfg.devicePollingInterval * 1000);

    ProcessManager::instance().setOutputCallback([](const std::string& line) {
        std::string trimmed = line;
        while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r'))
            trimmed.pop_back();
        if (!trimmed.empty()) LOG_INFO("[scrcpy] %s", trimmed.c_str());
    });

    if (cfg.theme == "light") applyLightTheme();

    LOG_INFO("App initialized (poll: %ds, scrcpy: %s, adb: %s)",
             cfg.devicePollingInterval, cfg.scrcpyPath.c_str(), cfg.adbPath.c_str());
    return true;
}

// ── Main Loop ───────────────────────────────────────────────────────────────

int App::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (m_running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) m_running = false;
        }
        if (!m_running) break;

        // ── Handle window resize ─────────────────────────────────────────────
        if (m_needsResize) {
            m_needsResize = false;
            if (m_windowWidth > 0 && m_windowHeight > 0) {
                resetDevice();
            }
        }

        // ── ImGui frame ──────────────────────────────────────────────────────
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

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

        auto selected = Sidebar::instance().render();
        ImGui::SameLine();
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

        // Handle device lost (alt-tab, lock screen, etc.)
        if (hr == D3DERR_DEVICELOST) {
            LOG_WARN("Device lost, waiting for restore...");
            Sleep(100);
            // Try to reset with current window size
            m_d3dpp.BackBufferWidth = m_windowWidth;
            m_d3dpp.BackBufferHeight = m_windowHeight;
            ImGui_ImplDX9_InvalidateDeviceObjects();
            hr = m_d3dDevice->Reset(&m_d3dpp);
            if (SUCCEEDED(hr)) {
                ImGui_ImplDX9_CreateDeviceObjects();
                LOG_INFO("Device restored");
            }
        } else if (hr == D3DERR_DEVICENOTRESET) {
            resetDevice();
        }
    }

    return 0;
}

// ── Shutdown ────────────────────────────────────────────────────────────────

void App::shutdown() {
    StateManager::instance().saveToJson("config/last_state.json");
    DeviceManager::instance().stopPolling();
    ProcessManager::instance().cleanup();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (m_d3dDevice) { m_d3dDevice->Release(); m_d3dDevice = nullptr; }
    if (m_d3d) { m_d3d->Release(); m_d3d = nullptr; }

    DestroyWindow(m_hwnd);
    LOG_INFO("Application shut down");
}
