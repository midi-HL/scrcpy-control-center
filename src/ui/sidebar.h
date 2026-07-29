#pragma once
#include <string>
#include <vector>

// Left sidebar navigation panel.
// Renders the 13 menu items as defined in 02_UI_Design.md.
class Sidebar {
public:
    // Menu item identifiers matching the UI design spec
    enum class MenuItem : int {
        Device = 0,
        Connection,
        Display,
        Audio,
        Control,
        Recording,
        VirtualDisplay,
        AppManager,
        Camera,
        Gaming,
        Advanced,
        Logs,
        Settings,
        COUNT // must be last
    };

    static Sidebar& instance();

    // Render the sidebar and return the selected item
    MenuItem render();

    // Get/set selected item programmatically
    MenuItem getSelected() const { return m_selected; }
    void setSelected(MenuItem item) { m_selected = item; }

    // Get display name for a menu item (uses I18n)
    static const char* getItemLabel(MenuItem item);
    static const char* getItemIcon(MenuItem item);

    // Sidebar width in pixels
    static constexpr float WIDTH = 180.0f;

private:
    Sidebar() = default;
    ~Sidebar() = default;
    Sidebar(const Sidebar&) = delete;
    Sidebar& operator=(const Sidebar&) = delete;

    MenuItem m_selected = MenuItem::Device;
};
