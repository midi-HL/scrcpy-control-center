#pragma once
#include "ui/sidebar.h"

// Right-side content panel that renders the active module's UI.
// Dynamically generates ImGui controls from scrcpy_features.json.
class ContentPanel {
public:
    static ContentPanel& instance();

    // Render the content panel for the currently selected sidebar item
    void render(Sidebar::MenuItem selected);

private:
    ContentPanel() = default;
    ~ContentPanel() = default;
    ContentPanel(const ContentPanel&) = delete;
    ContentPanel& operator=(const ContentPanel&) = delete;

    // Individual panel renderers
    void renderDevicePanel();
    void renderConnectionPanel();
    void renderDisplayPanel();
    void renderAudioPanel();
    void renderControlPanel();
    void renderRecordingPanel();
    void renderVirtualDisplayPanel();
    void renderAppManagerPanel();
    void renderCameraPanel();
    void renderGamingPanel();
    void renderAdvancedPanel();
    void renderLogsPanel();
    void renderSettingsPanel();

    // Dynamic feature-based rendering (the core data-driven UI)
    void renderFeatureControls(const std::string& category);

    // Command preview bar at the bottom
    void renderCommandPreview();

    // Running instances panel (shared by VD and main panels)
    void renderRunningInstances();

    // State for command preview input
    char m_cmdInputBuf[1024] = "";

    // State for batch VD launcher
    char m_vdAppListBuf[2048] = "";   // one package per line
    char m_vdSizeBuf[64] = "1920x1080";

    // State for app filter
    char m_appFilterBuf[128] = "";
};
