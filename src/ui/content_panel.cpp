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
#include "utils/encoding.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <sstream>

ContentPanel& ContentPanel::instance() {
    static ContentPanel s_instance;
    return s_instance;
}

// Helper: render an expandable description section
static void renderDescription(const char* descKey) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
    if (ImGui::TreeNodeEx("##desc", ImGuiTreeNodeFlags_NoTreePushOnOpen, "? %s", TR("desc_expand"))) {
        // Not expandable with this style, use CollapsingHeader instead
    }
    ImGui::PopStyleColor();

    // Use a small collapsible header for the description
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.65f, 1.0f));
    if (ImGui::CollapsingHeader(TR("desc_show_help"))) {
        ImGui::TextWrapped("%s", TR(descKey));
        ImGui::Spacing();
    }
    ImGui::PopStyleColor();
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

        // Get translated label
        const char* label = TR(feature.gui_label_en.c_str());

        if (feature.type == "int") {
            int* val = state.getIntRef(feature.id);
            if (feature.max_val > feature.min_val) {
                ImGui::SliderInt(label, val, feature.min_val, feature.max_val);
            } else {
                ImGui::InputInt(label, val);
                if (*val < 0) *val = 0;
            }
        } else if (feature.type == "boolean" || feature.type == "boolean_inverse") {
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

        // Show feature description on hover
        std::string featKey = "feat_" + feature.id;
        const char* featDesc = TR(featKey.c_str());
        if (featDesc != featKey.c_str() && ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(350);
            ImGui::TextUnformatted(featDesc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::PopID();
        if (isDisabled) ImGui::EndDisabled();
    }
}

// ── Running Instances Panel ──────────────────────────────────────────────────

void ContentPanel::renderRunningInstances() {
    auto& pm = ProcessManager::instance();
    auto instances = pm.getInstances();

    std::vector<InstanceInfo> running;
    for (const auto& inst : instances) if (inst.running) running.push_back(inst);
    if (running.empty()) return;

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "%s (%zu):", TR("running_instances"), running.size());
    ImGui::SameLine();
    if (ImGui::SmallButton(TR("btn_stop_all"))) pm.stopAll();

    ImGui::Spacing();

    if (ImGui::BeginTable("InstTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("App");
        ImGui::TableSetupColumn("Display");
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("##");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < instances.size(); ++i) {
            const auto& inst = instances[i];
            if (!inst.running) continue;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", inst.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", inst.appPackage.empty() ? "-" : inst.appPackage.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", inst.vdSize.empty() ? "main" : inst.vdSize.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%lu", inst.processId);
            ImGui::TableNextColumn();
            char btnId[32]; snprintf(btnId, sizeof(btnId), "%s##%zu", TR("btn_stop_scrcpy"), i);
            if (ImGui::SmallButton(btnId)) pm.stopScrcpy(i);
        }
        ImGui::EndTable();
    }
}

// ── Device Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderDevicePanel() {
    ImGui::Text("%s", TR("menu_device"));
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (ImGui::SmallButton(TR("btn_refresh"))) {
        auto& cfg = ConfigManager::instance().getSettings();
        DeviceManager::instance().stopPolling();
        DeviceManager::instance().startPolling(cfg.adbPath, cfg.devicePollingInterval * 1000);
    }
    ImGui::Separator();
    renderDescription("desc_device");

    auto& dm = DeviceManager::instance();
    auto devices = dm.getDevices();

    if (devices.empty()) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "%s", TR("no_device"));
        ImGui::Spacing();
        ImGui::BulletText("%s", TR("dev_usb_hint"));
        ImGui::BulletText("%s", TR("dev_wifi_hint"));
        ImGui::Spacing();
        ImGui::TextDisabled("%s", TR("dev_old_pc_hint"));
    } else {
        ImGui::Text("%s (%zu):", TR("dev_connected"), devices.size());
        ImGui::Spacing();

        if (ImGui::BeginTable("DevTable", 5,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn(TR("dev_select"), ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn(TR("dev_serial"));
            ImGui::TableSetupColumn(TR("dev_model"));
            ImGui::TableSetupColumn(TR("dev_state"), ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn(TR("dev_type"), ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableHeadersRow();

            for (const auto& dev : devices) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool sel = (dev.serial == dm.getSelectedSerial());
                char rid[128]; snprintf(rid, sizeof(rid), "##s_%s", dev.serial.c_str());
                if (ImGui::RadioButton(rid, sel)) dm.setSelectedSerial(dev.serial);

                ImGui::TableNextColumn(); ImGui::Text("%s", dev.serial.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", dev.model.empty() ? "..." : dev.model.c_str());
                ImGui::TableNextColumn();
                if (dev.state == "device") ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "%s", TR("device_online"));
                else if (dev.state == "unauthorized") ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s", TR("device_unauthorized"));
                else ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1), "%s", dev.state.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(dev.isUsb ? ImVec4(0.4f,0.8f,1,1) : ImVec4(0.4f,1,0.4f,1),
                    "%s", dev.isUsb ? "USB" : "WiFi");
            }
            ImGui::EndTable();
        }
    }

    renderRunningInstances();
}

// ── Connection Panel ─────────────────────────────────────────────────────────

void ContentPanel::renderConnectionPanel() {
    ImGui::Text("%s", TR("menu_connection"));
    ImGui::Separator();
    renderDescription("desc_connection");

    ImGui::Text("%s", TR("conn_wifi"));
    ImGui::Spacing();

    static char ipBuf[64] = "";
    static int port = 5555;
    ImGui::InputText(TR("conn_ip"), ipBuf, sizeof(ipBuf));
    ImGui::InputInt(TR("conn_port"), &port);

    if (ImGui::Button(TR("conn_connect_wifi"), ImVec2(200, 0))) {
        if (strlen(ipBuf) > 0) AdbManager::instance().connect(ipBuf, port);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", TR("conn_wizard"));
    ImGui::TextDisabled("%s", TR("conn_step1"));
    ImGui::TextDisabled("%s", TR("conn_step2"));
    ImGui::TextDisabled("%s", TR("conn_step3"));
    ImGui::TextDisabled("%s", TR("conn_step4"));

    auto& dm = DeviceManager::instance();
    std::string serial = dm.getSelectedSerial();
    if (serial.empty()) {
        ImGui::TextDisabled("%s", TR("no_device_selected"));
    } else {
        ImGui::Text("%s: %s", TR("conn_selected"), serial.c_str());
        if (ImGui::Button(TR("conn_enable_tcpip"), ImVec2(250, 0))) {
            AdbManager::instance().enableTcpip(serial, 5555);
        }
    }
}

// ── Display Panel ────────────────────────────────────────────────────────────

void ContentPanel::renderDisplayPanel() {
    ImGui::Text("%s", TR("menu_display"));
    ImGui::Separator();
    renderDescription("desc_display");
    if (StateManager::instance().getBool("otg", false)) {
        ImGui::TextColored(ImVec4(1,0.6f,0.2f,1), "%s", TR("disp_otg_warning"));
        ImGui::Spacing();
    }
    renderFeatureControls("display");
}

// ── Audio Panel ──────────────────────────────────────────────────────────────

void ContentPanel::renderAudioPanel() {
    ImGui::Text("%s", TR("menu_audio"));
    ImGui::Separator();
    renderDescription("desc_audio");
    if (StateManager::instance().getBool("otg", false)) {
        ImGui::TextColored(ImVec4(1,0.6f,0.2f,1), "%s", TR("audio_otg_warning"));
        ImGui::Spacing();
    }
    renderFeatureControls("audio");
}

// ── Control Panel ────────────────────────────────────────────────────────────

void ContentPanel::renderControlPanel() {
    ImGui::Text("%s", TR("menu_control"));
    ImGui::Separator();
    renderDescription("desc_control");
    renderFeatureControls("control");
}

// ── Recording Panel ──────────────────────────────────────────────────────────

void ContentPanel::renderRecordingPanel() {
    ImGui::Text("%s", TR("menu_recording"));
    ImGui::Separator();
    renderDescription("desc_recording");
    renderFeatureControls("recording");
}

// ── Virtual Display Panel ────────────────────────────────────────────────────

void ContentPanel::renderVirtualDisplayPanel() {
    ImGui::Text("%s", TR("menu_virtual_display"));
    ImGui::Separator();
    renderDescription("desc_virtual_display");

    renderFeatureControls("virtual_display");

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1, 1), "%s", TR("vd_launcher"));
    ImGui::TextWrapped("%s", TR("vd_launcher_desc"));
    ImGui::Spacing();

    ImGui::InputText(TR("vd_size"), m_vdSizeBuf, sizeof(m_vdSizeBuf));
    ImGui::InputTextMultiline("##applist", m_vdAppListBuf, sizeof(m_vdAppListBuf), ImVec2(-1, 120));

    auto& dm = DeviceManager::instance();
    std::string serial = dm.getSelectedSerial();

    if (serial.empty()) ImGui::BeginDisabled();

    if (ImGui::Button(TR("vd_launch_all"), ImVec2(300, 0))) {
        auto& cfg = ConfigManager::instance().getSettings();
        auto& pm = ProcessManager::instance();

        std::istringstream iss(m_vdAppListBuf);
        std::string line;
        int launched = 0;
        while (std::getline(iss, line)) {
            size_t s = line.find_first_not_of(" \t\r\n");
            size_t e = line.find_last_not_of(" \t\r\n");
            if (s == std::string::npos) continue;
            std::string pkg = line.substr(s, e - s + 1);
            if (pkg.empty()) continue;

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

            if (pm.startScrcpy(cfg.scrcpyPath, args, serial, meta)) launched++;
        }
        if (launched > 0) LOG_INFO("Launched %d VD(s)", launched);
    }

    if (serial.empty()) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1,0.5f,0.3f,1), "%s", TR("no_device_selected"));
    }

    renderRunningInstances();
}

// ── App Manager Panel ────────────────────────────────────────────────────────

void ContentPanel::renderAppManagerPanel() {
    ImGui::Text("%s", TR("menu_app_manager"));
    ImGui::Separator();
    renderDescription("desc_app_manager");

    auto& dm = DeviceManager::instance();
    auto& am = AppManager::instance();
    auto& cfg = ConfigManager::instance().getSettings();
    std::string serial = dm.getSelectedSerial();

    if (serial.empty()) {
        ImGui::TextColored(ImVec4(1,0.5f,0.3f,1), "%s", TR("no_device_selected"));
        ImGui::Text("%s", TR("select_device_first"));
        return;
    }

    if (am.isRefreshing()) {
        ImGui::Text("%s", TR("app_refreshing"));
    } else {
        if (ImGui::Button(TR("app_refresh_list"), ImVec2(200, 0))) {
            std::thread([serial]() { AppManager::instance().refreshAppList(serial); }).detach();
        }
    }

    ImGui::Spacing();

    auto apps = am.getApps();
    if (apps.empty()) {
        ImGui::TextDisabled("%s", TR("app_no_apps"));
        return;
    }

    ImGui::Text("%s (%zu):", TR("menu_app_manager"), apps.size());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##af", TR("app_filter"), m_appFilterBuf, sizeof(m_appFilterBuf));
    ImGui::Spacing();

    if (ImGui::BeginTable("AppTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
        ImVec2(0, 400))) {
        ImGui::TableSetupColumn("App", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Package", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(TR("dev_state"), ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn(TR("dev_type"), ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("##", ImGuiTableColumnFlags_WidthFixed, 280);
        ImGui::TableHeadersRow();

        for (const auto& app : apps) {
            if (m_appFilterBuf[0] != '\0' && app.packageName.find(m_appFilterBuf) == std::string::npos) continue;

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", app.packageName.c_str());
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s", app.packageName.c_str());
            ImGui::TableNextColumn();
            if (app.enabled) ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "ON");
            else ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1), "OFF");
            ImGui::TableNextColumn();
            ImGui::TextColored(app.isSystem ? ImVec4(0.6f,0.6f,1,1) : ImVec4(1,1,1,1),
                "%s", app.isSystem ? TR("app_system") : TR("app_user"));

            ImGui::TableNextColumn();
            ImGui::PushID(app.packageName.c_str());
            if (app.enabled) {
                if (ImGui::SmallButton(TR("app_freeze"))) am.freezeApp(serial, app.packageName);
            } else {
                if (ImGui::SmallButton(TR("app_unfreeze"))) am.unfreezeApp(serial, app.packageName);
            }
            ImGui::SameLine();
            if (app.enabled && ImGui::SmallButton(TR("app_launch"))) am.launchApp(serial, app.packageName);
            ImGui::SameLine();
            if (app.enabled && ImGui::SmallButton(TR("app_open_vd"))) {
                std::string vdSz = strlen(m_vdSizeBuf) > 0 ? m_vdSizeBuf : "1920x1080";
                am.launchAppOnVirtualDisplay(serial, app.packageName, vdSz);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// ── Camera Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderCameraPanel() {
    ImGui::Text("%s", TR("menu_camera"));
    ImGui::Separator();
    renderDescription("desc_camera");
    renderFeatureControls("camera");
}

// ── Gaming Panel ─────────────────────────────────────────────────────────────

void ContentPanel::renderGamingPanel() {
    ImGui::Text("%s", TR("menu_gaming"));
    ImGui::Separator();
    renderDescription("desc_gaming");
    renderFeatureControls("advanced");
    ImGui::Spacing();
    ImGui::TextDisabled("%s", TR("gaming_otg_desc"));
    ImGui::TextDisabled("%s", TR("gaming_driver_hint"));
}

// ── Advanced Panel ───────────────────────────────────────────────────────────

void ContentPanel::renderAdvancedPanel() {
    ImGui::Text("%s", TR("menu_advanced"));
    ImGui::Separator();
    renderDescription("desc_advanced");

    ImGui::Text("%s", TR("adv_parse_cmd"));
    if (ImGui::InputText("##cmd", m_cmdInputBuf, sizeof(m_cmdInputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        CommandBuilder::instance().parseCommandString(m_cmdInputBuf);
        memset(m_cmdInputBuf, 0, sizeof(m_cmdInputBuf));
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", TR("adv_params"));
    renderFeatureControls("advanced");
}

// ── Logs Panel ───────────────────────────────────────────────────────────────

void ContentPanel::renderLogsPanel() {
    ImGui::Text("%s", TR("menu_logs"));
    ImGui::Separator();
    Logger::instance().renderPanel();
}

// ── Settings Panel ───────────────────────────────────────────────────────────

void ContentPanel::renderSettingsPanel() {
    ImGui::Text("%s", TR("menu_settings"));
    ImGui::Separator();

    auto& i18n = I18n::instance();
    auto& cfg = ConfigManager::instance().getSettings();

    // Language
    ImGui::Text("%s", TR("settings_language"));
    const char* curLang = (i18n.getLanguage() == I18n::Language::Chinese) ? "Chinese" : "English";
    if (ImGui::BeginCombo("##lang", curLang)) {
        if (ImGui::Selectable("English", i18n.getLanguage() == I18n::Language::English)) { i18n.setLanguage(I18n::Language::English); cfg.language = "en"; }
        if (ImGui::Selectable("Chinese", i18n.getLanguage() == I18n::Language::Chinese)) { i18n.setLanguage(I18n::Language::Chinese); cfg.language = "zh"; }
        ImGui::EndCombo();
    }

    // Theme
    ImGui::Text("%s", TR("settings_theme"));
    const char* curTheme = (cfg.theme == "light") ? TR("theme_light") : TR("theme_dark");
    if (ImGui::BeginCombo("##theme", curTheme)) {
        if (ImGui::Selectable(TR("theme_dark"), cfg.theme == "dark")) cfg.theme = "dark";
        if (ImGui::Selectable(TR("theme_light"), cfg.theme == "light")) cfg.theme = "light";
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Profile
    ImGui::Text("%s", TR("settings_profile"));
    if (ImGui::CollapsingHeader(TR("settings_profile_desc"))) {
        ImGui::TextWrapped("%s", TR("settings_profile_desc"));
    }
    static int selProf = -1;
    const char* profNames[] = { "Custom", "Gaming", "Streaming", "Battery Saver" };
    if (ImGui::BeginCombo("##prof", profNames[selProf + 1])) {
        for (int i = 0; i < 4; ++i) {
            bool isSel = (selProf == i - 1);
            if (ImGui::Selectable(profNames[i], isSel)) {
                selProf = i - 1;
                if (selProf >= 0) {
                    const char* files[] = { "gaming", "streaming", "battery_saver" };
                    ConfigManager::instance().loadProfile(files[selProf]);
                }
            }
            if (isSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(TR("settings_save_profile"))) ConfigManager::instance().saveProfile("custom");

    ImGui::Spacing();
    ImGui::Separator();

    // Paths
    ImGui::Text("%s", TR("settings_scrcpy_path"));
    if (ImGui::CollapsingHeader(TR("settings_paths_desc"))) {
        ImGui::TextWrapped("%s", TR("settings_paths_desc"));
    }
    static char scrcpyPath[256] = "";
    static bool pInit = false;
    if (!pInit) { strncpy(scrcpyPath, cfg.scrcpyPath.c_str(), 255); pInit = true; }
    ImGui::InputText("##sp", scrcpyPath, sizeof(scrcpyPath));

    ImGui::Text("%s", TR("settings_adb_path"));
    static char adbPath[256] = "";
    static bool aInit = false;
    if (!aInit) { strncpy(adbPath, cfg.adbPath.c_str(), 255); aInit = true; }
    ImGui::InputText("##ap", adbPath, sizeof(adbPath));

    ImGui::Spacing();
    ImGui::Separator();

    // Polling
    ImGui::Text("%s", TR("settings_polling"));
    if (ImGui::CollapsingHeader(TR("settings_polling_desc"))) {
        ImGui::TextWrapped("%s", TR("settings_polling_desc"));
    }
    ImGui::SliderInt("##poll", &cfg.devicePollingInterval, 5, 60);

    // Icons
    ImGui::Checkbox(TR("settings_fetch_icons"), &cfg.fetchAppIcons);
    if (ImGui::CollapsingHeader(TR("settings_icons_desc"))) {
        ImGui::TextWrapped("%s", TR("settings_icons_desc"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Encoding
    ImGui::Text("%s", TR("settings_encoding"));
    if (ImGui::CollapsingHeader(TR("settings_encoding_desc"))) {
        ImGui::TextWrapped("%s", TR("settings_encoding_desc"));
    }
    const char* encNames[] = {
        "Auto (System Default)", "UTF-8", "GBK (Chinese Simplified)",
        "Big5 (Chinese Traditional)", "Shift-JIS (Japanese)",
        "EUC-KR (Korean)", "Latin-1 (Western)"
    };
    int curEnc = static_cast<int>(Encoding::g_encodingMode);
    if (ImGui::BeginCombo("##enc", encNames[curEnc])) {
        for (int i = 0; i < 7; ++i) {
            if (ImGui::Selectable(encNames[i], curEnc == i))
                Encoding::g_encodingMode = static_cast<Encoding::EncodingMode>(i);
            if (curEnc == i) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button(TR("btn_save"), ImVec2(150, 0))) {
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
        char pbuf[2048];
        strncpy(pbuf, preview.c_str(), sizeof(pbuf) - 1);
        pbuf[sizeof(pbuf) - 1] = '\0';
        ImGui::InputTextMultiline("##pv", pbuf, sizeof(pbuf), ImVec2(-1, 50), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        auto& pm = ProcessManager::instance();
        if (pm.getActiveCount() > 0) {
            if (ImGui::Button(TR("btn_stop_scrcpy"), ImVec2(200, 30))) pm.stopAll();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f,1,0.4f,1), "%s (%zu)", TR("dev_active"), pm.getActiveCount());
        } else {
            if (serial.empty()) ImGui::BeginDisabled();
            if (ImGui::Button(TR("btn_start_scrcpy"), ImVec2(200, 30))) {
                auto& cfg = ConfigManager::instance().getSettings();
                auto args = CommandBuilder::instance().buildArgs(serial);
                if (pm.startScrcpy(cfg.scrcpyPath, args, "", {}))
                    LOG_INFO("scrcpy started for %s", serial.c_str());
            }
            if (serial.empty()) {
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1,0.5f,0.3f,1), "%s", TR("no_device_selected"));
            }
        }
    }
}
