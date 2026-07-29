#include "ui/sidebar.h"
#include "core/device_manager.h"
#include "core/process_manager.h"
#include "core/state_manager.h"
#include "utils/i18n.h"
#include "imgui.h"
#include <cstdio>

Sidebar& Sidebar::instance() {
    static Sidebar s_instance;
    return s_instance;
}

const char* Sidebar::getItemLabel(MenuItem item) {
    switch (item) {
        case MenuItem::Device:         return TR("menu_device");
        case MenuItem::Connection:     return TR("menu_connection");
        case MenuItem::Display:        return TR("menu_display");
        case MenuItem::Audio:          return TR("menu_audio");
        case MenuItem::Control:        return TR("menu_control");
        case MenuItem::Recording:      return TR("menu_recording");
        case MenuItem::VirtualDisplay: return TR("menu_virtual_display");
        case MenuItem::AppManager:     return TR("menu_app_manager");
        case MenuItem::Camera:         return TR("menu_camera");
        case MenuItem::Gaming:         return TR("menu_gaming");
        case MenuItem::Advanced:       return TR("menu_advanced");
        case MenuItem::Logs:           return TR("menu_logs");
        case MenuItem::Settings:       return TR("menu_settings");
        default:                       return "???";
    }
}

const char* Sidebar::getItemIcon(MenuItem item) {
    // Using simple ASCII symbols as icon placeholders
    // In a full implementation, these would be icon font glyphs
    switch (item) {
        case MenuItem::Device:         return "[D]";
        case MenuItem::Connection:     return "[C]";
        case MenuItem::Display:        return "[S]";
        case MenuItem::Audio:          return "[A]";
        case MenuItem::Control:        return "[K]";
        case MenuItem::Recording:      return "[R]";
        case MenuItem::VirtualDisplay: return "[V]";
        case MenuItem::AppManager:     return "[M]";
        case MenuItem::Camera:         return "[P]";
        case MenuItem::Gaming:         return "[G]";
        case MenuItem::Advanced:       return "[X]";
        case MenuItem::Logs:           return "[L]";
        case MenuItem::Settings:       return "[T]";
        default:                       return "[?]";
    }
}

Sidebar::MenuItem Sidebar::render() {
    ImGui::BeginChild("Sidebar", ImVec2(WIDTH, 0), ImGuiChildFlags_None);

    // App title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    ImGui::Text("Scrcpy Control");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Get live counts for badges
    size_t deviceCount = DeviceManager::instance().getDeviceCount();
    size_t activeScrcpy = ProcessManager::instance().getActiveCount();

    // Render menu items
    for (int i = 0; i < static_cast<int>(MenuItem::COUNT); ++i) {
        MenuItem item = static_cast<MenuItem>(i);
        bool isSelected = (m_selected == item);

        // Build label with icon and optional badge
        char label[128];
        if (item == MenuItem::Device && deviceCount > 0) {
            snprintf(label, sizeof(label), "%s %s (%zu)",
                     getItemIcon(item), getItemLabel(item), deviceCount);
        } else {
            snprintf(label, sizeof(label), "%s %s",
                     getItemIcon(item), getItemLabel(item));
        }

        // Style for selected item
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.65f, 1.0f));
        }

        // Full-width button
        if (ImGui::Button(label, ImVec2(-1, 0))) {
            m_selected = item;
        }

        if (isSelected) {
            ImGui::PopStyleColor(2);
        }
    }

    // Show active scrcpy indicator at the bottom
    if (activeScrcpy > 0) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "scrcpy: %zu", activeScrcpy);
    }

    // Show OTG mode warning
    bool otgMode = StateManager::instance().getBool("otg", false);
    if (otgMode) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.6f, 0.2f, 1));
        ImGui::TextWrapped("OTG MODE ACTIVE");
        ImGui::PopStyleColor();
        ImGui::TextDisabled("Audio/Video disabled");
    }

    ImGui::EndChild();
    return m_selected;
}
