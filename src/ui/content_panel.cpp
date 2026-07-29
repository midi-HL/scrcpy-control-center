#include "ui/content_panel.h"
#include "core/feature_parser.h"
#include "core/state_manager.h"
#include "core/command_builder.h"
#include "core/process_manager.h"
#include "core/device_manager.h"
#include "core/config_manager.h"
#include "core/adb_manager.h"
#include "core/app_manager.h"
#include "core/icon_manager.h"
#include "utils/logger.h"
#include "utils/i18n.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <sstream>

ContentPanel& ContentPanel::instance() {
    static ContentPanel s_instance;
    return s_instance;
}

void ContentPanel::render(Sidebar::MenuItem selected) {
    ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_None);

    switch (selected) {
        case Sidebar::MenuItem::Device:         renderDevicePanel(); break;
        case Sidebar::MenuItem::Connection:     renderConnectionPanel(); break;
        case Sidebar::MenuItem::Display:        renderDisplayPanel(); break;
        case Sidebar::MenuItem::Audio:          renderAudioPanel(); break;
        case Sidebar::MenuItem::Control:        renderControlPanel(); break;
        case Sidebar::MenuItem::Recording:      renderRecordingPanel(); break;
        case Sidebar::MenuItem::VirtualDisplay: renderVirtualDisplayPanel(); break;
        case Sidebar::MenuItem::AppManager:     renderAppManagerPanel(); break;
        case Sidebar::MenuItem::Camera:         renderCameraPanel(); break;
        case Sidebar::MenuItem::Gaming:         renderGamingPanel(); break;
        case Sidebar::MenuItem::Advanced:       renderAdvancedPanel(); break;
        case Sidebar::MenuItem::Logs:           renderLogsPanel(); break;
        case Sidebar::MenuItem::Settings:       renderSettingsPanel(); break;
        default: ImGui::Text("Unknown panel"); break;
    }

    // Command preview at the bottom (always visible except in Logs/Settings)
    if (selected != Sidebar::MenuItem::Logs && selected != Sidebar::MenuItem::Settings) {
        renderCommandPreview();
    }

    ImGui::EndChild();
}

// ── Dynamic Feature Rendering ────────────────────────────────────────────────

void ContentPanel::renderFeatureControls(const std::string& category) {
    auto& features = FeatureParser::instance();
    auto& state = StateManager::instance();
    auto categoryFeatures = features.getFeaturesByCategory(category);

    for (const auto& feature : categoryFeatures) {
        if (feature.parameter.empty()) continue;

        bool isDisabled = features.isFeatureDisabled(feature.id);
        if (isDisabled) ImGui::BeginDisabled();

        ImGui::PushID(feature.id.c_str());

        const char* label = feature.gui_label_en.c_str();

        if (feature.type == "int") {
            int* val = state.getIntRef(feature.id);
            if (feature.max_val > feature.min_val) {
                ImGui::SliderInt(label, val, feature.min_val, feature.max_val);
            } else {
                ImGui::InputInt(label, val);
                if (*val < 0) *val = 0;
            }
        } else if (feature.type == "boolean") {
            bool* val = state.getBoolRef(feature.id);
            ImGui::Checkbox(label, val);
        } else if (feature.type == "boolean_inverse") {
            bool* val = state.getBoolRef(feature.id);
            ImGui::Checkbox(label, val);
        } else if (feature.type == "enum") {
            std::string* val = state.getStringRef(feature.id);
            if (ImGui::BeginCombo(label, val->c_str())) {
                for (const auto& opt : feature.enum_options) {
                    bool isSelected = (*val == opt);
                    if (ImGui::Selectable(opt.c_str(), isSelected)) *val = opt;
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        } else if (feature.type == "string") {
            std::string* val = state.getStringRef(feature.id);
            char buf[512];
            strncpy(buf, val->c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText(label, buf, sizeof(buf))) *val = buf;
        }

        ImGui::PopID();
        if (isDisabled) ImGui::EndDisabled();
    }
}

// ── Running Instances Panel ──────────────────────────────────────────────────

void ContentPanel::renderRunningInstances() {
    auto& pm = ProcessManager::instance();
    auto instances = pm.getInstances();

    // Filter to only running instances
    std::vector<InstanceInfo> running;
    for (const auto& inst : instances) {
        if (inst.running) running.push_back(inst);
    }

    if (running.empty()) return;

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "Running Instances (%zu):", running.size());
    ImGui::SameLine();
    if (ImGui::SmallButton("Stop All")) {
        pm.stopAll();
    }

    ImGui::Spacing();

    if (ImGui::BeginTable("InstanceTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("App", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Display", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < instances.size(); ++i) {
            const auto& inst = instances[i];
            if (!inst.running) continue;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", inst.name.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", inst.appPackage.empty() ? "-" : inst.appPackage.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", inst.vdSize.empty() ? "main" : inst.vdSize.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%lu", inst.processId);

            ImGui::TableNextColumn();
            char btnId[32];
            snprintf(btnId, sizeof(btnId), "Stop##%zu", i);
            if (ImGui::SmallButton(btnId)) {
                pm.stopScrcpy(i);
            }
        }
        ImGui::EndTable();
    }
}

// ── Device Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderDevicePanel() {
    ImGui::Text(TR("menu_device"));
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (ImGui::SmallButton("Refresh")) {
        auto& cfg = ConfigManager::instance().getSettings();
        DeviceManager::instance().stopPolling();
        DeviceManager::instance().startPolling(cfg.adbPath, cfg.devicePollingInterval * 1000);
    }
    ImGui::Separator();

    auto& dm = DeviceManager::instance();
    auto devices = dm.getDevices();

    if (devices.empty()) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), TR("no_device"));
        ImGui::Spacing();
        ImGui::BulletText("Connect a device via USB cable");
        ImGui::BulletText("Or use Connection tab for WiFi (port 5555)");
        ImGui::Spacing();
        ImGui::TextDisabled("Tip: If your old PC can't USB-connect, use another device");
        ImGui::TextDisabled("to enable wireless ADB, then connect via WiFi here.");
    } else {
        ImGui::Text("Connected Devices (%zu):", devices.size());
        ImGui::Spacing();

        if (ImGui::BeginTable("DeviceTable", 5,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Select", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Serial", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableHeadersRow();

            for (const auto& dev : devices) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                bool isSelected = (dev.serial == dm.getSelectedSerial());
                char radioId[128];
                snprintf(radioId, sizeof(radioId), "##sel_%s", dev.serial.c_str());
                if (ImGui::RadioButton(radioId, isSelected)) {
                    dm.setSelectedSerial(dev.serial);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", dev.serial.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", dev.model.empty() ? "..." : dev.model.c_str());

                ImGui::TableNextColumn();
                if (dev.state == "device") {
                    ImGui::TextColored(ImVec4(0.3f, 1, 0.3f, 1), TR("device_online"));
                } else if (dev.state == "unauthorized") {
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), TR("device_unauthorized"));
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), dev.state.c_str());
                }

                ImGui::TableNextColumn();
                ImGui::TextColored(
                    dev.isUsb ? ImVec4(0.4f, 0.8f, 1, 1) : ImVec4(0.4f, 1, 0.4f, 1),
                    "%s", dev.isUsb ? "USB" : "WiFi");
            }
            ImGui::EndTable();
        }
    }

    // Show running instances
    renderRunningInstances();
}

// ── Connection Panel ─────────────────────────────────────────────────────────

void ContentPanel::renderConnectionPanel() {
    ImGui::Text(TR("menu_connection"));
    ImGui::Separator();

    ImGui::Text("WiFi (TCP/IP) Connection");
    ImGui::Spacing();

    static char ipBuf[64] = "";
    static int port = 5555;
    ImGui::InputText("IP Address", ipBuf, sizeof(ipBuf));
    ImGui::InputInt("Port", &port);

    if (ImGui::Button("Connect via WiFi", ImVec2(200, 0))) {
        if (strlen(ipBuf) > 0) {
            AdbManager::instance().connect(ipBuf, port);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("TCP/IP Setup Wizard");
    ImGui::TextDisabled("1. Connect device via USB (or use another PC)");
    ImGui::TextDisabled("2. Click 'Enable TCP/IP' below");
    ImGui::TextDisabled("3. Disconnect USB cable");
    ImGui::TextDisabled("4. Enter device IP above and connect");

    auto& dm = DeviceManager::instance();
    std::string serial = dm.getSelectedSerial();

    if (serial.empty()) {
        ImGui::TextDisabled("No device selected");
    } else {
        ImGui::Text("Selected: %s", serial.c_str());
        if (ImGui::Button("Enable TCP/IP (port 5555)", ImVec2(250, 0))) {
            AdbManager::instance().enableTcpip(serial, 5555);
        }
    }
}

// ── Display Panel ────────────────────────────────────────────────────────────

void ContentPanel::renderDisplayPanel() {
    ImGui::Text(TR("menu_display"));
    ImGui::Separator();
    if (StateManager::instance().getBool("otg", false)) {
        ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "OTG mode active: video controls disabled.");
        ImGui::Spacing();
    }
    renderFeatureControls("display");
}

// ── Audio Panel ──────────────────────────────────────────────────────────────

void ContentPanel::renderAudioPanel() {
    ImGui::Text(TR("menu_audio"));
    ImGui::Separator();
    if (StateManager::instance().getBool("otg", false)) {
        ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "OTG mode active: audio controls disabled.");
        ImGui::Spacing();
    }
    renderFeatureControls("audio");
}

// ── Control Panel ────────────────────────────────────────────────────────────

void ContentPanel::renderControlPanel() {
    ImGui::Text(TR("menu_control"));
    ImGui::Separator();
    renderFeatureControls("control");
}

// ── Recording Panel ──────────────────────────────────────────────────────────

void ContentPanel::renderRecordingPanel() {
    ImGui::Text(TR("menu_recording"));
    ImGui::Separator();
    renderFeatureControls("recording");
}

// ── Virtual Display Panel ────────────────────────────────────────────────────

void ContentPanel::renderVirtualDisplayPanel() {
    ImGui::Text(TR("menu_virtual_display"));
    ImGui::Separator();

    renderFeatureControls("virtual_display");

    ImGui::Spacing();
    ImGui::Separator();

    // ── Batch VD Launcher ────────────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1, 1), "Multi-Window Launcher");
    ImGui::TextWrapped("Enter one app package per line. Each creates a separate "
                       "resizable virtual display window with --flex-display.");
    ImGui::Spacing();

    ImGui::InputText("VD Size", m_vdSizeBuf, sizeof(m_vdSizeBuf));
    ImGui::InputTextMultiline("##applist", m_vdAppListBuf, sizeof(m_vdAppListBuf),
        ImVec2(-1, 120));

    auto& dm = DeviceManager::instance();
    std::string serial = dm.getSelectedSerial();

    if (serial.empty()) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Launch All as Virtual Displays", ImVec2(300, 0))) {
        auto& cfg = ConfigManager::instance().getSettings();
        auto& pm = ProcessManager::instance();

        // Parse package names (one per line)
        std::istringstream iss(m_vdAppListBuf);
        std::string line;
        int launched = 0;
        while (std::getline(iss, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            std::string pkg = line.substr(start, end - start + 1);
            if (pkg.empty()) continue;

            // Build args for this VD
            std::vector<std::string> args = {
                "--new-display=" + std::string(m_vdSizeBuf),
                "--start-app=" + pkg,
                "--flex-display",
                "--no-vd-system-decorations"
            };

            InstanceInfo meta;
            meta.name = pkg + " - VD " + m_vdSizeBuf;
            meta.appPackage = pkg;
            meta.vdSize = m_vdSizeBuf;
            meta.serial = serial;

            if (pm.startScrcpy(cfg.scrcpyPath, args, serial, meta)) {
                launched++;
                LOG_INFO("VD launched: %s on %s", pkg.c_str(), m_vdSizeBuf);
            } else {
                LOG_ERROR("Failed to launch VD for %s", pkg.c_str());
            }
        }

        if (launched > 0) {
            LOG_INFO("Launched %d virtual display(s)", launched);
        }
    }

    if (serial.empty()) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "(no device)");
    }

    // ── Running Instances ────────────────────────────────────────────────────
    renderRunningInstances();
}

// ── App Manager Panel ────────────────────────────────────────────────────────

void ContentPanel::renderAppManagerPanel() {
    ImGui::Text(TR("menu_app_manager"));
    ImGui::Separator();

    auto& dm = DeviceManager::instance();
    auto& am = AppManager::instance();
    auto& iconMgr = IconManager::instance();
    auto& cfg = ConfigManager::instance().getSettings();
    std::string serial = dm.getSelectedSerial();

    if (serial.empty()) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "No device selected");
        return;
    }

    // Refresh button
    if (am.isRefreshing()) {
        ImGui::Text("Refreshing app list...");
    } else {
        if (ImGui::Button("Refresh App List", ImVec2(200, 0))) {
            std::thread([serial]() {
                AppManager::instance().refreshAppList(serial);
            }).detach();
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Polling: %ds", cfg.devicePollingInterval);

    ImGui::Spacing();

    auto apps = am.getApps();
    if (apps.empty()) {
        ImGui::TextDisabled("No apps loaded. Click 'Refresh App List' to scan device.");
        return;
    }

    ImGui::Text("Apps (%zu):", apps.size());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##app_filter", "Filter...", m_appFilterBuf, sizeof(m_appFilterBuf));

    ImGui::Spacing();

    // App table
    if (ImGui::BeginTable("AppTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
        ImVec2(0, 400))) {
        ImGui::TableSetupColumn("App", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Package", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 280);
        ImGui::TableHeadersRow();

        for (const auto& app : apps) {
            if (m_appFilterBuf[0] != '\0' &&
                app.packageName.find(m_appFilterBuf) == std::string::npos) {
                continue;
            }

            ImGui::TableNextRow();

            // App name / icon
            ImGui::TableNextColumn();
            // TODO: render icon texture if available
            ImGui::Text("%s", app.packageName.c_str());

            // Package name
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", app.packageName.c_str());

            // Status
            ImGui::TableNextColumn();
            if (app.enabled) {
                ImGui::TextColored(ImVec4(0.3f, 1, 0.3f, 1), "ON");
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "OFF");
            }

            // Type
            ImGui::TableNextColumn();
            ImGui::TextColored(app.isSystem ? ImVec4(0.6f, 0.6f, 1, 1) : ImVec4(1, 1, 1, 1),
                               "%s", app.isSystem ? "Sys" : "User");

            // Actions
            ImGui::TableNextColumn();
            ImGui::PushID(app.packageName.c_str());

            // Freeze/Unfreeze
            if (app.enabled) {
                if (ImGui::SmallButton("Freeze")) am.freezeApp(serial, app.packageName);
            } else {
                if (ImGui::SmallButton("Unfreeze")) am.unfreezeApp(serial, app.packageName);
            }
            ImGui::SameLine();

            // Launch (main display)
            if (app.enabled) {
                if (ImGui::SmallButton("Launch")) am.launchApp(serial, app.packageName);
            }
            ImGui::SameLine();

            // Launch on VD (one-click multi-task)
            if (app.enabled && ImGui::SmallButton("Open in VD")) {
                std::string vdSize = strlen(m_vdSizeBuf) > 0 ? m_vdSizeBuf : "1920x1080";
                am.launchAppOnVirtualDisplay(serial, app.packageName, vdSize);
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// ── Camera Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderCameraPanel() {
    ImGui::Text(TR("menu_camera"));
    ImGui::Separator();
    renderFeatureControls("camera");
}

// ── Gaming Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderGamingPanel() {
    ImGui::Text(TR("menu_gaming"));
    ImGui::Separator();
    renderFeatureControls("advanced");
    ImGui::Spacing();
    ImGui::TextDisabled("OTG mode: Direct USB input without mirroring.");
    ImGui::TextDisabled("Requires WinUSB driver (install via Zadig on Win8.1).");
}

// ── Advanced Panel ───────────────────────────────────────────────────────────

void ContentPanel::renderAdvancedPanel() {
    ImGui::Text(TR("menu_advanced"));
    ImGui::Separator();

    ImGui::Text("Paste a scrcpy command to reverse-parse:");
    if (ImGui::InputText("##cmd_parse", m_cmdInputBuf, sizeof(m_cmdInputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue)) {
        CommandBuilder::instance().parseCommandString(m_cmdInputBuf);
        memset(m_cmdInputBuf, 0, sizeof(m_cmdInputBuf));
    }

    ImGui::Spacing();
    ImGui::Separator();
    renderFeatureControls("advanced");
}

// ── Logs Panel ───────────────────────────────────────────────────────────────

void ContentPanel::renderLogsPanel() {
    Logger::instance().renderPanel();
}

// ── Settings Panel ───────────────────────────────────────────────────────────

void ContentPanel::renderSettingsPanel() {
    ImGui::Text(TR("menu_settings"));
    ImGui::Separator();

    auto& i18n = I18n::instance();
    auto& cfg = ConfigManager::instance().getSettings();

    // Language
    ImGui::Text(TR("settings_language"));
    const char* currentLang = (i18n.getLanguage() == I18n::Language::Chinese) ? "Chinese" : "English";
    if (ImGui::BeginCombo("##lang", currentLang)) {
        if (ImGui::Selectable("English", i18n.getLanguage() == I18n::Language::English)) {
            i18n.setLanguage(I18n::Language::English);
            cfg.language = "en";
        }
        if (ImGui::Selectable("Chinese", i18n.getLanguage() == I18n::Language::Chinese)) {
            i18n.setLanguage(I18n::Language::Chinese);
            cfg.language = "zh";
        }
        ImGui::EndCombo();
    }

    // Theme
    ImGui::Text(TR("settings_theme"));
    const char* currentTheme = (cfg.theme == "light") ? "Light" : "Dark";
    if (ImGui::BeginCombo("##theme", currentTheme)) {
        if (ImGui::Selectable("Dark", cfg.theme == "dark")) cfg.theme = "dark";
        if (ImGui::Selectable("Light", cfg.theme == "light")) cfg.theme = "light";
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Profile
    ImGui::Text("Profile (Scene Preset)");
    static int selectedProfile = -1;
    const char* profileNames[] = { "Custom", "Gaming", "Streaming", "Battery Saver" };
    if (ImGui::BeginCombo("##profile", profileNames[selectedProfile + 1])) {
        for (int i = 0; i < 4; ++i) {
            bool isSelected = (selectedProfile == i - 1);
            if (ImGui::Selectable(profileNames[i], isSelected)) {
                selectedProfile = i - 1;
                if (selectedProfile >= 0) {
                    const char* profileFiles[] = { "gaming", "streaming", "battery_saver" };
                    ConfigManager::instance().loadProfile(profileFiles[selectedProfile]);
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Save as Profile")) {
        ConfigManager::instance().saveProfile("custom");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Paths
    ImGui::Text(TR("settings_scrcpy_path"));
    static char scrcpyPath[256] = "";
    static bool pathInit = false;
    if (!pathInit) { strncpy(scrcpyPath, cfg.scrcpyPath.c_str(), 255); pathInit = true; }
    ImGui::InputText("##scrcpy_path", scrcpyPath, sizeof(scrcpyPath));

    ImGui::Text(TR("settings_adb_path"));
    static char adbPath[256] = "";
    static bool adbPathInit = false;
    if (!adbPathInit) { strncpy(adbPath, cfg.adbPath.c_str(), 255); adbPathInit = true; }
    ImGui::InputText("##adb_path", adbPath, sizeof(adbPath));

    ImGui::Spacing();
    ImGui::Separator();

    // Polling interval
    ImGui::Text("Device Polling Interval (seconds)");
    ImGui::SliderInt("##poll", &cfg.devicePollingInterval, 5, 60);

    // Icon fetching toggle
    ImGui::Checkbox("Fetch app icons from device (slower first load)", &cfg.fetchAppIcons);

    if (ImGui::Button("Save Settings", ImVec2(150, 0))) {
        cfg.scrcpyPath = scrcpyPath;
        cfg.adbPath = adbPath;
        ConfigManager::instance().saveSettings();
        LOG_INFO("Settings saved");
    }
}

// ── Command Preview Bar ──────────────────────────────────────────────────────

void ContentPanel::renderCommandPreview() {
    ImGui::Separator();
    if (ImGui::CollapsingHeader(TR("cmd_preview"), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& dm = DeviceManager::instance();
        dm.ensureValidSelection();
        std::string serial = dm.getSelectedSerial();

        std::string preview = CommandBuilder::instance().buildPreviewString("scrcpy", serial);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
        char previewBuf[2048];
        strncpy(previewBuf, preview.c_str(), sizeof(previewBuf) - 1);
        previewBuf[sizeof(previewBuf) - 1] = '\0';
        ImGui::InputTextMultiline("##preview", previewBuf, sizeof(previewBuf),
            ImVec2(-1, 50), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        auto& pm = ProcessManager::instance();

        if (pm.getActiveCount() > 0) {
            if (ImGui::Button(TR("btn_stop_scrcpy"), ImVec2(200, 30))) pm.stopAll();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "Running (%zu)", pm.getActiveCount());
        } else {
            if (serial.empty()) ImGui::BeginDisabled();

            if (ImGui::Button(TR("btn_start_scrcpy"), ImVec2(200, 30))) {
                auto& cfg = ConfigManager::instance().getSettings();
                auto args = CommandBuilder::instance().buildArgs(serial);
                if (pm.startScrcpy(cfg.scrcpyPath, args, "", {})) {
                    LOG_INFO("scrcpy started for device: %s", serial.c_str());
                }
            }

            if (serial.empty()) {
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "No device");
            }
        }
    }
}
